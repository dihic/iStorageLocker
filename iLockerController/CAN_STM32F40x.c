#include <cmsis_os.h>                     /* RTX kernel functions & defines      */
#include "Driver_CAN.h"                  /* CAN Generic functions & defines     */
#include <RTE_Device.h>
#include <string.h>
#include "stm32f4xx_hal.h"


#define ARM_CAN_DRV_VERSION ARM_DRIVER_VERSION_MAJOR_MINOR(1,0)   /* driver version */

#define __CTRL1                 1
#define __CTRL2                 2

#define __FILTER_BANK_MAX      27     /* filter banks 0..27 */


/************************* CAN Hardware Configuration ************************/

// *** <<< Use Configuration Wizard in Context Menu >>> ***

// <o> CAN Peripheral Clock (in Hz) <1-1000000000>
//     <i> Same as APB1 clock
#define CAN_CLK               42000000

// *** <<< End of Configuration section             >>> ***


#if    (!RTE_CAN1 && !RTE_CAN2)
#error "CAN not configured in CanConfig.h!"
#endif

/*----------------------------------------------------------------------------
 *      CAN RTX Hardware Specific Driver Functions
 *----------------------------------------------------------------------------
 *  Functions implemented in this module:
 *    static      void CAN_set_timing      (U32 ctrl, U32 tseg1, U32 tseg2, U32 sjw, U32 brp)
 *    static CAN_ERROR CAN_hw_set_baudrate (U32 ctrl, U32 baudrate)
 *           CAN_ERROR CAN_hw_setup        (U32 ctrl)
 *           CAN_ERROR CAN_hw_init         (U32 ctrl, U32 baudrate)
 *           CAN_ERROR CAN_hw_start        (U32 ctrl)
 *           CAN_ERROR CAN_hw_testmode     (U32 ctrl, U32 testmode)
 *           CAN_ERROR CAN_hw_tx_empty     (U32 ctrl)
 *           CAN_ERROR CAN_hw_wr           (U32 ctrl,         CAN_msg *msg)
 *    static      void CAN_hw_rd           (U32 ctrl, U32 ch, CAN_msg *msg)
 *           CAN_ERROR CAN_hw_set          (U32 ctrl,         CAN_msg *msg)
 *           CAN_ERROR CAN_hw_rx_object    (U32 ctrl, U32 ch, U32 id, U32 object_para)
 *           CAN_ERROR CAN_hw_tx_object    (U32 ctrl, U32 ch,         U32 object_para)
 *    Interrupt fuction
 *---------------------------------------------------------------------------*/

/* Static functions used only in this module                                 */
static U8 CAN_hw_rx_object_chk (U16 bank, U32 object_para);

#if RTE_CAN1 == 1
CAN_HandleTypeDef hCAN1;
CanRxMsgTypeDef canRxMsg1;
CanTxMsgTypeDef canTxMsg1;
#endif
#if RTE_CAN2 == 1
CAN_HandleTypeDef hCAN2;
CanRxMsgTypeDef canRxMsg2;
CanTxMsgTypeDef canTxMsg2;
#endif


/* Driver Version */
static const ARM_DRIVER_VERSION CanDriverVersion = {
  ARM_CAN_API_VERSION,
  ARM_CAN_DRV_VERSION
};

/**
  \fn          ARM_DRV_VERSION SPI_GetVersion (void)
  \brief       Get driver version.
  \return      \ref ARM_DRV_VERSION
*/
static ARM_DRIVER_VERSION CANX_GetVersion (void) {
  return CanDriverVersion;
}


/************************* Auxiliary Functions *******************************/

/*************************** Module Functions ********************************/

