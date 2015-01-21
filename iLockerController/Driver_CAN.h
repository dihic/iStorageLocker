/*----------------------------------------------------------------------------
 *      RL-ARM - CAN
 *----------------------------------------------------------------------------
 *      Name:    RTX_CAN.h
 *      Purpose: Header for CAN Generic Layer Driver
 *      Rev.:    V4.70
 *----------------------------------------------------------------------------
 *      This code is part of the RealView Run-Time Library.
 *      Copyright (c) 2004-2013 KEIL - An ARM Company. All rights reserved.
 *---------------------------------------------------------------------------*/

#include <cmsis_os.h>                      /* RTX kernel functions & defines      */
#include <stdint.h>
#include <Driver_Common.h>
#include "RTE_Device.h"
#include "CanConfig.h"

#ifndef __RTX_CAN_H
#define __RTX_CAN_H

#define ARM_CAN_API_VERSION ARM_DRIVER_VERSION_MAJOR_MINOR(1,0)   /* API version */

#ifdef __cplusplus
extern "C"  {
#endif

#define U32 	uint32_t
#define U16 	uint16_t
#define U8 		uint8_t
#define BOOL 	uint8_t
#define __TRUE 1
#define __FALSE 0

/* CAN message object structure                                              */
typedef struct {
  U32 id;                 /* 29 bit identifier                               */
  U8  data[8];            /* Data field                                      */
  U8  len;                /* Length of data field in bytes                   */
  U8  format;             /* 0 - STANDARD,   1 - EXTENDED IDENTIFIER         */
  U8  type;               /* 0 - DATA FRAME, 1 - REMOTE FRAME                */
} CAN_msg;

#define CanSendMailDef(channel)  		osMailQDef(MBX_TX##channel, CAN_No_SendObjects    + 4, CAN_msg)
#define CanReceiveMailDef(channel)  osMailQDef(MBX_RX##channel, CAN_No_ReceiveObjects + 4, CAN_msg)

#if   (RTE_CAN4)
  #define CAN_CTRL_MAX_NUM      4		
#elif (RTE_CAN3)
  #define CAN_CTRL_MAX_NUM      3
#elif (RTE_CAN2)
  #define CAN_CTRL_MAX_NUM      2
#elif (RTE_CAN1)
  #define CAN_CTRL_MAX_NUM      1
#else
  #error "No CAN Controller defined"
#endif

/* Externaly declared mailbox, for CAN transmit messages                     */
extern osMailQId MBX_tx_ctrl[CAN_CTRL_MAX_NUM];//[4 + CAN_No_SendObjects];

/* Externaly declared mailbox, for CAN receive messages                      */
extern osMailQId MBX_rx_ctrl[CAN_CTRL_MAX_NUM];//[4 + CAN_No_ReceiveObjects];

/* Semaphores used for protecting writing to CAN hardware                    */
extern osSemaphoreId wr_sem[CAN_CTRL_MAX_NUM];

/* Symbolic names for formats of CAN message                                 */
typedef enum {STANDARD_FORMAT = 0, EXTENDED_FORMAT} CAN_FORMAT;

/* Symbolic names for type of CAN message                                    */
typedef enum {DATA_FRAME      = 0, REMOTE_FRAME   } CAN_FRAME;

/* Definitions for filter_type                                               */
#define FORMAT_TYPE     (1 << 0)
#define FRAME_TYPE      (1 << 1)

#define STANDARD_TYPE   (0 << 0)
#define EXTENDED_TYPE   (1 << 0)

#define DATA_TYPE       (0 << 1)
#define REMOTE_TYPE     (1 << 1)

#define FILTER_MASK_TYPE (0 << 2)
#define FILTER_LIST_TYPE (1 << 2)

/* Error values that functions can return                                    */
typedef enum   
{ CAN_OK = 0,                       /* No error                              */
  CAN_NOT_IMPLEMENTED_ERROR,        /* Function has not been implemented     */
  CAN_MEM_POOL_INIT_ERROR,          /* Memory pool initialization error      */
  CAN_BAUDRATE_ERROR,               /* Baudrate was not set                  */
  CAN_TX_BUSY_ERROR,                /* Transmitting hardware busy            */
  CAN_OBJECTS_FULL_ERROR,           /* No more rx or tx objects available    */
  CAN_ALLOC_MEM_ERROR,              /* Unable to allocate memory from pool   */
  CAN_DEALLOC_MEM_ERROR,            /* Unable to deallocate memory           */
  CAN_TIMEOUT_ERROR,                /* Timeout expired                       */
  CAN_UNEXIST_CTRL_ERROR,           /* Controller does not exist             */
  CAN_UNEXIST_CH_ERROR,             /* Channel does not exist                */
	CAN_INIT_ERROR
} CAN_ERROR;

typedef enum 
{
	CAN_125Kbps = 125000,
	CAN_250Kbps = 250000,
	CAN_500Kbps = 500000,
	CAN_800Kbps = 800000,
	CAN_1Mbps =	 1000000
} CAN_Speed;

/* Functions defined CAN hardware driver (module CAN_chip.c)                 */
extern CAN_ERROR CAN_hw_testmode   (U32 ctrl, U32 testmode);
extern CAN_ERROR CAN_hw_init       (U32 ctrl, U32 baudrate);
extern CAN_ERROR CAN_hw_start      (U32 ctrl);
extern CAN_ERROR CAN_hw_tx_empty   (U32 ctrl);
extern CAN_ERROR CAN_hw_wr         (U32 ctrl, CAN_msg *msg);
extern CAN_ERROR CAN_hw_set        (U32 ctrl, CAN_msg *msg);
extern CAN_ERROR CAN_hw_rx_object  (U32 ctrl, U32 ch, U32 id, U32 object_para, U32 mask);
extern CAN_ERROR CAN_hw_tx_object  (U32 ctrl, U32 ch,         U32 object_para);

/* Functions defined in module RTX_CAN.c                                     */
CAN_ERROR CAN_push (U32 ctrl, CAN_msg *msg, U16 timeout);
CAN_ERROR CAN_pull (U32 ctrl, CAN_msg *msg, U16 timeout);
CAN_ERROR CAN_init (U32 ctrl, U32 baudrate);
CAN_ERROR CAN_set  (U32 ctrl, CAN_msg *msg, U16 timeout);
//CAN_ERROR CAN_start                (U32 ctrl);
//CAN_ERROR CAN_send                 (U32 ctrl, CAN_msg *msg, U16 timeout);
//CAN_ERROR CAN_request              (U32 ctrl, CAN_msg *msg, U16 timeout);
//CAN_ERROR CAN_receive              (U32 ctrl, CAN_msg *msg, U16 timeout);
//CAN_ERROR CAN_rx_object            (U32 ctrl, U32 ch, U32 id, U32 object_para);
//CAN_ERROR CAN_tx_object            (U32 ctrl, U32 ch,         U32 object_para);

/**
\brief  Access structure of the SPI Driver.
*/
typedef struct _ARM_DRIVER_CAN {
	ARM_DRIVER_VERSION		(*GetVersion)     (void);                              ///< Pointer to \ref ARM_SPI_GetVersion : Get driver version.
	CAN_ERROR 				(*Initialize)		(CAN_Speed baudrate);
	CAN_ERROR 				(*Start)		 		(void);
	CAN_ERROR 				(*Send)      		(CAN_msg *msg, U16 timeout);
	CAN_ERROR 				(*Request)   		(CAN_msg *msg, U16 timeout);
	CAN_ERROR 				(*Set)   				(CAN_msg *msg, U16 timeout);
	CAN_ERROR 				(*Receive)   		(CAN_msg *msg, U16 timeout);
	CAN_ERROR 				(*SetRxObject) 	(U32 ch, U32 id, U32 object_para, U32 mask);
	CAN_ERROR 				(*SetTxObject) 	(U32 ch,         U32 object_para);
	CAN_ERROR 				(*SetTestMode) 	(U32 testmode);
} const ARM_DRIVER_CAN;

#ifdef __cplusplus
}
#endif

#endif /* __RTX_CAN_H */

/*----------------------------------------------------------------------------
 * end of file
 *---------------------------------------------------------------------------*/
