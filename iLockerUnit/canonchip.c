#include "canonchip.h"
#include <string.h>
#include "gpio.h"

#ifndef NULL
#define NULL    ((void *)0)
#endif

#define CAN_EN_PIN PORT2,6
#define HOST_ID 0x001

#define BEGIN_SEND_INDEX 3

#define INDEX_HB 3
#define INDEX_SYNC 4
#define INDEX_NORMAL 5

#ifdef __cplusplus
extern "C" {
#endif

//Reserved RAM for CAN-on-chip
static volatile uint8_t can_memory[0x70] __attribute__((at(0x10000050)))={0xff};

CanRom **rom = (CanRom **)0x1fff1ff8;
CAN_MSG_OBJ msg_objs[33];
CAN_MSG_OBJ filter_objs[2];

volatile uint32_t msgSignals=0;

ReceiverEventHandler CANEXReceiverEvent = NULL;
TriggerSyncEventHandler CANTEXTriggerSyncEvent = NULL;

volatile uint16_t NodeId = 0x0FFF;
uint16_t HeartbeatInterval = 200;
uint16_t SyncInterval = 10;

uint32_t ClkInitTable[2];

uint32_t baudRateBak;

/* Publish CAN Callback Functions */
static CAN_CALLBACKS Callbacks = {
   CAN_rx,
   CAN_tx,
   CAN_error,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
};

void CAN_cfgBaudrate(uint32_t baudRateinK)  
{
	ClkInitTable[0]=(SystemCoreClock / 8000000UL) - 1;
	switch (baudRateinK)
	{
		case 1000:
			ClkInitTable[1]=0x1400;        /* 1MBit/s @ 8MHz CAN clk */
			break;
		case 800:
			ClkInitTable[1]=0x1600;        /* 800kBit/s @ 8MHz CAN clk */
			break;
		case 500:
			ClkInitTable[1]=0x1C00;        /* 500kBit/s @ 8MHz CAN clk */
			break;
		case 250:
			ClkInitTable[1]=0x1C01;        /* 250kBit/s @ 8MHz CAN clk */
			break;
		case 125:
			ClkInitTable[1]=0x1C03;        /* 125kBit/s @ 8MHz CAN clk */
			break;
	}
}

void CAN_IRQHandler(void)
{
  (*rom)->pCAND->isr();
}

uint8_t rxdata[0x100];
CAN_ODENTRY rxEntry;//={0,0,0,rxdata};

void SystemEvent(uint16_t sourceId, CAN_ODENTRY *entry)
{
	if (entry->index==0)
	{
		switch (entry->subindex)
		{
			case 0:
				HeartbeatInterval = (entry->val[1]<<8) | entry->val[0]; 
				break;
			case 1:
				SyncInterval = (entry->val[1]<<8) | entry->val[0];
				break;
		}
	}
}

/*	CAN receive callback */
/*	Function is executed by the Callback handler after
	a CAN message has been received */
void CAN_rx(uint8_t msg_obj_num)
{
	static uint8_t rxIndex = 0;
  uint32_t command;
	uint16_t sourceId;
	
	CAN_MSG_OBJ *msg = msg_objs+msg_obj_num;
	msgSignals |= 1<<msg_obj_num;
  /* Determine which CAN message has been received */
  //msg->msg_obj_num = msg_obj_num;
	
  /* Now load up the msg_obj structure with the CAN message */
  (*rom)->pCAND->can_receive(msg);
	
	command = msg->mode_id & 0x0F000000;
	sourceId = (msg->mode_id>>12) & 0xFFF;
	switch (command)
	{
		case COMMAND_REQUEST:
			if (msg->dlc >= 4)
			{
				rxEntry.index    			= (msg->data[1]<<8) | msg->data[0];
				rxEntry.subindex 			=  msg->data[2];
				rxEntry.entrytype_len =  msg->data[3];
				if (rxEntry.entrytype_len <= 4)
				{
					memcpy(rxEntry.val,msg->data+4,rxEntry.entrytype_len);
					SystemEvent(sourceId,&rxEntry);
					if (CANEXReceiverEvent)
						CANEXReceiverEvent(sourceId,&rxEntry);
				}
				else
				{
					memcpy(rxEntry.val,msg->data+4,4);
					rxIndex = 4;
				}
			}
			break;
		case COMMAND_REQUEST | COMMAND_EXTENDED:
			memcpy(rxEntry.val+rxIndex,msg->data,msg->dlc);
			rxIndex+=msg->dlc;
			if (rxIndex>=rxEntry.entrytype_len)
			{
				SystemEvent(sourceId,&rxEntry);
				if (CANEXReceiverEvent)
					CANEXReceiverEvent(sourceId,&rxEntry);
			}
			break;
		case COMMAND_SYNC:
			if (msg->data[3]==TRIGGER && CANTEXTriggerSyncEvent)
			{
				CANTEXTriggerSyncEvent((msg->data[1]<<8) | msg->data[0],msg->data[2]);
			}
			break;
		case COMMAND_HEARTBEAT:
			break;
	}
	msgSignals &= ~(1<<msg_obj_num);
}

/*	CAN transmit callback */
/*	Function is executed by the Callback handler after
	a CAN message has been transmitted */
void CAN_tx(uint8_t msg_obj_num)
{
	msgSignals &= ~(1<<msg_obj_num);
}

/*	CAN error callback */
/*	Function is executed by the Callback handler after
	an error has occured on the CAN bus */
void CAN_error(uint32_t error_info)
{
	if ((error_info & (CAN_ERROR_STUF | CAN_ERROR_PASS)) == (CAN_ERROR_STUF | CAN_ERROR_PASS))
	{
		NVIC_DisableIRQ(CAN_IRQn);
		CANInit(baudRateBak);	//Force to reset CAN bus
	}
}

void CANChangeNodeId(uint16_t id)
{
	NodeId = id & 0x0FFF;
	
	//Receive NodeId Msg
	filter_objs[0].mode_id = NodeId | CAN_MSGOBJ_EXT;
	filter_objs[0].mask = 0x00000fff;
	filter_objs[0].msg_obj_num=2;
	CANSetFilter(&filter_objs[0]);
}

void CANInit(uint32_t baudRateinK)
{
	int i;
	for(i=0;i<33;++i)
		msg_objs[i].msg_obj_num=i;
	
	msgSignals=0;
	
	baudRateBak=baudRateinK;
	
	//nmtMsg = msg_objs + INDEX_HB;
	
	rxEntry.val=rxdata;
	
	//CAN_STB Enable
	GPIOSetDir(CAN_EN_PIN, E_IO_OUTPUT);
	GPIOSetValue(CAN_EN_PIN, 0);
	
	CAN_cfgBaudrate(baudRateinK);
	(*rom)->pCAND->init_can(ClkInitTable,1);
	(*rom)->pCAND->config_calb(&Callbacks);
	
	
	//Receive NodeId Msg
	filter_objs[0].mode_id = NodeId | CAN_MSGOBJ_EXT;
	filter_objs[0].mask = 0x00000fff;
	filter_objs[0].msg_obj_num = 1;
	CANSetFilter(&filter_objs[0]);
	
	//Receive Broadcast Msg
	filter_objs[1].mode_id = CAN_MSGOBJ_EXT | (HOST_ID<<12);
	filter_objs[1].mask = 0x00ffffff;
	filter_objs[1].msg_obj_num = 2;
	CANSetFilter(&filter_objs[1]);
	
	NVIC_EnableIRQ(CAN_IRQn);
}

void CANSend(CAN_MSG_OBJ *pMsg)
{
	uint32_t sync = 1<<BEGIN_SEND_INDEX;
	uint8_t num = BEGIN_SEND_INDEX;
	uint32_t counter=0;
	
	if ((LPC_CAN->STAT & 0x7)!=0) //Make sure no error on bus to send 
		return;
	
	while (msgSignals & sync)
	{
		//DELAY(100);
		if (++counter<1000)
			continue;
		counter=0;
//		if (pMsg->msg_obj_num == num)
//			msgSignals &= ~sync;
		if (++num >= 32)
			num = BEGIN_SEND_INDEX;
		sync = 1<<num;
//			if (pMsg->msg_obj_num == num)	//all failed
//			{
//				NVIC_DisableIRQ(CAN_IRQn);
//				CANInit(baudRateBak);	//Force to reset CAN bus
//				return;  
//			}
	}
	
	msgSignals |= sync;
	
	//Copy pMsg to msg_obj[num]
	msg_objs[num].mode_id = pMsg->mode_id;
	msg_objs[num].mask = pMsg->mask;
	msg_objs[num].dlc = pMsg->dlc;
	memcpy(msg_objs[num].data, pMsg->data, 8);
	
	//Transmit
	(*rom)->pCAND->can_transmit(&msg_objs[num]);
}

void CANSetFilter(CAN_MSG_OBJ *pMsg)
{
	(*rom)->pCAND->config_rxmsgobj(pMsg);
}


void CANEXHeartbeat(enum SystemState state)
{
	CAN_MSG_OBJ nmtMsg;
	nmtMsg.mode_id = CAN_MSGOBJ_EXT | COMMAND_HEARTBEAT | (NodeId & 0xFFF)<<12;
	nmtMsg.dlc = 1;
	nmtMsg.data[0]=state;
	CANSend(&nmtMsg);
}


void CANEXSync(uint16_t syncId,uint16_t index,enum SyncMode mode)
{
	CAN_MSG_OBJ syncMsg;
	syncId &= 0xfff;
	syncMsg.mode_id = CAN_MSGOBJ_EXT | COMMAND_SYNC | syncId;
	syncMsg.dlc = 4;
	syncMsg.data[0] =  index     & 0xff;
	syncMsg.data[1] = (index>>8) & 0xff;
	syncMsg.data[2] =  0;
	syncMsg.data[3] = mode;
	CANSend(&syncMsg);
}

void CANTransmit(uint32_t command,uint16_t targetId,CAN_ODENTRY *entry,uint16_t sourceId)
{
	uint8_t segment, remainder, i, offset;
	CAN_MSG_OBJ msg;
	
	NodeId   &= 0xfff;
	targetId &= 0xfff;
	
	msg.mode_id= CAN_MSGOBJ_EXT | command | targetId;
	if (sourceId==0xffff)
		msg.mode_id |= NodeId<<12;
	else
		msg.mode_id |= ((sourceId & 0xfff)<<12);
	msg.data[0] =  entry->index     & 0xff;
	msg.data[1] = (entry->index>>8) & 0xff;
	msg.data[2] =  entry->subindex;
	msg.data[3] =  entry->entrytype_len;
	if (entry->entrytype_len<=4)
	{
		msg.dlc = 4 + entry->entrytype_len;
		memcpy(msg.data+4, entry->val, entry->entrytype_len);
		CANSend(&msg);
		return;
	}

	msg.dlc = 8;
	memcpy(msg.data+4, entry->val, 4);
	CANSend(&msg);
	
	segment 	= (entry->entrytype_len-4) >> 3;
	remainder = (entry->entrytype_len-4) & 0x7;
	offset=4;
	if (remainder!=0)
		segment++;
	
	for(i=0; i<segment; ++i)
	{
		msg.mode_id = CAN_MSGOBJ_EXT | command | COMMAND_EXTENDED | NodeId<<12 | targetId;
		msg.dlc = (i<segment-1 || remainder==0) ? 8 : remainder;
		memcpy(msg.data, entry->val+offset, msg.dlc);
		CANSend(&msg);
		offset += msg.dlc;
	}
}

void CANEXRequset(uint16_t serviceId,CAN_ODENTRY *entry)
{
	CANTransmit(COMMAND_REQUEST,serviceId,entry,NodeId);
}

void CANEXSetInterval(uint16_t serviceId,enum IntervalType type,uint16_t interval)
{
	CAN_ODENTRY config;
	config.index					=	0;
	config.subindex				=	type;
	config.entrytype_len	=	2;
	config.val						=	(uint8_t *)&interval;
	CANTransmit(COMMAND_REQUEST, serviceId, &config, NodeId);
}

void CANEXResponse(uint16_t clientId,CAN_ODENTRY *entry)
{
	CANTransmit(COMMAND_RESPONSE,clientId,entry,NodeId);
}

void CANEXBroadcast(CAN_ODENTRY *entry)
{
	CANTransmit(COMMAND_BROADCAST,0,entry,NodeId);
}

#ifdef __cplusplus
}
#endif
