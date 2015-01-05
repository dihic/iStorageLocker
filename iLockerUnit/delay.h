#ifndef DELAY

#ifdef __cplusplus
extern "C" {
#endif
	
extern void SysCtlDelay(unsigned long ulCount);
#define DELAY(us) SysCtlDelay((SystemCoreClock/8000000)*(us))

#ifdef __cplusplus
}
#endif
	
#endif
