#ifndef _SYSTEM_H
#define _SYSTEM_H

#include "Driver_CAN.h"
#include "Driver_I2C.h"                 // ::CMSIS Driver:I2C
#include "Driver_SPI.h"                 // ::CMSIS Driver:SPI
#include "Driver_USART.h"               // ::CMSIS Driver:USART

extern ARM_DRIVER_CAN Driver_CAN1;
extern ARM_DRIVER_I2C Driver_I2C;
extern ARM_DRIVER_SPI Driver_SPI1;
extern ARM_DRIVER_SPI Driver_SPI2;
extern ARM_DRIVER_SPI Driver_SPI3;
extern ARM_DRIVER_USART Driver_USART2;
extern ARM_DRIVER_USART Driver_USART3;

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

void SetupSystemClock(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
	
#endif
