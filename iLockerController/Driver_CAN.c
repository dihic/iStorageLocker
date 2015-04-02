/*----------------------------------------------------------------------------
 *      RL-ARM - CAN
 *----------------------------------------------------------------------------
 *      Name:    RTX_CAN.c
 *      Purpose: CAN Generic Layer Driver
 *      Rev.:    V4.70
 *----------------------------------------------------------------------------
 *      This code is part of the RealView Run-Time Library.
 *      Copyright (c) 2004-2013 KEIL - An ARM Company. All rights reserved.
 *---------------------------------------------------------------------------*/

#include "Driver_CAN.h"                  /* CAN Generic functions & defines     */
#include "stdio.h"

#pragma diag_suppress 550

/* Declare mailbox, for CAN transmit messages                                */
osMailQId MBX_tx_ctrl[CAN_CTRL_MAX_NUM];

/* Declare mailbox, for CAN receive messages                                 */
osMailQId MBX_rx_ctrl[CAN_CTRL_MAX_NUM];

/* Semaphores used for protecting writing to CAN hardware                    */
osSemaphoreId wr_sem[CAN_CTRL_MAX_NUM];

#if (RTE_CAN1 == 1)
	osSemaphoreDef(CAN_SEMA1);
	CanSendMailDef(1);
	CanReceiveMailDef(1);
#endif

#if (RTE_CAN2 == 1)
	osSemaphoreDef(CAN_SEMA2);
	CanSendMailDef(2);
	CanReceiveMailDef(2);
#endif

#if (RTE_CAN3 == 1)
	osSemaphoreDef(CAN_SEMA3);
	CanSendMailDef(3);
	CanReceiveMailDef(3);
#endif

#if (RTE_CAN4 == 1)
	osSemaphoreDef(CAN_SEMA4);
	CanSendMailDef(4);
	CanReceiveMailDef(4);
#endif

//#else
//  #error "No CAN Controller defined"
//#endif


/*----------------------------------------------------------------------------
 *      CAN RTX Generic Driver Functions
 *----------------------------------------------------------------------------
 *  Functions implemented in this module:
 *           CAN_ERROR CAN_mem_init  (void);
 *           CAN_ERROR CAN_setup     (void)
 *           CAN_ERROR CAN_init      (U32 ctrl, U32 baudrate)
 *           CAN_ERROR CAN_start     (U32 ctrl)
 *    static CAN_ERROR CAN_push      (U32 ctrl, CAN_msg *msg, U16 timeout)
 *           CAN_ERROR CAN_send      (U32 ctrl, CAN_msg *msg, U16 timeout)
 *           CAN_ERROR CAN_request   (U32 ctrl, CAN_msg *msg, U16 timeout)
 *           CAN_ERROR CAN_set       (U32 ctrl, CAN_msg *msg, U16 timeout)
 *    static CAN_ERROR CAN_pull      (U32 ctrl, CAN_msg *msg, U16 timeout)
 *           CAN_ERROR CAN_receive   (U32 ctrl, CAN_msg *msg, U16 timeout)
 *           CAN_ERROR CAN_rx_object (U32 ctrl, U32 ch, U32 id, U32 object_para)
 *           CAN_ERROR CAN_tx_object (U32 ctrl, U32 ch,         U32 object_para)
 *---------------------------------------------------------------------------*/


/*--------------------------- CAN_init --------------------------------------
 *
 *  The first time this function is called initialize the memory pool for 
 *  CAN messages and setup CAN controllers hardware
 *
 *  Initialize mailboxes for CAN messages and initialize CAN controller
 *
 *  Parameter:  ctrl:       Index of the hardware CAN controller (1 .. x)
 *              baudrate:   Baudrate
 *
 *  Return:     CAN_ERROR:  Error code
 *---------------------------------------------------------------------------*/

