#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>

#include "stm32f4xx_hal.h"
#include "cmsis_os.h"                   // ARM::CMSIS:RTOS:Keil RTX
#include "rl_net.h"                     // Keil.MDK-Pro::Network:CORE

#include "System.h"
#include "CanEx.h"
#include "NetworkConfig.h"
#include "SimpleFS.h"
#include "UsbKeyboard.h"
#include "UnitManager.h"
#include "FastDelegate.h"
#include "NetworkEngine.h"
#include "vs10xx.h"
#include "tas5727.h"

using namespace fastdelegate;
using namespace IntelliStorage;
using namespace Skewworks;
using namespace std;

#define SYNC_DATA				0x0100
#define SYNC_LIVE				0x01ff

#define AUDIO_SCANID		1
#define AUDIO_GET				2
#define AUDIO_POSITION	3
#define AUDIO_TIMEOUT		4
#define AUDIO_REPEATED	5
#define AUDIO_NONE			6


boost::shared_ptr<CANExtended::CanEx> CanEx;
boost::shared_ptr<NetworkConfig> ethConfig;
boost::shared_ptr<NetworkEngine> ethEngine;
boost::shared_ptr<ConfigComm> configComm;
boost::shared_ptr<SimpleFS> fileSystem;
boost::shared_ptr<VS10XX> codecs;
boost::shared_ptr<PowerAmp> powerAmp;

UnitManager unitManager;

static uint8_t CommandState = 0;

void PlayAudio(uint8_t index)
{
	if (!codecs->Available())
		return;
	while(codecs->IsBusy());
	uint32_t audioSize = fileSystem->Access(index);
	if (audioSize>0)
		codecs->Play(audioSize);
}

void SystemHeartbeat(void const *argument)
{
	static uint8_t hbcount = 20;
	static uint8_t stcount = 0;
	static bool st = false;
	
	if (st==false)
	{
		if (CommandState==1)
		{
			stcount = 40;
			st = true;
		}
	}
	else
	{
		if (--stcount==0)
		{
			PlayAudio(AUDIO_TIMEOUT);
			CommandState = 0;
		}
		if (CommandState==0)
			st = false;
	}
	
	HAL_GPIO_TogglePin(STATUS_PIN);
	
	//Send a heart beat per 10s
	if (++hbcount>20 && (ethEngine.get()!=NULL))
	{
		hbcount = 0;
 		ethEngine->SendHeartBeat();
	}
	
	//CanEx->SyncAll(SYNC_DATA, CANExtended::Trigger);	//Sync data for all CAN devices
	
}
osTimerDef(TimerHB, SystemHeartbeat);

void ReadKBLine(string command)
{
	boost::shared_ptr<StorageUnit> unit;
	string empty;
	cout<<command<<endl;
	switch (CommandState)
	{
		case 0:
			if (command == "MAN PUT")
			{
				CommandState = 1;
				PlayAudio(AUDIO_SCANID);
			}
			else if (command.find("MAN OPEN") == 0)
			{
				if (command.length() == 8)
				{
					unit = unitManager.FindEmptyUnit();
					if (unit.get()!=NULL)
					{
						PlayAudio(AUDIO_POSITION);
						unit->SetNotice(2, false);
						unit->OpenDoor();
					}
				}
				else
				{
					command.erase(0, 8);
					uint16_t id = atoi(command.c_str()) | 0x0100;
					unit = unitManager.FindUnit(id);
					if (unit.get()!=NULL)
					{
						unit->SetNotice(2, false);
						unit->OpenDoor();
						unit->SetPresId(empty);
					}
				}
			}
			else
			{
				unit = unitManager.FindUnit(command);
				if (unit.get()!=NULL)
				{
					PlayAudio(AUDIO_GET);
					unit->SetNotice(2, false);
					unit->OpenDoor();
					unit->SetPresId(empty);
				}
				else
					PlayAudio(AUDIO_NONE);
			}
			break;
		case 1:
			unit = unitManager.FindUnit(command);
			if (unit.get()!=NULL)
			{
				//cout<<"existed!"<<endl;
				PlayAudio(AUDIO_REPEATED);
			}
			else
			{
				unit = unitManager.FindEmptyUnit();
				if (unit.get()!=NULL)
				{
					PlayAudio(AUDIO_POSITION);
					unit->SetPresId(command);
					unit->SetNotice(2, false);
					unit->OpenDoor();
				}
			}
			CommandState = 0;
			break;
	}
}

void HeartbeatArrival(uint16_t sourceId, CANExtended::DeviceState state)
{
	static int dc =0;
	if (state != CANExtended::Operational)
		return;
	if (sourceId & 0x100)
	{
		boost::shared_ptr<StorageUnit> unit = unitManager.FindUnit(sourceId);
		if (unit.get() == NULL)
		{
			cout<<"#"<<++dc<<" DeviceID: "<<(sourceId & 0xff)<<" Added"<<endl;
			unit.reset(new StorageUnit(*CanEx, sourceId));
			CanEx->AddDevice(unit);
			unit->ReadCommandResponse.bind(ethEngine.get(), &NetworkEngine::DeviceReadResponse);
			unit->WriteCommandResponse.bind(ethEngine.get(), &NetworkEngine::DeviceWriteResponse);
			unitManager.Add(sourceId, unit);
		}
		CanEx->Sync(sourceId, SYNC_LIVE, CANExtended::Trigger); //Confirm & Stop
	}
}

