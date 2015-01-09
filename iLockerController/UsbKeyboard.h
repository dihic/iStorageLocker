#ifndef _USB_KEYBOARD_H
#define _USB_KEYBOARD_H

#include "FastDelegate.h"
#include <cstdint>
#include <string>

using namespace fastdelegate;

class Keyboard
{
	private:
		static std::string str;
		static char con;
		static void USBH_Thread (void const *arg);
		static void KeyRead_Thread (void const *arg);
	public:
		typedef FastDelegate1<std::string> LineReadHandler;
		static LineReadHandler OnLineReadEvent;
		static void Init();
		static bool IsConnected() { return con!=0; } 
};

#endif
