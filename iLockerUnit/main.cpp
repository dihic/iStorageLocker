#include <LPC11xx.h>
#include <cstring>
#include <cmath>
#include "gpio.h"
#include "timer32.h"
#include "ssp.h"
#include "trf796x.h"
#include "iso15693.h"
#include "canonchip.h"
#include "CardInfo.h"
#include "delay.h"

using namespace std;

#define FB_LOCKER 	PORT0,2
#define CON_LED 		PORT2,7
#define CON_LOCKER 	PORT2,8

#define IS_DOOR_OPEN 		(GPIOGetValue(FB_LOCKER)==0)
#define IS_DOOR_CLOSED 	(GPIOGetValue(FB_LOCKER)!=0)

#define LED_ON					GPIOSetValue(CON_LED, 1)
#define LED_OFF 				GPIOSetValue(CON_LED, 0)
#define LED_BLINK 			GPIOSetValue(CON_LED, GPIOGetValue(CON_LED)==0)
#define LOCKER_ON 			GPIOSetValue(CON_LOCKER, 0)
#define LOCKER_OFF 			GPIOSetValue(CON_LOCKER, 1)
#define IS_LOCKER_ON 		GPIOGetValue(CON_LOCKER)==0

uint8_t LedState = 0;
bool DoorClosedEvent = false;

#define MAX_ZERO_DRIFT  2
#define ZERO_DRIFT_COEFF 0.2f
#define INTRUSION_EXTENSION_TIME 500   //500ms

#define MEM_BUFSIZE 0x200

#define NVDATA_ADDR 0x10
#define NVINFO_ADDR 0x80

#define NVDATA_BASE 0x30

#define SYNC_SENSOR     0x0101
#define SYNC_DATA				0x0100
#define SYNC_LIVE				0x01ff

#define OP_NOTICE     0x9000
#define OP_LED				0x9001
#define OP_OPEN				0x9002

__align(16) volatile uint8_t MemBuffer[MEM_BUFSIZE];

CardInfo *cardInfo;

uint8_t *pRfidCardType;


uint8_t buf[300];
volatile uint8_t i_reg 							= 0x01;						// interrupt register
volatile uint8_t irq_flag 					= 0x00;
volatile uint8_t rx_error_flag 			= 0x00;
volatile uint8_t rxtx_state 				= 1;							// used for transmit recieve byte count
volatile uint8_t host_control_flag 	= 0;
volatile uint8_t stand_alone_flag 	= 1;
extern uint8_t UID[8];

struct CanResponse
{
	uint16_t sourceId;
	CAN_ODENTRY response;
	uint8_t result;
};

CanResponse res;

static bool syncTriggered = false;
static CAN_ODENTRY syncEntry;

static bool Connected = true;		// host initialize connection, so set true when powerup 
static bool RfidTimeup = true;	// for power saving

static uint8_t LockCount = 0xff;

#define LOCK_WAIT_SECONDS  5

//#define ADDR_ZERO  					0x00
//#define ADDR_RAMP  					0x10
//#define ADDR_UNIT						0x20
//#define ADDR_DEVIATION			0x24
//#define ADDR_CAL_WEIGHT			0x28
//#define ADDR_SENSOR_TYPE		0x2C
//#define ADDR_SENSOR_DISABLE	0x2E
//#define ADDR_CURRENT				0x30
//#define ADDR_Q							0x40
//#define ADDR_Q_BACKUP				0x44
//#define ADDR_Q_PRES					0x48
//#define ADDR_Q_GONE					0x4C
//#define ADDR_WEIGHT_RATIO   0x50
//#define ADDR_MED_GUID				0x60
//#define ADDR_UNIT						0x70
//#define ADDR_DEVIATION			0x74
#define ADDR_PRES_ID_LEN		0x78
#define ADDR_PRES_ID_NAME		0x79

void Setup()
{
	uint8_t id;
 	id = LPC_GPIO[PORT1]->MASKED_ACCESS[0x07];
	id |= (LPC_GPIO[PORT1]->MASKED_ACCESS[0xF0])>>1;
	NodeId = 0x100 | id;
	
	GPIOSetDir(FB_LOCKER, E_IO_INPUT);
	GPIOSetDir(CON_LED, E_IO_OUTPUT);
	GPIOSetDir(CON_LOCKER, E_IO_OUTPUT);
	
	LED_OFF;
	LOCKER_OFF;
	
	memset((void *)MemBuffer,0,MEM_BUFSIZE);
	MemBuffer[0]=0xEE;
	
	pRfidCardType = (uint8_t *)(MemBuffer+1);
	
}


#define RFID_TIME_COUNT    3
#define RFID_TIME_INTERVAL 50

