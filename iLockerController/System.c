#include "stm32f4xx.h"                  // Device header
#include "stm32f4xx_hal_conf.h"         // Keil::Device:STM32Cube Framework:Classic

#include "System.h"

//SYSCLK       168000000
//AHB Clock    168000000
//APB1 Clock   42000000                  
//APB2 Clock   84000000

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

void SetupSystemClock()
{
		  /* Enable Power Control clock */
  __PWR_CLK_ENABLE();
  
  /* The voltage scaling allows optimizing the power consumption when the device is 
     clocked below the maximum system frequency, to update the voltage scaling value 
     regarding system frequency refer to product datasheet.  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
	
	RCC_OscInitTypeDef oscType = {
			RCC_OSCILLATORTYPE_HSE,
			RCC_HSE_ON,
			RCC_LSE_OFF,
			RCC_HSI_ON,
			0x00,
			RCC_LSI_ON,
			{
				RCC_PLL_ON,
				RCC_PLLSOURCE_HSE,
				HSE_VALUE/1000000,
				336,
				RCC_PLLP_DIV2,
				7
			}
		};
	
	HAL_RCC_OscConfig(&oscType);
	
	RCC_ClkInitTypeDef clkType = {
			RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2,
			RCC_SYSCLKSOURCE_PLLCLK,		//System Clock Source (SYSCLKS)
			RCC_SYSCLK_DIV1,						//AHB Clock
			RCC_HCLK_DIV4,							//APB1 Clock
			RCC_HCLK_DIV2 							//APB2 Clock
		};
	
	HAL_RCC_ClockConfig(&clkType, FLASH_LATENCY_5);
		
	SystemCoreClockUpdate();
}

#ifdef __cplusplus
}
#endif  /* __cplusplus */

