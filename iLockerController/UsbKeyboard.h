#ifndef _USB_KEYBOARD_H
#define _USB_KEYBOARD_H

#include "FastDelegate.h"
#include "cmsis_os.h"                   // ARM::CMSIS:RTOS:Keil RTX
#include <cstdint>
#include <string>

using namespace fastdelegate;

class Keyboard
{
	private:
		static std::string str;
		static char con;
		static osThreadId readTid;
		static void USBH_Thread (void const *arg);
		static void KeyRead_Thread (void const *arg);
	public:
		typedef FastDelegate1<std::string, bool> LineReadHandler;
		static LineReadHandler OnLineReadEvent;
		static void Init();
		static bool IsConnected() { return con!=0; } 
};

#endif