extern "C" {

//volatile uint8_t lastState=0;	//0:intrusion 1:stable
//volatile uint32_t TickCount = 0;
	
void TIMER32_0_IRQHandler()
{
	static uint32_t counter1=0;
	static uint32_t counter2=0;
	bool DoorState = false;
	
	if ( LPC_TMR32B0->IR & 0x01 )
  {    
		LPC_TMR32B0->IR = 1;			/* clear interrupt flag */
		
		if (IS_LOCKER_ON)
		{
			if (LockCount == 0xff)
				LockCount = LOCK_WAIT_SECONDS*2;
			if (IS_DOOR_OPEN)
			{
				LOCKER_OFF;
				LockCount = 0xff;
				DoorState = true;
			}
			
		}
		else if (IS_DOOR_CLOSED && DoorState)
		{
			LedState = 0;
			DoorState = false;
			DoorClosedEvent = true;
		}
		
		if (LedState == 0)
			LED_OFF;
		else if (LedState == 1)
			LED_ON;
		
		if (!RfidTimeup)
		{
			if (counter2++>=RFID_TIME_INTERVAL)
			{
				RfidTimeup =true;
				counter2 = 0;
			}				
		}
		
		if (Connected==false && counter1++>=HeartbeatInterval)
		{
			counter1=0;
			CANEXHeartbeat(STATE_OPERATIONAL);
		}
		
	}
}

void TIMER32_1_IRQHandler()
{
	if ( LPC_TMR32B1->IR & 0x01 )
  {    
		LPC_TMR32B1->IR = 1;			/* clear interrupt flag */
		if (LedState >= 2)
			LED_BLINK;
		if (LockCount!=0xff && IS_LOCKER_ON)
		{
			if (--LockCount == 0)
			{
				LOCKER_OFF;
				LockCount = 0xff;
			}
		}
	}
}

}


void CanexReceived(uint16_t sourceId, CAN_ODENTRY *entry)
{
	CAN_ODENTRY *response=&(res.response);

	res.sourceId = sourceId;
	res.result=0xff;
	
	response->val  = &(res.result);
	response->index = entry->index;
	response->subindex = entry->subindex;
	response->entrytype_len = 1;
	
	switch (entry->index)
	{
		case 0:	//system config
			break;
		case OP_LED:
			if (entry->subindex!=0)
			{
				if (entry->val[0]>2)
					LedState = 0;
				else
					LedState = entry->val[0];
				*(response->val)=0;
			}
			break;
		case OP_OPEN:
			if (entry->subindex!=0)
			{
				if (entry->val[0])
					LOCKER_ON;
				else
					LOCKER_OFF;
				*(response->val)=0;
			}
			break;
		}
	CANEXResponse(sourceId,response);
}

uint8_t syncBuf[0x30];

void CanexSyncTrigger(uint16_t index, uint8_t mode)
{	
	if (mode!=0)
		return;
	if (syncTriggered)
		return;
	
	syncEntry.index=index;
	syncEntry.subindex=0;
	syncEntry.val=syncBuf;

	uint8_t *presId;
	
	switch(index)
	{
		case SYNC_DATA:
			if (Connected)
			{
				syncBuf[0] = *pRfidCardType; 		//Card Type
				memcpy(syncBuf+1, (void *)(MemBuffer+0x20), 8);	  	//Card Id
				if (*pRfidCardType == 2)
				{					
					presId = cardInfo->GetPresId(syncBuf[9]);
					memcpy(syncBuf+10, presId, syncBuf[9]);
					syncBuf[10+syncBuf[9]]=IS_DOOR_OPEN;
					syncEntry.entrytype_len = 11+syncBuf[9];
				}
				else
				{
					syncBuf[9] = IS_DOOR_OPEN;
					syncEntry.entrytype_len = 10;
				}
				syncTriggered = true;
			}
			break;
		case SYNC_LIVE:
			Connected=!Connected;
			break;
	}
}


void UpdateRfid()
{	
	static uint8_t UIDlast[8];
	//static uint8_t found = 0;
	static uint32_t lostCount = 0;
	
	Trf796xTurnRfOn();
	DELAY(2000);
	if (Iso15693FindTag())
	{
		lostCount = 0;
		if (memcmp(UID, UIDlast, 8) != 0)
		{
			memcpy(UIDlast, UID, 8);
			memcpy((void *)(MemBuffer+0x20), UID, 8);
			Iso15693ReadSingleBlockWithAddress(0, UID, cardInfo->GetHeader());
			if (cardInfo->IsValid())
			{
				*pRfidCardType = 0x02;
				memcpy(cardInfo->UID, UID, 8);
				Iso15693ReadMultipleBlockWithAddress(1, 27, UID, cardInfo->GetData());
			}
			else
			{
				*pRfidCardType = 0x01;
				memset(cardInfo->UID, 0, 8);
			}
		}
		if (*pRfidCardType == 0x02 && cardInfo->NeedUpdate())
			Iso15693WriteSingleBlock(1, cardInfo->UID, cardInfo->GetData());
	}
	else
	{
		if (++lostCount>RFID_TIME_COUNT)
		{
			lostCount = RFID_TIME_COUNT;
			if (*pRfidCardType!=0)
			{
				*pRfidCardType = 0x00;
				memset(UIDlast, 0, 8);
				lostCount=100;
			}
		}
	}
	Trf796xTurnRfOff();
	DELAY(2000);
}

int main()
{
	SystemCoreClockUpdate();
	GPIOInit();
	
	cardInfo = new CardInfo; 
	
	Setup();
	
	CANInit(500);
	CANEXReceiverEvent = CanexReceived;
	CANTEXTriggerSyncEvent = CanexSyncTrigger;
	
	
	Trf796xCommunicationSetup();
	Trf796xInitialSettings();
	
	init_timer32(0,TIME_INTERVAL(1000));	//	1000Hz
	init_timer32(1,TIME_INTERVAL(2));		//	2Hz
	DELAY(10);
	enable_timer32(0);
	enable_timer32(1);
	
	//DELAY(1000000); 	//Await 1sec
	
	while(1)
	{
		if (RfidTimeup)
		{
			RfidTimeup = false;
			UpdateRfid();
		}
		
		if (syncTriggered)
		{
			syncTriggered = false;
			CANEXBroadcast(&syncEntry);
		}
	}
}
