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
#include "Audio.h"

using namespace fastdelegate;
using namespace IntelliStorage;
using namespace std;

#define AUDIO_SCANID		1
#define AUDIO_GET				2
#define AUDIO_POSITION	3
#define AUDIO_TIMEOUT		4
#define AUDIO_REPEATED	5
#define AUDIO_NONE			6

namespace boost
{
	void throw_exception(const std::exception& ex) {}
}

boost::shared_ptr<CANExtended::CanEx> CanEx;
boost::shared_ptr<NetworkConfig> ethConfig;
boost::shared_ptr<NetworkEngine> ethEngine;
boost::shared_ptr<ConfigComm> configComm;
boost::shared_ptr<SimpleFS> fileSystem;

UnitManager unitManager;

volatile uint8_t CommandState = 0;

void CommandTimeoutDetector()
{
	static bool st = false;
	static uint8_t stcount = 0;
	
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
			Audio::Play(AUDIO_TIMEOUT);
			CommandState = 0;
		}
		if (CommandState==0)
			st = false;
	}
}

void SystemHeartbeat(void const *argument)
{
	static uint8_t hbcount = 20;
	
	HAL_GPIO_TogglePin(STATUS_PIN);
	
	CommandTimeoutDetector();
	
	//Send a heart beat per 10s
	if (ethEngine!=nullptr && ++hbcount>20)
	{
		hbcount = 0;
 		ethEngine->SendHeartBeat();
	}
}
osTimerDef(TimerHB, SystemHeartbeat);

void ReadKBLine(string command)
{
	boost::shared_ptr<StorageUnit> unit;
	string empty;
#ifdef DEBUG_PRINT
	cout<<command<<endl;
#endif
	if (Audio::IsBusy())
		return;
	switch (CommandState)
	{
		case 0:
			if (command == "MAN PUT")
			{
				CommandState = 1;
				Audio::Play(AUDIO_SCANID);
			}
			else if (command.find("MAN PUTID") == 0 && command.length() > 9)
			{
				command.erase(0, 9);
				unit = unitManager.FindEmptyUnit();
				if (unit!=nullptr)
				{
					Audio::Play(AUDIO_POSITION);
					unit->SetPresId(command);
					unit->SetNotice(2, false);
					unit->OpenDoor();
				}
			}
			else if (command.find("MAN OPEN") == 0)
			{
				if (command.length() == 8)
				{
					unit = unitManager.FindEmptyUnit();
					if (unit!=nullptr)
					{
						Audio::Play(AUDIO_POSITION);
						unit->SetNotice(2, false);
						unit->OpenDoor();
					}
				}
				else
				{
					command.erase(0, 8);
					if (command == "ALL")
					{
						auto unitList = unitManager.GetList();
						for(auto it = unitList.begin(); it != unitList.end(); ++it)
						{
							it->second->SetNotice(2, false);
							it->second->OpenDoor();
							if (!it->second->IsRfid())
								it->second->SetPresId(empty);
							osDelay(1000);
						}
					}
					else
					{
						uint16_t id = atoi(command.c_str()) | 0x0100;
						unit = unitManager.FindUnit(id);
						if (unit!=nullptr)
						{
							unit->SetNotice(2, false);
							unit->OpenDoor();
							if (!unit->IsRfid())
								unit->SetPresId(empty);
						}
					}
				}
			}
			else	//User fetching
			{
				unit = unitManager.FindUnit(command);
				if (unit!=nullptr)
				{
					Audio::Play(AUDIO_GET);
					unit->SetNotice(2, false);
					unit->OpenDoor();
					if (!unit->IsRfid())
						unit->SetPresId(empty);
				}
				else
					Audio::Play(AUDIO_NONE);
			}
			break;
		case 1:
			if (command.substr(0,3) == "MAN")
				break;
			unit = unitManager.FindUnit(command);
			if (unit!=nullptr)
			{
				//cout<<"existed!"<<endl;
				Audio::Play(AUDIO_REPEATED);
			}
			else
			{
				unit = unitManager.FindEmptyUnit();
				if (unit!=nullptr)
				{
					Audio::Play(AUDIO_POSITION);
					unit->SetPresId(command);
					unit->SetNotice(2, false);
					unit->OpenDoor();
				}
			}
			CommandState = 0;
			break;
	}
}

