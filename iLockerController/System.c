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
  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;

  __PWR_CLK_ENABLE();

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = HSE_VALUE/1000000;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1
                              |RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);

	HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_HSE, RCC_MCODIV_1); //Use HSE as MCO Source with same frequency
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
	__GPIOB_CLK_ENABLE();
	__GPIOC_CLK_ENABLE();
	__GPIOE_CLK_ENABLE();
	
	//Configure PE0 as WP
	//Configure PE1 as HOLD
	gpioType.Pin = GPIO_PIN_0 | GPIO_PIN_1;
	gpioType.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GPIOE, &gpioType);
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_0, GPIO_PIN_SET);	//Source WP Set
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, GPIO_PIN_SET);	//Source HOLD Set

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
	
	//Configure PE2 as PowerAmp PowerDown
	//Configure PE3 as PowerAmp Reset
	gpioType.Pin = GPIO_PIN_2 | GPIO_PIN_3;
	gpioType.Mode = GPIO_MODE_OUTPUT_PP;
	gpioType.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GPIOE, &gpioType);
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_2, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_RESET);
	
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
	
	//PowerAmp Hard Reset
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_2, GPIO_PIN_SET);
	osDelay(5);
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_SET);
	osDelay(15);
}

void HAL_MspInit(void)
{
	InitSystemClock();
	IOSetup();
	
	__RNG_CLK_ENABLE();
	HAL_RNG_Init(&RNGHandle);
}

void HAL_Delay(__IO uint32_t Delay)
{
	osDelay(Delay);
}

#ifdef __cplusplus
}
#endif  /* __cplusplus */

