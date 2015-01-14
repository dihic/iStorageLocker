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
#include "s25fl064.h"
#include "UsbKeyboard.h"
#include "UnitManager.h"
#include "FastDelegate.h"
#include "NetworkEngine.h"

using namespace fastdelegate;
using namespace IntelliStorage;
using namespace std;

#define SYNC_DATA				0x0100
#define SYNC_LIVE				0x01ff

boost::shared_ptr<CANExtended::CanEx> CanEx;
boost::shared_ptr<NetworkConfig> ethConfig;
boost::shared_ptr<NetworkEngine> ethEngine;
Spansion::Flash *nvrom;
UnitManager unitManager;


void SystemHeartbeat(void const *argument)
{
	static uint8_t hbcount = 20;
	
	HAL_GPIO_TogglePin(STATUS_PIN);
	
	//Send a heart beat per 10s
	if (++hbcount>20 && (ethEngine.get()!=NULL))
	{
		hbcount = 0;
 		ethEngine->SendHeartBeat();
	}
	
	CanEx->SyncAll(SYNC_DATA, CANExtended::Trigger);	//Sync data for all CAN devices
	
	ethEngine->InventoryRfid();
}
osTimerDef(TimerHB, SystemHeartbeat);

void ReadKBLine(string command)
{
	static uint8_t CommandState = 0;
	boost::shared_ptr<StorageUnit> unit;
	cout<<command<<endl;
	switch (CommandState)
	{
		case 0:
			if (command == "MAN PUT")
			{
				CommandState = 1;
			}
			else if (command.find("MAN OPEN") == 0)
			{
				if (command.length() == 8)
				{
					unit = unitManager.FindEmptyUnit();
					if (unit.get()!=NULL)
					{
						unit->SetNotice(1);
						unit->OpenDoor();
					}
				}
				else
				{
					command.erase(0, 8);
					uint16_t id = atoi(command.c_str()) | 0x0100;
					unit = unitManager.FindUnit(id);
					if (unit.get()!=NULL)
						unit->OpenDoor();
				}
			}
			else
			{
				unit = unitManager.FindUnit(command);
				if (unit.get()!=NULL)
				{
					unit->SetNotice(1);
					unit->OpenDoor();
				}
			}
			break;
		case 1:
			unit = unitManager.FindEmptyUnit();
			if (unit.get()!=NULL)
			{
				unit->GetPresId() = command;
				unit->SetNotice(1);
				unit->OpenDoor();
			}
			CommandState = 0;
			break;
	}
}

void HeartbeatArrival(uint16_t sourceId, CANExtended::DeviceState state)
{
	if (state != CANExtended::Operational)
		return;
	if (sourceId & 0x100)
	{
		boost::shared_ptr<StorageUnit> unit = unitManager.FindUnit(sourceId);
		if (unit.get() == NULL)
		{
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
		ethConfig->Poll();
		osThreadYield();
	}
}
osThreadDef(UpdateWorker, osPriorityNormal, 1, 0);

int main()
{
	HAL_Init();		/* Initialize the HAL Library    */
	
	Spansion::Flash source(&Driver_SPI2, GPIOB, GPIO_PIN_12);
	
	cout<<"Flash "<<(source.IsAvailable()?"available":"missing")<<endl;
	
	nvrom = &source;
	
	ethConfig.reset(new NetworkConfig(Driver_USART2));
	
	//Initialize CAN
	CanEx.reset(new CANExtended::CanEx(Driver_CAN1, 0x001));
	CanEx->HeartbeatArrivalEvent.bind(&HeartbeatArrival);
#ifdef DEBUG_PRINT
	cout<<"CAN Inited"<<endl;
#endif
	
	cout<<"Started..."<<endl;
	
	Keyboard::Init();
	Keyboard::OnLineReadEvent.bind(&ReadKBLine);
	
	//Initialize Ethernet interface
  net_initialize();
	
	ethEngine.reset(new NetworkEngine(ethConfig->GetIpConfig(IpConfigGetServiceEnpoint), unitManager.GetList()));
	ethConfig->ServiceEndpointChangedEvent.bind(ethEngine.get(),&NetworkEngine::ChangeServiceEndpoint);
	
#ifdef DEBUG_PRINT
	cout<<"LAN SPI Speed: "<<Driver_SPI1.Control(ARM_SPI_GET_BUS_SPEED, 0)<<endl;
	cout<<"Ethernet Inited"<<endl;
#endif

	//Start to find unit devices
	CanEx->SyncAll(SYNC_LIVE, CANExtended::Trigger);
	
	//Initialize system heatbeat
	osTimerId id = osTimerCreate(osTimer(TimerHB), osTimerPeriodic, NULL);
  osTimerStart(id, 500); 
	
	osThreadCreate(osThread(UpdateWorker), NULL);
	
  while(1) 
	{
		net_main();
		CanEx->Poll();
		net_main();
    ethEngine->Process();
    osThreadYield();
  }
}