void HeartbeatArrival(uint16_t sourceId, const std::uint8_t *data, std::uint8_t len)
{
	static int dc =0;
	CANExtended::DeviceState state = static_cast<CANExtended::DeviceState>(data[0]);
	if (state != CANExtended::Operational)
		return;
	CanEx->Sync(sourceId, SYNC_LIVE, CANExtended::Trigger); //Confirm & Stop
	if (sourceId & 0x100)
	{
		auto unit = unitManager.FindUnit(sourceId);
		if (unit!=nullptr)
		{
#ifdef DEBUG_PRINT
			cout<<"#"<<++dc<<" DeviceID: "<<(sourceId & 0xff)<<" Added"<<endl;
#endif
			unit.reset(new StorageUnit(CanEx, sourceId));
			CanEx->RegisterDevice(unit);
			unit->ReadCommandResponse.bind(ethEngine.get(), &NetworkEngine::DeviceReadResponse);
			unit->WriteCommandResponse.bind(ethEngine.get(), &NetworkEngine::DeviceWriteResponse);
			unitManager.Add(sourceId, unit);
		}
	}
}

static void UpdateWorker (void const *argument)  
{
	while(1)
	{
		configComm->DataReceiver();
		osThreadYield();
	}
}
osThreadDef(UpdateWorker, osPriorityNormal, 1, 0);

static void UpdateUnits(void const *argument)  //Prevent missing status
{
	while(1)
	{
		CanEx->Poll();
		unitManager.Traversal();	//Update all units
		osThreadYield();
	}
}
osThreadDef(UpdateUnits, osPriorityNormal, 1, 0);

//static void UpdateNetwork(void const *argument) 
//{
//	osDelay(3000);
//	while(1)
//	{
//		
//		osThreadYield();
//	}
//}
//osThreadDef(UpdateNetwork, osPriorityHigh, 1, 0);

int main()
{
	HAL_Init();		/* Initialize the HAL Library    */
	osDelay(100);	//Wait for voltage stable
	
	CommStructures::Register();
	
	cout<<"Started..."<<endl;
	
	Keyboard::Init();
	Keyboard::OnLineReadEvent.bind(&ReadKBLine);
	
	configComm = ConfigComm::CreateInstance(Driver_USART2);
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
	
	Audio::Setup();
	Audio::ReadDataDelegate().bind(fileSystem.get(), &SimpleFS::ReadFile);
	Audio::AccessDelegate().bind(fileSystem.get(), &SimpleFS::Access);
	
	//Initialize Ethernet interface
	net_initialize();
	
	ethEngine.reset(new NetworkEngine(ethConfig->GetIpConfig(IpConfigGetServiceEnpoint), unitManager.GetList()));
	ethEngine->StrCommandDelegate.bind(&ReadKBLine);
	ethConfig->ServiceEndpointChangedEvent.bind(ethEngine.get(),&NetworkEngine::ChangeServiceEndpoint);
	unitManager.OnSendData.bind(ethEngine.get(),&NetworkEngine::SendRfidData);
	
#ifdef DEBUG_PRINT
	cout<<"Ethernet SPI Speed: "<<Driver_SPI1.Control(ARM_SPI_GET_BUS_SPEED, 0)<<endl;
	cout<<"Ethernet Inited"<<endl;
#endif

	//Start to find unit devices
	//CanEx->SyncAll(SYNC_LIVE, CANExtended::Trigger);
	
	//Initialize system heatbeat
	osTimerId id = osTimerCreate(osTimer(TimerHB), osTimerPeriodic, NULL);
  osTimerStart(id, 500); 
	
	osThreadCreate(osThread(UpdateWorker), NULL);
	osThreadCreate(osThread(UpdateUnits), NULL);
	
	//Search all units
	for(int8_t i=1;i<0x7f;++i)
	{
		CanEx->Sync(i|0x100, SYNC_LIVE, CANExtended::Trigger);
		osDelay(100);
	}
	
	//osThreadCreate(osThread(UpdateNetwork), NULL);
	
  while(1) 
	{
		net_main();
    ethEngine->Process();
		osThreadYield();
  }
}
