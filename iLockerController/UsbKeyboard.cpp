#include "UsbKeyboard.h"
#include "rl_usb.h"                     // Keil.MDK-Pro::USB:CORE

#include <iostream>

using namespace std;

char Keyboard::con = 0;
osThreadId Keyboard::readTid = NULL;
string Keyboard::str;
Keyboard::LineReadHandler Keyboard::OnLineReadEvent;

/*------------------------------------------------------------------------------
 *        USB Host Thread
 *----------------------------------------------------------------------------*/
void Keyboard::USBH_Thread (void const *arg) {
  //osPriority priority;                       /* Thread priority               */
  //char KbConnected    =  0;                          /* Connection status of kbd      */
  char con_ex =  40;                         /* Previous connection status
                                                + initial time in 100 ms
                                                intervals for initial disp    */
  //char out    =  1;                          /* Output to keyboard LEDs       */

  USBH_Initialize (0);                       /* Initialize USB Host 0         */
	//USBH_Initialize (1);
	
	osDelay(100);

	
  while (1) {
    con = USBH_HID_GetDeviceStatus(0) == usbOK;  /* Get kbd connection status */
    if ((con ^ con_ex) & 1) {                /* If connection status changed  */
 //     priority = osThreadGetPriority(readTid);
 //     osThreadSetPriority(readTid, osPriorityAboveNormal);
      if (con) {
        //USBH_HID_Write (0,(uint8_t *)&out,1);/* Turn on NUM LED               */
        cout<<"Keyboard connected"<<endl;
      } else {
        cout<<"Keyboard disconnected ..."<<endl;
      }
      //osThreadSetPriority (osThreadGetId(), priority);
      con_ex = con;
    } else if (con_ex > 1) {                 /* If initial time active        */
      con_ex -= 2;                           /* Decrement initial time        */
      if ((con_ex <= 1) && (!con)) {         /* If initial time expired       */
//				osThreadSetPriority(readTid, osPriorityNormal);
//        priority = osThreadGetPriority (osThreadGetId());
//        osThreadSetPriority (osThreadGetId(), osPriorityNormal);
        cout<<"No keyboard connected ..."<<endl;
//        osThreadSetPriority (osThreadGetId(), priority);
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
	int ch;
	while(1) 
	{
		//getline(cin,str);
		str.clear();
		while(1)
		{
			do {
				ch = USBH_HID_GetKeyboardKey(0);
			} while (ch < 0);
			//osThreadSetPriority (osThreadGetId(), osPriorityAboveNormal);
			if (ch==0x0A || ch==0x0D)
				break;
			str.push_back(ch);
		}
		//osThreadSetPriority (osThreadGetId(), osPriorityNormal);
		
		if (str.length()>2 && str.substr(0,2)=="SS")
		{
			str.erase(0,2);
			if (OnLineReadEvent)
				OnLineReadEvent(str);
		}
#ifdef DEBUG_PRINT
		else
			cout<<"Scan Error: "<<str<<endl;
#endif
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
	threadDef.tpriority = osPriorityNormal;
	threadDef.pthread = KeyRead_Thread;
	readTid = osThreadCreate(&threadDef, NULL);
}