CAN_ERROR CAN_init (U32 ctrl, U32 baudrate)  {
	static U8 first_run_flag[4] = {0, 0, 0, 0};

  //CAN_ERROR error_code;
  U32 ctrl0 = ctrl-1;                 /* Controller index 0 .. x-1           */
	if (first_run_flag[ctrl0]==0)
	{
		first_run_flag[ctrl0]=1;
		switch (ctrl)
		{
	#if CAN_CTRL_MAX_NUM>0
			case 1:
				wr_sem[ctrl0] = osSemaphoreCreate(osSemaphore(CAN_SEMA1), 1);
				MBX_tx_ctrl[ctrl0] = osMailCreate(osMailQ(MBX_TX1), NULL);
				MBX_rx_ctrl[ctrl0] = osMailCreate(osMailQ(MBX_RX1), NULL);
				break;
	#if CAN_CTRL_MAX_NUM>1
			case 2:
				wr_sem[ctrl0] = osSemaphoreCreate(osSemaphore(CAN_SEMA2), 1);
				MBX_tx_ctrl[ctrl0] = osMailCreate(osMailQ(MBX_TX2), NULL);
				MBX_rx_ctrl[ctrl0] = osMailCreate(osMailQ(MBX_RX2), NULL);
				break;
	#if CAN_CTRL_MAX_NUM>2
			case 3:
				wr_sem[ctrl0] = osSemaphoreCreate(osSemaphore(CAN_SEMA3), 1);
				MBX_tx_ctrl[ctrl0] = osMailCreate(osMailQ(MBX_TX3), NULL);
				MBX_rx_ctrl[ctrl0] = osMailCreate(osMailQ(MBX_RX3), NULL);
				break;
	#if CAN_CTRL_MAX_NUM>3
			case 4:
				wr_sem[ctrl0] = osSemaphoreCreate(osSemaphore(CAN_SEMA4), 1);
				MBX_tx_ctrl[ctrl0] = osMailCreate(osMailQ(MBX_TX4), NULL);
				MBX_rx_ctrl[ctrl0] = osMailCreate(osMailQ(MBX_RX4), NULL);
				break;
				#endif
			#endif
		#endif
	#endif
			default:
				break;
		}
	}

//  error_code = _CAN_hw_setup(ctrl);
//  if (error_code != CAN_OK) 
//    return error_code;

  return (CAN_hw_init (ctrl, baudrate));
}


/*--------------------------- CAN_start -------------------------------------
 *
 *  Start CAN controller (enable it to participate on CAN network)
 *
 *  Parameter:  ctrl:       Index of the hardware CAN controller (1 .. x)
 *
 *  Return:     CAN_ERROR:  Error code
 *---------------------------------------------------------------------------*/

//CAN_ERROR CAN_start (U32 ctrl)  {
//  return (CAN_hw_start (ctrl));
//}


/*--------------------------- CAN_push --------------------------------------
 *
 *  Send CAN_msg if hardware is free for sending, otherwise push message to 
 *  message queue to be sent when hardware becomes free
 *
 *  Parameter:  ctrl:       Index of the hardware CAN controller (1 .. x)
 *              msg:        Pointer to CAN message to be sent
 *              timeout:    Timeout value for message sending
 *
 *  Return:     CAN_ERROR:  Error code
 *---------------------------------------------------------------------------*/

CAN_ERROR CAN_push (U32 ctrl, CAN_msg *msg, U16 timeout)  {
  CAN_msg *ptrmsg;
	osEvent event;
  U32 ctrl0 = ctrl-1;                 /* Controller index 0 .. x-1           */

  if (CAN_hw_tx_empty (ctrl) == CAN_OK)  /* Transmit hardware free for send */
	{
    CAN_hw_wr (ctrl, msg);            /* Send message                        */
  }
  else                               /* If hardware for sending is busy     */
	{
    /* Write the message to send mailbox if there is room for it             */
    ptrmsg = (CAN_msg *)osMailAlloc(MBX_tx_ctrl[ctrl0], timeout);
    if (ptrmsg == NULL) 
			return CAN_TIMEOUT_ERROR;
		
    *ptrmsg = *msg;
		osMailPut(MBX_tx_ctrl[ctrl0], ptrmsg);
		
		/* Check once again if transmit hardware is ready for transmission   */
		if (CAN_hw_tx_empty (ctrl) == CAN_OK)  /* Transmit hw free for send */ 
		{
			event = osMailGet (MBX_tx_ctrl[ctrl0], 0);
			if (event.status != osEventMail) 
			{
				osSemaphoreRelease(wr_sem[ctrl0]); /* Return a token back to semaphore  */
				return CAN_OK;              			 /* Message was sent from IRQ already */
			}
			ptrmsg = (CAN_msg *)event.value.p;
			osMailFree (MBX_tx_ctrl[ctrl0], ptrmsg);
			CAN_hw_wr (ctrl, msg);      /* Send message                        */
		}
  }
  return CAN_OK;
}


/*--------------------------- CAN_send --------------------------------------
 *
 *  Send DATA FRAME message, see CAN_push function comment
 *
 *  Parameter:  ctrl:       Index of the hardware CAN controller (1 .. x)
 *              msg:        Pointer to CAN message to be sent
 *              timeout:    Timeout value for message sending
 *
 *  Return:     CAN_ERROR:  Error code
 *---------------------------------------------------------------------------*/

//CAN_ERROR CAN_send (U32 ctrl, CAN_msg *msg, U16 timeout)  {
//  msg->type = DATA_FRAME;

//  return (CAN_push (ctrl, msg, timeout));
//}


/*--------------------------- CAN_request -----------------------------------
 *
 *  Send REMOTE FRAME message, see CAN_push function comment
 *
 *  Parameter:  ctrl:       Index of the hardware CAN controller (1 .. x)
 *              msg:        Pointer to CAN message to be sent
 *              timeout:    Timeout value for message sending
 *
 *  Return:     CAN_ERROR:  Error code
 *---------------------------------------------------------------------------*/

