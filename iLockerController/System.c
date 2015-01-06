#include "stm32f4xx.h"                  // Device header
#include "cmsis_os.h"                   // ARM::CMSIS:RTOS:Keil RTX
#include "System.h"

RNG_HandleTypeDef RNGHandle = { RNG, 
																HAL_UNLOCKED,
																HAL_RNG_STATE_RESET };

const uint8_t *UserFlash = (const uint8_t *)USER_ADDR;

//SYSCLK       168000000
//AHB Clock    168000000
//APB1 Clock   42000000                  
//APB2 Clock   84000000

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

void InitSystemClock()
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

void IOSetup()
{
	GPIO_InitTypeDef gpioType = { GPIO_PIN_All, 
																GPIO_MODE_OUTPUT_PP,
																GPIO_NOPULL,
																GPIO_SPEED_HIGH,
																0 };
	
	__GPIOA_CLK_ENABLE();
	__GPIOC_CLK_ENABLE();
	__GPIOE_CLK_ENABLE();
	
	//Configure PE0 as WP
	//Configure PE1 as HOLD
	gpioType.Pin = GPIO_PIN_0 | GPIO_PIN_1;
	gpioType.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GPIOE, &gpioType);
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_0, GPIO_PIN_SET);	//Source WP Set
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, GPIO_PIN_SET);	//Source HOLD Set

	HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_HSE, RCC_MCODIV_1); //Use HSE as MCO Source with same frequency
	osDelay(1);
	//Configure PA8 as MCO1
	gpioType.Pin = GPIO_PIN_8;
	gpioType.Mode = GPIO_MODE_AF_PP;
	gpioType.Alternate = GPIO_AF0_MCO;
	gpioType.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOA, &gpioType);
	osDelay(1);
	
	//Configure PC0 as LAN Reset
	gpioType.Pin = GPIO_PIN_0;
	gpioType.Mode = GPIO_MODE_OUTPUT_PP;
	gpioType.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GPIOC, &gpioType);
	
	//Configure PC13 as system heartbeat
	gpioType.Pin = GPIO_PIN_13;
	gpioType.Mode = GPIO_MODE_OUTPUT_PP;
	gpioType.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOC, &gpioType);
	

	//Configure PC14 as EINT
	//Configure PC15 as PME
	gpioType.Pin = GPIO_PIN_14 | GPIO_PIN_15;
	gpioType.Mode = GPIO_MODE_INPUT;
	gpioType.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOC, &gpioType);
	
	//Reset Network
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET);
	osDelay(20);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET);
	osDelay(20);	// require >10ms
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET);
}

void HAL_MspInit(void)
{
	InitSystemClock();
	IOSetup();
	
	__RNG_CLK_ENABLE();
	HAL_RNG_Init(&RNGHandle);
}

#ifdef __cplusplus
}
#endif  /* __cplusplus */