void HAL_CAN_MspInit(CAN_HandleTypeDef* hcan)
{
  GPIO_InitTypeDef GPIO_InitStruct;
  if(hcan->Instance==CAN1)
  {
    /* Peripheral clock enable */
    __CAN1_CLK_ENABLE();
		
		hcan->pRxMsg = &canRxMsg1;
		hcan->pTxMsg = &canTxMsg1;
		hcan->Instance->MCR &= (~(uint32_t)CAN_MCR_DBF);
		
		if (RTE_CAN1_RX_PORT==GPIOB || RTE_CAN1_TX_PORT==GPIOB)
			__GPIOB_CLK_ENABLE();
		if (RTE_CAN1_RX_PORT==GPIOD || RTE_CAN1_TX_PORT==GPIOD)
			__GPIOD_CLK_ENABLE();
  
		GPIO_InitStruct.Pin = 1<<RTE_CAN1_RX_BIT;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_CAN1;
    HAL_GPIO_Init(RTE_CAN1_RX_PORT, &GPIO_InitStruct);
		GPIO_InitStruct.Pin = 1<<RTE_CAN1_TX_BIT;
		HAL_GPIO_Init(RTE_CAN1_TX_PORT, &GPIO_InitStruct);

  /* System interrupt init*/
    HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
    HAL_NVIC_SetPriority(CAN1_TX_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(CAN1_TX_IRQn);
    HAL_NVIC_SetPriority(CAN1_SCE_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(CAN1_SCE_IRQn);
  }
}

void HAL_CAN_MspDeInit(CAN_HandleTypeDef* hcan)
{
  if(hcan->Instance==CAN1)
  {
    /* Peripheral clock disable */
    __CAN1_CLK_DISABLE();
  
		HAL_GPIO_DeInit(RTE_CAN1_RX_PORT, 1<<RTE_CAN1_RX_BIT);
		HAL_GPIO_DeInit(RTE_CAN1_TX_PORT, 1<<RTE_CAN1_TX_BIT);

    /* Peripheral interrupt DeInit*/
		HAL_NVIC_DisableIRQ(CAN1_RX0_IRQn);
    HAL_NVIC_DisableIRQ(CAN1_RX1_IRQn);
    HAL_NVIC_DisableIRQ(CAN1_TX_IRQn);
    HAL_NVIC_DisableIRQ(CAN1_SCE_IRQn);
  }

}

/*--------------------------- CAN_hw_init -----------------------------------
 *
 *  Initialize the CAN hardware
 *
 *  Parameter:  ctrl:       Index of the hardware CAN controller (1 .. x)
 *              baudrate:   Baudrate
 *
 *  Return:     CAN_ERROR:  Error code
 *---------------------------------------------------------------------------*/

CAN_ERROR CAN_hw_init (U32 ctrl, U32 baudrate)  
{
	CAN_HandleTypeDef *hCAN;
	switch (ctrl)
	{
#if RTE_CAN1 == 1
		case __CTRL1:
			hCAN = &hCAN1;
			hCAN->Instance = CAN1;
			break;
#endif
#if RTE_CAN2 == 1
		case __CTRL2:
			hCAN= &hCan2;
			hCAN->Instance = CAN2;
			break;
#endif
		default:
			return CAN_UNEXIST_CTRL_ERROR;
	}
	
	hCAN->Init.Mode = CAN_MODE_NORMAL;
	//hCAN->Init.Mode = CAN_MODE_SILENT_LOOPBACK;
	hCAN->Init.TXFP = ENABLE;
	hCAN->Init.NART = DISABLE;
	hCAN->Init.ABOM = DISABLE;
	hCAN->Init.AWUM = DISABLE;
	hCAN->Init.TTCM = DISABLE;
	hCAN->Init.RFLM = DISABLE;
	
	if (baudrate <= 1000000)
    hCAN->Init.Prescaler = (CAN_CLK / 7) / baudrate;
	else
		return CAN_BAUDRATE_ERROR;
	
//	hCAN->Init.BS1 = CAN_BS1_12TQ;
//	hCAN->Init.BS2 = CAN_BS2_2TQ;
//	hCAN->Init.SJW = CAN_SJW_1TQ;
	
	hCAN->Init.BS1 = CAN_BS1_4TQ;
	hCAN->Init.BS2 = CAN_BS2_2TQ;
	hCAN->Init.SJW = CAN_SJW_3TQ;

	if (HAL_CAN_Init(hCAN) != HAL_OK)
		return CAN_INIT_ERROR;
	
	return CAN_OK;
}

/*--------------------------- CAN_hw_start ----------------------------------
 *
 *  reset CAN initialisation mode
 *
 *  Parameter:  ctrl:       Index of the hardware CAN controller (1 .. x)
 *
 *  Return:     CAN_ERROR:  Error code
 *---------------------------------------------------------------------------*/

CAN_ERROR CAN_hw_start (U32 ctrl)  
{
	CAN_HandleTypeDef *hCAN;
	switch (ctrl)
	{
#if RTE_CAN1 == 1
		case __CTRL1:
			hCAN = &hCAN1;
			break;
#endif
#if RTE_CAN2 == 1
		case __CTRL2:
			hCAN= &hCan2;
			break;
#endif
		default:
			return CAN_UNEXIST_CTRL_ERROR;
	}
	if (hCAN->State==HAL_CAN_STATE_ERROR | hCAN->State == HAL_CAN_STATE_RESET)
		return CAN_INIT_ERROR;
	
	HAL_CAN_Receive_IT(hCAN, CAN_FIFO0);
  return CAN_OK;
}

/*--------------------------- CAN_set_testmode ------------------------------
 *
 *  Setup the CAN testmode
 *
 *  Parameter:  ctrl:       Index of the hardware CAN controller (1 .. x)
 *              testmode:   Kind of testmode
 *
 *  Return:     CAN_ERROR:  Error code
 *---------------------------------------------------------------------------*/

//CAN_ERROR CAN_hw_testmode (U32 ctrl, U32 testmode) {
//  CAN_HandleTypeDef *hCAN;
//	switch (ctrl)
//	{
//#if RTE_CAN1 == 1
//		case __CTRL1:
//			hCAN = &hCAN1;
//			break;
//#endif
//#if RTE_CAN2 == 1
//		case __CTRL2:
//			hCAN= &hCan2;
//			break;
//#endif
//		default:
//			return CAN_UNEXIST_CTRL_ERROR;
//	}

//  CANx->BTR &= ~(CAN_BTR_LBKM | CAN_BTR_SILM); 
//  CANx->BTR |=  (testmode & (CAN_BTR_LBKM | CAN_BTR_SILM));

//  return CAN_OK;
//}

/*--------------------------- CAN_hw_tx_empty -------------------------------
 *
 *  Check if transmit mailbox 0 is available for usage (empty)
 *
 *  Parameter:  ctrl:       Index of the hardware CAN controller (1 .. x)
 *
 *  Return:     CAN_ERROR:  Error code
 *---------------------------------------------------------------------------*/

CAN_ERROR CAN_hw_tx_empty (U32 ctrl)  
{
	HAL_CAN_StateTypeDef state;
  CAN_HandleTypeDef *hCAN;
	
	int av = osSemaphoreWait(wr_sem[ctrl-1], 0);
	
  if (av >0)
	{
		switch (ctrl)
		{
	#if RTE_CAN1 == 1
			case __CTRL1:
				hCAN = &hCAN1;
				break;
	#endif
	#if RTE_CAN2 == 1
			case __CTRL2:
				hCAN= &hCan2;
				break;
	#endif
			default:
				return CAN_UNEXIST_CTRL_ERROR;
		}
		state = HAL_CAN_GetState(hCAN);
		if((state == HAL_CAN_STATE_READY) || (state == HAL_CAN_STATE_BUSY_RX))
			return CAN_OK;
		else 
			osSemaphoreRelease(wr_sem[ctrl-1]);    /* Return a token back to semaphore    */
	}

  return CAN_TX_BUSY_ERROR;
}

/*--------------------------- CAN_hw_wr -------------------------------------
 *
 *  Write CAN_msg to the hardware registers of the requested controller
 *
 *  Parameter:  ctrl:       Index of the hardware CAN controller (1 .. x)
 *              msg:        Pointer to CAN message to be written to hardware
 *
 *  Return:     CAN_ERROR:  Error code
 *---------------------------------------------------------------------------*/

CAN_ERROR CAN_hw_wr (U32 ctrl, CAN_msg *msg)  {
	uint8_t i;
	
  CAN_HandleTypeDef *hCAN;
	switch (ctrl)
	{
#if RTE_CAN1 == 1
		case __CTRL1:
			hCAN = &hCAN1;
			break;
#endif
#if RTE_CAN2 == 1
		case __CTRL2:
			hCAN= &hCan2;
			break;
#endif
		default:
			return CAN_UNEXIST_CTRL_ERROR;
	}
	
	hCAN->pTxMsg->DLC = msg->len;
	for(i=0;i<msg->len;++i)
		hCAN->pTxMsg->Data[i] = msg->data[i];
	hCAN->pTxMsg->RTR = (msg->type == REMOTE_FRAME) ? CAN_RTR_REMOTE : CAN_RTR_DATA;
	if (msg->format==EXTENDED_FORMAT)
	{
		hCAN->pTxMsg->IDE = CAN_ID_EXT;
		hCAN->pTxMsg->ExtId = msg->id & 0x1fffffff;
	}
	else
	{
		hCAN->pTxMsg->IDE = CAN_ID_STD;
		hCAN->pTxMsg->StdId = msg->id & 0x7ff;
	}
	
	HAL_StatusTypeDef result = HAL_CAN_Transmit_IT(hCAN);
	while (result == HAL_TIMEOUT)
	{
		osDelay(5);
		result = HAL_CAN_Transmit_IT(hCAN);
	}
//	if (HAL_CAN_Transmit_IT(hCAN) != HAL_OK)
	if (result != HAL_OK)
		return CAN_TX_BUSY_ERROR;
	return CAN_OK;
}

/*--------------------------- CAN_hw_set ------------------------------------
 *  Set a message that will automatically be sent as an answer to the REMOTE
 *  FRAME message
 *
 *  Parameter:  ctrl:       Index of the hardware CAN controller (1 .. x)
 *              msg:        Pointer to CAN message to be set
 *
 *  Return:     CAN_ERROR:  Error code
 *---------------------------------------------------------------------------*/

CAN_ERROR CAN_hw_set (U32 ctrl, CAN_msg *msg)  {

  return CAN_NOT_IMPLEMENTED_ERROR;
}

/*--------------------------- CAN_hw_rx_object_chk --------------------------
 *
 *  This function checks if an object that is going to be used for the message 
 *  reception can be added to the Filter Bank
 *
 *  Parameter:  ctrl:       Index of the hardware CAN controller (1 .. x)
 *              bank:       Index of the Filter bank (0..__FILTER_BANK_MAX)
 *              object_para:Object parameters (standard or extended format, 
 *                          data or remote frame)
 *
 *  Return:     BOOL:  True   Object can     be added to Filter Bank
 *                     False  Object can not be added to Filter Bank
 *---------------------------------------------------------------------------*/

U8 CAN_hw_rx_object_chk(U16 bank, U32 object_para)  
{
  if ((CAN1->FA1R & (1UL << bank)) == 0)                             /* filter bank unused ? */
    return 0;
	/* check if another identifier is possible */
	if ((CAN1->FS1R & (1UL << bank)) == 0)		/* 16-bit identifier format is used */
	{
		if ((object_para & FILTER_LIST_TYPE) == 0 && (CAN1->FM1R & (1UL << bank)) == 0)	//Mask mode
		{
			if (CAN1->sFilterRegister[bank].FR1 == 0)				/* check if position n is used */
				return 0;
			else if (CAN1->sFilterRegister[bank].FR2 == 0)	/* check if position n+1 is used */
				return 1;
			else
				return 0xff;
		}
		if ((object_para & FILTER_LIST_TYPE) != 0 && (CAN1->FM1R & (1UL << bank)) != 0)	//List mode
		{
			if ((CAN1->sFilterRegister[bank].FR1 & 0x0000FFFF) == 0)		/* check if position n is used */
				return 0;
			if ((CAN1->sFilterRegister[bank].FR1 & 0xFFFF0000) == 0)  /* check if position n+1 is used */
				return 1;
			else if ((CAN1->sFilterRegister[bank].FR2 & 0x0000FFFF) == 0)   /* check if position n+2 is used */
				return 2;
			else if ((CAN1->sFilterRegister[bank].FR2 & 0xFFFF0000) == 0)   /* check if position n+3 is used */
				return 3;
			else
				return 0xff;
		}
	}
	else		/* 32-bit identifier format is used */
	{
		if ((object_para & FILTER_LIST_TYPE) == 0 && (CAN1->FM1R & (1UL << bank)) == 0)	//Mask mode
		{
			return 0xff;
		}
		if ((object_para & FILTER_LIST_TYPE) != 0 && (CAN1->FM1R & (1UL << bank)) != 0)	//List mode
		{
			if (CAN1->sFilterRegister[bank].FR1 == 0)				/* check if position n is used */
				return 0;
			else if (CAN1->sFilterRegister[bank].FR2 == 0)	/* check if position n+1 is used */
				return 1;
			else
				return 0xff;
		}
	}
	return 0xff;
}

/*--------------------------- CAN_hw_rx_object ------------------------------
 *
 *  This function setups object that is going to be used for the message 
 *  reception
 *
 *  Parameter:  ctrl:       Index of the hardware CAN controller (1 .. x)
 *              ch:         Index of object used for reception
 *              id:         Identifier of receiving messages
 *              object_para:Object parameters (standard or extended format, 
 *                          data or remote frame)
 *
 *  Return:     CAN_ERROR:  Error code
 *---------------------------------------------------------------------------*/

CAN_ERROR CAN_hw_rx_object (U32 ctrl, U32 ch, U32 id, U32 object_para, U32 mask)  
{
  static uint32_t sb = __FILTER_BANK_MAX;
  U8 filterIdx = 0;
	
	CAN_FilterConfTypeDef filter;
	
	filter.FilterActivation = ENABLE;
	filter.FilterMode = CAN_FILTERMODE_IDMASK;
	filter.FilterScale = CAN_FILTERSCALE_32BIT;
	filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;	
	
	filter.BankNumber = sb; //(CAN2_filterIdx == 0) ? __FILTER_BANK_MAX : __FILTER_BANK_MAX - CAN2_filterIdx - 1;
		
  /* find free filter */
  switch (ctrl) {
#if RTE_CAN1 == 1
    case __CTRL1:        /* for CTRL1 we check from 0 .. __FILTER_BANK_MAX - CAN2_filterIdx */
      for (filter.FilterNumber = 0; filter.FilterNumber < __FILTER_BANK_MAX; filter.FilterNumber++) 
			{
				filterIdx = CAN_hw_rx_object_chk (filter.FilterNumber, object_para);
        if (filterIdx != 0xff) 
				{
					if (filter.FilterNumber==filter.BankNumber)
						sb = ++filter.BankNumber;
					if ((object_para & FORMAT_TYPE) == STANDARD_TYPE)
					{
						filter.FilterIdHigh = id<<5;
						filter.FilterIdLow = object_para & FRAME_TYPE;
						filter.FilterMaskIdHigh = mask<<5;
						filter.FilterMaskIdLow = object_para & FRAME_TYPE;
					}
					else
					{
						filter.FilterIdHigh = id>>13;
						filter.FilterIdLow = ((id & 0x1FFF)<<3) | (1<<2) | (object_para & FRAME_TYPE);
						filter.FilterMaskIdHigh = mask>>13;
						filter.FilterMaskIdLow = ((mask & 0x1FFF)<<3) | (1<<2) | (object_para & FRAME_TYPE);
					}
					HAL_CAN_ConfigFilter(&hCAN1, &filter);
          return CAN_OK;
        }
				else
				{
					if (filter.FilterNumber>=filter.BankNumber)
						return CAN_OBJECTS_FULL_ERROR;
				}
      }
      break;
#endif
#if RTE_CAN2 == 1
    case __CTRL2:        /* for CTRL2 we check from __FILTER_BANK_MAX .. [1 | CAN1_filterIdx] */
      for (filter.FilterNumber = __FILTER_BANK_MAX; filter.FilterNumber >0; filter.FilterNumber--) 
			{
				filterIdx = CAN_hw_rx_object_chk (filter.FilterNumber, object_para);
        if (filterIdx != 0xff) 
				{
					if (filter.FilterNumber<filter.BankNumber)
						sb = --filter.BankNumber;
					if ((object_para & FORMAT_TYPE) == STANDARD_TYPE)
					{
						filter.FilterIdHigh = id<<5;
						filter.FilterIdLow = object_para & FRAME_TYPE;
						filter.FilterMaskIdHigh = mask<<5;
						filter.FilterMaskIdLow = object_para & FRAME_TYPE;
					}
					else
					{
						filter.FilterIdHigh = id>>13;
						filter.FilterIdLow = ((id & 0x1FFF)<<3) | (1<<2) | (object_para & FRAME_TYPE);
						filter.FilterMaskIdHigh = mask>>13;
						filter.FilterMaskIdLow = ((mask & 0x1FFF)<<3) | (1<<2) | (object_para & FRAME_TYPE);
					}
					HAL_CAN_ConfigFilter(&hCAN2, &filter);
          return CAN_OK;
				}
				else
				{
					if (filter.FilterNumber<filter.BankNumber)
						return CAN_OBJECTS_FULL_ERROR;
				}
      }
      break;
#endif
		default:
			break;
  }
  return CAN_OBJECTS_FULL_ERROR;
}


/*--------------------------- CAN_hw_tx_object ------------------------------
 *
 *  This function setups object that is going to be used for the message 
 *  transmission, the setup of transmission object is not necessery so this 
 *  function is not implemented
 *
 *  Parameter:  ctrl:       Index of the hardware CAN controller (1 .. x)
 *              ch:         Index of object used for transmission
 *              object_para:Object parameters (standard or extended format, 
 *                          data or remote frame)
 *
 *  Return:     CAN_ERROR:  Error code
 *---------------------------------------------------------------------------*/

CAN_ERROR CAN_hw_tx_object (U32 ctrl, U32 ch, U32 object_para)  {

  return CAN_NOT_IMPLEMENTED_ERROR;
}


/************************* Interrupt Functions *******************************/

/*--------------------------- CAN_IRQ_Handler -------------------------------
 *
 *  CAN interrupt function 
 *  If transmit interrupt occured and there are messages in mailbox for 
 *  transmit it writes it to hardware and starts the transmission
 *  If receive interrupt occured it reads message from hardware registers 
 *  and puts it into receive mailbox
 *---------------------------------------------------------------------------*/

#if (RTE_CAN1 == 1)

void CAN1_TX_IRQHandler (void) 
{
	HAL_CAN_IRQHandler(&hCAN1);
}

void CAN1_RX0_IRQHandler (void) 
{
	HAL_CAN_IRQHandler(&hCAN1);
}

void CAN1_SCE_IRQHandler (void ) 
{	
	HAL_CAN_IRQHandler(&hCAN1);
}
#endif

#if (RTE_CAN2 == 1)
void CAN2_TX_IRQHandler (void) 
{
	HAL_CAN_IRQHandler(&hCAN2);
}

void CAN2_RX0_IRQHandler (void) 
{
	HAL_CAN_IRQHandler(&hCAN2);
}

void CAN2_SCE_IRQHandler (void ) 
{	
	HAL_CAN_IRQHandler(&hCAN2);
}

#endif

void HAL_CAN_TxCpltCallback(CAN_HandleTypeDef* hcan)
{
//	CAN_msg *ptrmsg;
//	osEvent event;
//	/* If there is a message in the mailbox ready for send, read the 
//     message from the mailbox and send it			 */
//#if (RTE_CAN1 == 1)
//	if (hcan == &hCAN1)
//	{
//		event = osMailGet(MBX_tx_ctrl[__CTRL1-1], 0);
//    while (event.status == osEventMail) 
//		{
//			ptrmsg = (CAN_msg *)event.value.p;
//      CAN_hw_wr(__CTRL1, ptrmsg);
//			osMailFree(MBX_tx_ctrl[__CTRL1-1], ptrmsg);
//			event = osMailGet(MBX_tx_ctrl[__CTRL1-1], 0);
//    }
////		else
////		{
//		osSemaphoreRelease(wr_sem[__CTRL1-1]);
////			hcan->Instance->IER &= ~CAN_IER_TMEIE;             /* disable  TME interrupt */ 
////		}
//		return;
//	}
//#endif
//#if (RTE_CAN2 == 1)
//	if (hcan == &hCAN2)
//	{
//		event = osMailGet(MBX_tx_ctrl[__CTRL2-1], 0);
//    if (event.status == osEventMail) 
//		{
//			ptrmsg = event.value.p;
//      CAN_hw_wr(__CTRL2, ptrmsg);
//			osMailFree(MBX_tx_ctrl[__CTRL2-1], ptrmsg);
//    }
//		else
//		{
//			osSemaphoreRelease(wr_sem[__CTRL2-1]);
//			hcan->Instance->IER &= ~CAN_IER_TMEIE;             /* disable  TME interrupt */ 
//		}
//	}
//#endif
}

void HAL_CAN_RxCpltCallback(CAN_HandleTypeDef* hcan)
{
	uint8_t i;
	CAN_msg *ptrmsg;
	int ctrl0 = -1;
#if (RTE_CAN1 == 1)
	if (hcan == &hCAN1)
		ctrl0 = __CTRL1-1;
#endif
#if (RTE_CAN2 == 1)
	if (hcan == &hCAN2)
		ctrl0 = __CTRL2-1;
#endif

	ptrmsg = (CAN_msg *)osMailAlloc(MBX_rx_ctrl[ctrl0], 0);
	if (ptrmsg) 
	{
		ptrmsg->len = hcan->pRxMsg->DLC;
		for(i=0;i<ptrmsg->len;++i)
			ptrmsg->data[i] = hcan->pRxMsg->Data[i];
		
		ptrmsg->type = (hcan->pRxMsg->RTR == CAN_RTR_REMOTE) ? REMOTE_FRAME : DATA_FRAME;
		if (hcan->pRxMsg->IDE == CAN_ID_EXT)
		{
			ptrmsg->format = EXTENDED_FORMAT;
			ptrmsg->id = hcan->pRxMsg->ExtId;
		}
		else
		{
			ptrmsg->format = EXTENDED_FORMAT;
			ptrmsg->id = hcan->pRxMsg->ExtId;
		}
		osMailPut (MBX_rx_ctrl[ctrl0], ptrmsg);
	}
	HAL_CAN_Receive_IT(hcan, CAN_FIFO0);
}
void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan)
{
	int ctrl0 = -1;
#if (RTE_CAN1 == 1)
	if (hcan == &hCAN1)
		ctrl0 = __CTRL1-1;
#endif
#if (RTE_CAN2 == 1)
	if (hcan == &hCAN2)
		ctrl0 = __CTRL2-1;
#endif
	
	if (hcan->Instance->TSR & CAN_TSR_TERR0)	//In transmission error
	{
		//Abort current transmission
		hcan->Instance->TSR |= CAN_TSR_ABRQ0;
		while (hcan->Instance->TSR & CAN_TSR_ABRQ0);
		//Clear transmission request
		hcan->Instance->TSR |= CAN_TSR_RQCP0;
		//Release resource
		osSemaphoreRelease(wr_sem[ctrl0]);
		hcan->Instance->IER &= ~CAN_IER_TMEIE;             /* disable  TME interrupt */ 
	}
	else if (hcan->Instance->TSR & CAN_TSR_TERR1)	//In transmission error
	{
		//Abort current transmission
		hcan->Instance->TSR |= CAN_TSR_ABRQ1;
		while (hcan->Instance->TSR & CAN_TSR_ABRQ1);
		//Clear transmission request
		hcan->Instance->TSR |= CAN_TSR_RQCP1;
		//Release resource
		osSemaphoreRelease(wr_sem[ctrl0]);
		hcan->Instance->IER &= ~CAN_IER_TMEIE;             /* disable  TME interrupt */ 
	}
	else if (hcan->Instance->TSR & CAN_TSR_TERR2)	//In transmission error
	{
		//Abort current transmission
		hcan->Instance->TSR |= CAN_TSR_ABRQ2;
		while (hcan->Instance->TSR & CAN_TSR_ABRQ2);
		//Clear transmission request
		hcan->Instance->TSR |= CAN_TSR_RQCP2;
		//Release resource
		osSemaphoreRelease(wr_sem[ctrl0]);
		hcan->Instance->IER &= ~CAN_IER_TMEIE;             /* disable  TME interrupt */ 
	}
	//Clear error status
	hcan->Instance->MSR |= CAN_MSR_ERRI;
}

#if (RTE_CAN1)
static CAN_ERROR CAN1_Initialize (CAN_Speed baudrate) {
  return CAN_init(1, baudrate);
}
static CAN_ERROR CAN1_Start (void) {
  return CAN_hw_start(1);
}
static CAN_ERROR CAN1_Send(CAN_msg *msg, U16 timeout)
{
	msg->type = DATA_FRAME;
  return (CAN_push (1, msg, timeout));
}
static CAN_ERROR CAN1_Request(CAN_msg *msg, U16 timeout)
{
	msg->type = REMOTE_FRAME;
  return (CAN_push (1, msg, timeout));
}

static CAN_ERROR CAN1_Set(CAN_msg *msg, U16 timeout)
{
	return CAN_set(1, msg, timeout);
}

static CAN_ERROR CAN1_Receive (CAN_msg *msg, U16 timeout)  {
	CAN_msg *ptrmsg;
	osEvent event;
	event = osMailGet(MBX_tx_ctrl[__CTRL1-1], 0);
	if (event.status == osEventMail) 
	{
		ptrmsg = (CAN_msg *)event.value.p;
		CAN_hw_wr(__CTRL1, ptrmsg);
		osMailFree(MBX_tx_ctrl[__CTRL1-1], ptrmsg);
	}
	else
		osSemaphoreRelease(wr_sem[__CTRL1-1]);
  return (CAN_pull (1, msg, timeout));
}

static CAN_ERROR CAN1_Rx_object (U32 ch, U32 id, U32 object_para, U32 mask)  {
  return (CAN_hw_rx_object (1, ch, id, object_para, mask));
}

static CAN_ERROR CAN1_Tx_object (U32 ch, U32 object_para)  {
  return (CAN_hw_tx_object (1, ch, object_para));
}

//static CAN_ERROR CAN1_TestMode( U32 testmode)
//{
//	return CAN_hw_testmode(1, testmode);
//}

#endif

#if (RTE_CAN2)
static CAN_ERROR CAN2_Initialize (CAN_Speed baudrate) {
  return CAN_init(2, baudrate);
}
static CAN_ERROR CAN2_Start (void) {
  return CAN_hw_start(2);
}

static CAN_ERROR CAN2_Send(CAN_msg *msg, U16 timeout)
{
	msg->type = DATA_FRAME;
  return (CAN_push (2, msg, timeout));
}

static CAN_ERROR CAN2_Request(CAN_msg *msg, U16 timeout)
{
	msg->type = REMOTE_FRAME;
  return (CAN_push (2, msg, timeout));
}

static CAN_ERROR CAN2_Set(CAN_msg *msg, U16 timeout)
{
	return CAN_set(2, msg, timeout);
}

static CAN_ERROR CAN2_Receive (CAN_msg *msg, U16 timeout)  {
  return (CAN_pull (2, msg, timeout));
}

static CAN_ERROR CAN2_Rx_object (U32 ch, U32 id, U32 object_para)  {
  return (CAN_hw_rx_object (2, ch, id, object_para));
}

static CAN_ERROR CAN2_Tx_object (U32 ch, U32 object_para)  {
  return (CAN_hw_tx_object (2, ch, object_para));
}

//static CAN_ERROR CAN2_TestMode( U32 testmode)
//{
//	return CAN_hw_testmode(2, testmode);
//}

#endif

/* CAN1 Driver Control Block */
#if (RTE_CAN1)
ARM_DRIVER_CAN Driver_CAN1 = {
  CANX_GetVersion,
  CAN1_Initialize,
  CAN1_Start,
	CAN1_Send,
  CAN1_Request,
  CAN1_Set,
  CAN1_Receive,
  CAN1_Rx_object,
  CAN1_Tx_object
	//CAN1_TestMode
};
#endif

/* CAN2 Driver Control Block */
#if (RTE_CAN2)
ARM_DRIVER_CAN Driver_CAN2 = {
  CANX_GetVersion,
  CAN2_Initialize,
  CAN2_Start,
	CAN2_Send,
  CAN2_Request,
  CAN2_Set,
  CAN2_Receive,
  CAN2_Rx_object,
  CAN2_Tx_object
	//CAN2_TestMode
};
#endif

/*----------------------------------------------------------------------------
 * end of file
 *---------------------------------------------------------------------------*/
