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

#include "FastDelegate.h"

using namespace fastdelegate;

using namespace std;

boost::shared_ptr<CANExtended::CanEx> CanEx;
boost::shared_ptr<NetworkConfig> ethConfig;


void SystemHeartbeat(void const *argument)
{
	//cout<<"Heartbeat Start"<<endl;
	HAL_GPIO_TogglePin(STATUS_PIN);
}
osTimerDef(TimerHB, SystemHeartbeat);

Spansion::Flash *nvrom;

void ReadKBLine(string line)
{
	cout<<line<<endl;
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
	//CanEx->HeartbeatArrivalEvent.bind(&HeartbeatArrival);
#ifdef DEBUG_PRINT
	cout<<"CAN Inited"<<endl;
#endif
	
	cout<<"Started..."<<endl;
	
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

