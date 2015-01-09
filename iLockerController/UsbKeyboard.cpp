#include "UsbKeyboard.h"
#include "cmsis_os.h"                   // ARM::CMSIS:RTOS:Keil RTX
#include "rl_usb.h"                     // Keil.MDK-Pro::USB:CORE

#include <iostream>

using namespace std;

char Keyboard::con = 0;
string Keyboard::str;
Keyboard::LineReadHandler Keyboard::OnLineReadEvent;

/*------------------------------------------------------------------------------
 *        USB Host Thread
 *----------------------------------------------------------------------------*/
void Keyboard::USBH_Thread (void const *arg) {
  osPriority priority;                       /* Thread priority               */
  //char KbConnected    =  0;                          /* Connection status of kbd      */
  char con_ex =  40;                         /* Previous connection status
                                                + initial time in 100 ms
                                                intervals for initial disp    */
  //char out    =  1;                          /* Output to keyboard LEDs       */

  USBH_Initialize (0);                       /* Initialize USB Host 0         */
	
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

void Keyboard::KeyRead_Thread (void const *arg) 
{
	while(1) 
	{
		getline(cin,str);
		if (str.substr(0,2)=="SS")
		{
			str.erase(0,2);
			if (OnLineReadEvent)
				OnLineReadEvent(str);
		}
	}
}

void Keyboard::Init()
{
	osThreadDef_t threadDef;
	threadDef.pthread = USBH_Thread;
	threadDef.tpriority = osPriorityNormal;
	threadDef.instances = 1;
	threadDef.stacksize = 0;
	osThreadCreate(&threadDef, NULL);
	threadDef.pthread = KeyRead_Thread;
	osThreadCreate(&threadDef, NULL);
}