static void UpdateWorker (void const *argument)  
{
	while(1)
	{
		if (configComm.get())
			configComm->DataReceiver();
		//osThreadYield();
	}
}
osThreadDef(UpdateWorker, osPriorityNormal, 1, 0);

static void UpdateUnits(void const *argument)  
{
	while(1)
	{
		std::map<std::uint16_t, boost::shared_ptr<StorageUnit> > unitList = unitManager.GetList();
		for(UnitManager::UnitIterator it = unitList.begin(); it != unitList.end(); ++it)
		{
			CanEx->Sync(it->first, SYNC_DATA, CANExtended::Trigger); 
			osDelay(10);
		}
		unitManager.Traversal();	//Update all units
	}
}
osThreadDef(UpdateUnits, osPriorityNormal, 1, 0);


#define INSTRUCTION_CACHE_ENABLE 			1
#define DATA_CACHE_ENABLE 						1
#define PREFETCH_ENABLE								1

void AudioPlayComplete()
{
	cout<<"Play Completed!"<<endl;
	powerAmp->SetMainVolume(0);
	powerAmp->Shutdown();
}

void AudioHeaderReady(int duration, int bitRate, int layer)
{
	cout<<"Audio Ready"<<endl;
//	cout<<"MP Layer: "<<layer<<endl;
//	cout<<"Bitrate: "<<bitRate<<endl;
//	cout<<"Duration: "<<duration<<" seconds"<<endl<<endl;
	powerAmp->Start();
	powerAmp->SetMainVolume(100);
}

void AudioPositionChanged(int position)
{
	//cout<<"Play on: "<<position<<endl;
}

int main()
{
	HAL_Init();		/* Initialize the HAL Library    */
	osDelay(100);	//Wait for voltage stable
	
	cout<<"Started..."<<endl;
	
	Keyboard::Init();
	Keyboard::OnLineReadEvent.bind(&ReadKBLine);
	
	configComm.reset(new ConfigComm(Driver_USART2));
	configComm->Start();
	
	fileSystem.reset(new SimpleFS(&Driver_SPI2, GPIOB, GPIO_PIN_12, configComm.get()));
	if (fileSystem->IsAvailable())
	{
		if (!fileSystem->IsFormated())
			fileSystem->Format();
#ifdef DEBUG_PRINT
		cout<<"External Flash Available"<<endl;
#endif
	}
#ifdef DEBUG_PRINT
	else
		cout<<"External Flash Missing"<<endl;
#endif
	
	ethConfig.reset(new NetworkConfig(configComm.get()));
	
	//Initialize CAN
	CanEx.reset(new CANExtended::CanEx(Driver_CAN1, 0x001));
	CanEx->HeartbeatArrivalEvent.bind(&HeartbeatArrival);
#ifdef DEBUG_PRINT
	cout<<"CAN Inited"<<endl;
#endif
	
	VSConfig vsconfig = { &Driver_SPI3, 
											  GPIOC, GPIO_PIN_9, 
												GPIOC, GPIO_PIN_8, 
												GPIOC, GPIO_PIN_7, 
											};
	codecs.reset(new VS10XX(&vsconfig));
	codecs->ReadData.bind(fileSystem.get(), &SimpleFS::ReadFile);										
  codecs->PlayComplete.bind(&AudioPlayComplete);
	codecs->HeaderReady.bind(&AudioHeaderReady);
	codecs->PositionChanged.bind(&AudioPositionChanged);
#ifdef DEBUG_PRINT	
	cout<<"Audio Chip: "<<codecs->GetChipVersion()<<endl;
#endif
	
	powerAmp.reset(new PowerAmp(Driver_I2C1));
	if (powerAmp->Available())
	{
		cout<<"PowerAmp Available"<<endl;
		powerAmp->Init(false);										
		cout<<"PowerAmp Inited"<<endl;
	}
	else
		cout<<"PowerAmp Unavailable"<<endl;
	
	//Initialize Ethernet interface
	net_initialize();
	
	ethEngine.reset(new NetworkEngine(ethConfig->GetIpConfig(IpConfigGetServiceEnpoint), unitManager.GetList()));
	ethConfig->ServiceEndpointChangedEvent.bind(ethEngine.get(),&NetworkEngine::ChangeServiceEndpoint);
	unitManager.OnSendData.bind(ethEngine.get(),&NetworkEngine::SendRfidData);
	
#ifdef DEBUG_PRINT
	cout<<"Ethernet SPI Speed: "<<Driver_SPI1.Control(ARM_SPI_GET_BUS_SPEED, 0)<<endl;
	cout<<"Ethernet Inited"<<endl;
#endif

	//Start to find unit devices
	CanEx->SyncAll(SYNC_LIVE, CANExtended::Trigger);
	
	//Initialize system heatbeat
	osTimerId id = osTimerCreate(osTimer(TimerHB), osTimerPeriodic, NULL);
  osTimerStart(id, 500); 
	
	osThreadCreate(osThread(UpdateWorker), NULL);
	osThreadCreate(osThread(UpdateUnits), NULL);

  while(1) 
	{
		net_main();
		CanEx->Poll();
		net_main();
    ethEngine->Process();
    osThreadYield();
  }
}

