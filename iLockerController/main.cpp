#include <cstdint>
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
#include "StorageUnit.h"
#include "FastDelegate.h"

using namespace fastdelegate;
using namespace IntelliStorage;
using namespace std;

#define SYNC_DATA				0x0100
#define SYNC_LIVE				0x01ff

boost::shared_ptr<CANExtended::CanEx> CanEx;
boost::shared_ptr<NetworkConfig> ethConfig;


void SystemHeartbeat(void const *argument)
{
//	static uint8_t hbcount = 20;
	
	HAL_GPIO_TogglePin(STATUS_PIN);
	
//	if (++hbcount>20 && (ethEngine.get()!=NULL))
//	{
//		hbcount = 0;
// 		ethEngine->SendHeartBeat();
//	}
	
	CanEx->SyncAll(SYNC_DATA, CANExtended::Trigger);	//Sync data for all CAN devices
}
osTimerDef(TimerHB, SystemHeartbeat);

Spansion::Flash *nvrom;

void ReadKBLine(string line)
{
	cout<<line<<endl;
}

map<std::uint16_t, boost::shared_ptr<StorageUnit> > UnitList;

void HeartbeatArrival(uint16_t sourceId, CANExtended::DeviceState state)
{
	if (state != CANExtended::Operational)
		return;
	if (sourceId & 0x100)
	{
		map<std::uint16_t, boost::shared_ptr<StorageUnit> >::iterator it = UnitList.find(sourceId);
		if (it == UnitList.end())
		{
			boost::shared_ptr<StorageUnit> unit(new StorageUnit(*CanEx, sourceId));
			CanEx->AddDevice(unit);
			//unit->ReadCommandResponse.bind(ethEngine.get(), &NetworkEngine::DeviceReadResponse);
			//unit->WriteCommandResponse.bind(ethEngine.get(), &NetworkEngine::DeviceWriteResponse);
			UnitList[sourceId] = unit;
		}
		CanEx->Sync(sourceId, SYNC_LIVE, CANExtended::Trigger); //Confirm & Stop
	}
}

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
	
	osDelay(100);
	CanEx->SyncAll(SYNC_LIVE, CANExtended::Trigger);
	
	Keyboard::Init();
	Keyboard::OnLineReadEvent.bind(&ReadKBLine);
	
	//Initialize Ethernet interface
  net_initialize();
	
	cout<<"LAN SPI Speed: "<<Driver_SPI1.Control(ARM_SPI_GET_BUS_SPEED, 0)<<endl;
	
	//Initialize system heatbeat
	osTimerId id = osTimerCreate(osTimer(TimerHB), osTimerPeriodic, NULL);
  osTimerStart(id, 500); 
	
	while(1) 
	{
		net_main();
    osThreadYield();
  }
}

