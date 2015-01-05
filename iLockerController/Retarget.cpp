#include <rt_sys.h>
#include "stm32f4xx.h"
#include "rl_usb.h"                     // Keil.MDK-Pro::USB:CORE
#include <cstring>
#include <exception>

namespace boost
{
	void throw_exception(const std::exception& ex) {}
}
	
#define DEFAULT_HANDLE 	 0x100
#define DUMP_FILE_HANDLE 0x101

enum {BACKSPACE = 0x08, 
		LF        = 0x0A, 
		CR        = 0x0D, 
		CNTLQ     = 0x11, 
		CNTLS     = 0x13, 
		ESC       = 0x1B, 
		DEL       = 0x7F };

/*
 * These names are special strings which will be recognized by 
 * _sys_open and will cause it to return the standard I/O handles, instead
 * of opening a real file.
 */
const char __stdin_name[] ="STDIN";
const char __stdout_name[]="STDOUT";
const char __stderr_name[]="STDERR";
	
volatile int32_t ITM_RxBuffer;

extern "C" {
		
FILEHANDLE _sys_open(const char * name, int openmode)
{
	if (name==__stdin_name || name==__stdout_name || name==__stderr_name)
    return DEFAULT_HANDLE;
	return DUMP_FILE_HANDLE;
}

/*
 * Close a file. Should return 0 on success or a negative value on error.
 * Not required in this implementation. Always return success.
 */
int _sys_close(FILEHANDLE fh)
{
    return 0; //return success
}

/*
 * Write to a file. Returns 0 on success, negative on error, and the number
 * of characters _not_ written on partial success. This implementation sends
 * a buffer of size 'len' to the UART.
 */
int _sys_write(FILEHANDLE fh, const unsigned char * buf,
               unsigned len, int mode)
{
	if (fh!=DEFAULT_HANDLE)
		return 0;
	for(int i=0;i<len;i++) 
	{
			ITM_SendChar(buf[i]);
			// Fix for HyperTerminal    
			if(buf[i]=='\n') 
			{
				ITM_SendChar('\r');
			}
	}
	return 0;
}

/*
 * Read from a file. Can return:
 *  - zero if the read was completely successful
 *  - the number of bytes _not_ read, if the read was partially successful
 *  - the number of bytes not read, plus the top bit set (0x80000000), if
 *    the read was partially successful due to end of file
 *  - -1 if some error other than EOF occurred
 * This function receives a character from the UART, processes the character
 * if required (backspace) and then echo the character to the Terminal 
 * Emulator, printing the correct sequence after successive keystrokes.  
 */
int _sys_read(FILEHANDLE fh, unsigned char * buf,
              unsigned len, int mode)
{
	if (fh!=DEFAULT_HANDLE)
		return 0;
	//int cnt = 0;
	int ch;
	unsigned char *p = buf;
	
	do
	{
		do {
			ch = USBH_HID_GetKeyboardKey (0);
		} while (ch < 0);
		
		len--;
		switch (ch) 
		{
		case CR:                             /* CR - done, stop editing line   */
			//ITM_SendChar(*p = ch = LF);
			*p = ch = LF;
      p++;                            /* increment line pointer         */
		break;
    default:
			//ITM_SendChar(*p = ch);						 /* echo and store character       */
			*p = ch;
			p++;                            /* increment line pointer         */
			break;
		} 
	//} while (cnt < len - 2  &&  ch != LF);      /* check limit and CR             */
	} while (len>1  &&  ch != LF);
	*p = 0;                               		/* mark end of string             */
  return len | 0x80000000;	       
}

/*
 * Writes a character to the output channel. This function is used
 * for last-resort error message output.
 */
void _ttywrch(int ch)
{
    // Convert correctly for endianness change
    char ench=ch;
    ITM_SendChar(ench);
}

/*
 * Return non-zero if the argument file is connected to a terminal.
 */
int _sys_istty(FILEHANDLE fh)
{
    return 1; // no interactive device present
}

/*
 * Move the file position to a given offset from the file start.
 * Returns >=0 on success, <0 on failure. Seeking is not supported for the 
 * UART.
 */
int _sys_seek(FILEHANDLE fh, long pos)
{
    return 0; // error
}

/*
 * Flush any OS buffers associated with fh, ensuring that the file
 * is up to date on disk. Result is >=0 if OK, negative for an
 * error.
 */
int _sys_ensure(FILEHANDLE fh)
{
    return 0; // success
}

/*
 * Return the current length of a file, or <0 if an error occurred.
 * _sys_flen is allowed to reposition the file pointer (so Unix can
 * implement it with a single lseek, for example), since it is only
 * called when processing SEEK_END relative fseeks, and therefore a
 * call to _sys_flen is always followed by a call to _sys_seek.
 */
long _sys_flen(FILEHANDLE fh)
{
    return 0;
}

/*
 * Return the name for temporary file number sig in the buffer
 * name. Returns 0 on failure. maxlen is the maximum name length
 * allowed.
 */
int _sys_tmpnam(char * name, int sig, unsigned maxlen)
{
    return 0; // fail, not supported
}

/*
 * Terminate the program, passing a return code back to the user.
 * This function may not return.
 */
void _sys_exit(int returncode)
{
    while(1) {};
}

}

