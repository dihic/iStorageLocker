#include <cstdint>
#include <iostream>
#include <string>
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"                   // ARM::CMSIS:RTOS:Keil RTX
#include "rl_net.h"                     // Keil.MDK-Pro::Network:CORE
#include "rl_usb.h"                     // Keil.MDK-Pro::USB:CORE
#include "System.h"
#include "CanEx.h"
#include "NetworkConfig.h"
#include "s25fl064.h"


using namespace std;

boost::shared_ptr<CANExtended::CanEx> CanEx;
boost::shared_ptr<NetworkConfig> ethConfig;

char con = 0;

/*------------------------------------------------------------------------------
 *        USB Host Thread
 *----------------------------------------------------------------------------*/
void USBH_Thread (void const *arg) {
  osPriority priority;                       /* Thread priority               */
  //char KbConnected    =  0;                          /* Connection status of kbd      */
  char con_ex =  40;                         /* Previous connection status
                                                + initial time in 100 ms
                                                intervals for initial disp    */
  //char out    =  1;                          /* Output to keyboard LEDs       */

  USBH_Initialize (0);                       /* Initialize USB Host 0         */
  //USBH_Initialize (1);                     /* Initialize USB Host 1         */
	
	osDelay(100);

  while (1) {
    con = USBH_HID_GetDeviceStatus(0) == usbOK;  /* Get kbd connection status */
    if ((con ^ con_ex) & 1) {                /* If connection status changed  */
      priority = osThreadGetPriority (osThreadGetId());
      osThreadSetPriority (osThreadGetId(), osPriorityAboveNormal);
      if (con) {
        //USBH_HID_Write (0,(uint8_t *)&out,1);/* Turn on NUM LED               */
        cout<<"Keyboard connected"<<endl;
      } else {
        cout<<"Keyboard disconnected ..."<<endl;
      }
      osThreadSetPriority (osThreadGetId(), priority);
      con_ex = con;
    } else if (con_ex > 1) {                 /* If initial time active        */
      con_ex -= 2;                           /* Decrement initial time        */
      if ((con_ex <= 1) && (!con)) {         /* If initial time expired       */
        priority = osThreadGetPriority (osThreadGetId());
        osThreadSetPriority (osThreadGetId(), osPriorityAboveNormal);
        cout<<"No keyboard connected ..."<<endl;
        osThreadSetPriority (osThreadGetId(), priority);
        con_ex = con;
      } else {
        osDelay(200);
      }
    }
    osDelay(100);
  }
}
osThreadDef(USBH_Thread, osPriorityNormal, 1, NULL);

string str;

#define STATE_IDLE 0
#define STATE_PUT  1

uint8_t MgmtState = STATE_IDLE;

void KeyRead_Thread (void const *arg) 
{
	while(1) 
	{
		getline(cin,str);
		if (str.substr(0,2)=="SS")
		{
			str.erase(0,2);
			cout<<str<<endl;
			switch (MgmtState)
			{
				case STATE_IDLE:
					if (str=="DIHBC MMGT PUT")
						MgmtState = STATE_PUT;
					break;
				case STATE_PUT:
					if (str=="DIHBC MMGT PUT")
					break;
				default:
					break;
			}
		}
	}
}

osThreadDef(KeyRead_Thread, osPriorityNormal, 1, NULL);

void SystemHeartbeat(void const *argument)
{
	//cout<<"Heartbeat Start"<<endl;
	HAL_GPIO_TogglePin(STATUS_PIN);
}
osTimerDef(TimerHB, SystemHeartbeat);

Spansion::Flash *nvrom;

int main()
{
	//HAL_Init();		/* Initialize the HAL Library    */
	HAL_MspInit();
	
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
	
	osThreadCreate (osThread(USBH_Thread), NULL);
	osThreadCreate (osThread(KeyRead_Thread), NULL);
	
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