//CAN_ERROR CAN_request (U32 ctrl, CAN_msg *msg, U16 timeout)  {
//  msg->type = REMOTE_FRAME;

//  return (CAN_push (ctrl, msg, timeout));
//}


/*--------------------------- CAN_set ---------------------------------------
 *
 *  Set a message that will automatically be sent as an answer to REMOTE
 *  FRAME message
 *
 *  Parameter:  ctrl:       Index of the hardware CAN controller (1 .. x)
 *              msg:        Pointer to CAN message to be set
 *              timeout:    Timeout value for message to be set
 *
 *  Return:     CAN_ERROR:  Error code
 *---------------------------------------------------------------------------*/

CAN_ERROR CAN_set (U32 ctrl, CAN_msg *msg, U16 timeout)  {
  int32_t i = timeout;
  CAN_ERROR error_code;

  do {
    if (CAN_hw_tx_empty (ctrl) == CAN_OK)  {  /* Transmit hardware free      */
      error_code = CAN_hw_set (ctrl, msg);    /* Set message                 */
      osSemaphoreRelease (wr_sem[ctrl-1]);     /* Return a token back to semaphore  */
      return error_code;
    }
    if (timeout == 0xffff)              /* Indefinite wait                   */
      i++;
    i--;
		osDelay(1);                   /* Wait 1 timer tick                 */
  }  while (i >= 0);

  return CAN_TIMEOUT_ERROR;             /* CAN message not set               */
}


/*--------------------------- CAN_pull --------------------------------------
 *
 *  Pull first received and unread CAN_msg from receiving message queue
 *
 *  Parameter:  ctrl:       Index of the hardware CAN controller (1 .. x)
 *              msg:        Pointer where CAN message will be read
 *              timeout:    Timeout value for message receiving
 *
 *  Return:     CAN_ERROR:  Error code
 *---------------------------------------------------------------------------*/


CAN_ERROR CAN_pull (U32 ctrl, CAN_msg *msg, U16 timeout)  {
  CAN_msg *ptrmsg;
  U32 ctrl0 = ctrl-1;                 /* Controller index 0 .. x-1           */
	//printf("Canbus mail get started!\n");
	osEvent event = osMailGet (MBX_rx_ctrl[ctrl0], timeout);
	
  /* Wait for received message in mailbox                                    */
  if (event.status != osEventMail)
	{
		//printf("Canbus mail fail!\n");
    return CAN_TIMEOUT_ERROR;
	}
	
	//printf("Canbus mail got!\n");
	ptrmsg = (CAN_msg *)event.value.p;

  /* Copy received message from mailbox to address given in function parameter msg */
  *msg = *ptrmsg;

  /* Free box where message was kept  	*/
  if (osMailFree(MBX_rx_ctrl[ctrl0], ptrmsg) != osOK)
    return CAN_DEALLOC_MEM_ERROR;

  return CAN_OK;
}


/*--------------------------- CAN_receive -----------------------------------
 *
 *  Read received message, see CAN_pull function comment
 *
 *  Parameter:  ctrl:       Index of the hardware CAN controller (1 .. x)
 *              msg:        Pointer where CAN message will be read
 *              timeout:    Timeout value for message receiving
 *
 *  Return:     CAN_ERROR:  Error code
 *---------------------------------------------------------------------------*/

//CAN_ERROR CAN_receive (U32 ctrl, CAN_msg *msg, U16 timeout)  {
//  return (CAN_pull (ctrl, msg, timeout));
//}


/*--------------------------- CAN_rx_object ---------------------------------
 *
 *  Enable reception of messages on specified controller and channel with 
 *  specified identifier
 *
 *  Parameter:  ctrl:       Index of the hardware CAN controller (1 .. x)
 *              ch:         Channel for the message transmission
 *              id:         CAN message identifier
 *              object_para:Object parameters (standard or extended format, 
 *                          data or remote frame)
 *
 *  Return:     CAN_ERROR:  Error code
 *---------------------------------------------------------------------------*/

//CAN_ERROR CAN_rx_object (U32 ctrl, U32 ch, U32 id, U32 object_para)  {
//  return (CAN_hw_rx_object (ctrl, ch, id, object_para));
//}


/*--------------------------- CAN_tx_object ---------------------------------
 *
 *  Enable transmission of messages on specified controller and channel with 
 *  specified identifier
 *
 *  Parameter:  ctrl:       Index of the hardware CAN controller (1 .. x)
 *              ch:         Channel for the message transmission
 *              object_para:Object parameters (standard or extended format, 
 *                          data or remote frame)
 *
 *  Return:     CAN_ERROR:  Error code
 *---------------------------------------------------------------------------*/

//CAN_ERROR CAN_tx_object (U32 ctrl, U32 ch, U32 object_para)  {
//  return (CAN_hw_tx_object (ctrl, ch, object_para));
//}

/*----------------------------------------------------------------------------
 * end of file
 *---------------------------------------------------------------------------*/

