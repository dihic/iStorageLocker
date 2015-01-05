#include "iso15693.h"
#include <string.h>

//===============================================================

uint8_t afi = 0;
uint8_t flags = 0;							// stores the mask value (used in anticollision)



extern uint8_t buf[300];
extern volatile uint8_t i_reg;
extern volatile uint8_t irq_flag;
extern volatile uint8_t rxtx_state;
extern volatile uint8_t rx_error_flag;
extern volatile uint8_t stand_alone_flag;
extern volatile uint8_t host_control_flag;

uint8_t viccFound=0;
uint8_t UID[8];


//===============================================================
// NAME: void Iso15693FindTag(void)
//
// BRIEF: Is used to detect ISO15693 conform tags in stand alone 
// mode.
//
// INPUTS:
//	
// OUTPUTS:
//
// PROCESS:	[1] turn on RF driver
//			[2] do a complete anticollision sequence
//			[3] turn off RF driver
//
// NOTE: If ISO15693 conform Tag is detected, ISO15693 LED will
//       be turned on.
//
// CHANGE:
// DATE  		WHO	DETAIL
// 23Nov2010	RP	Original Code
//===============================================================

extern uint8_t registers[32];


uint8_t Iso15693FindTag(void)
{
	//Trf796xTurnRfOn();
	Trf796xWriteIsoControl(0x02);

	// The VCD should wait at least 1 ms after it activated the
	// powering field before sending the first request, to
	// ensure that the VICCs are ready to receive it. (ISO15693-3)
	DELAY(1000);
	
//	registers[0]=0;
//	Trf796xReadCont(registers,32);

	//flags = SIXTEEN_SLOTS;
	flags=ONE_SLOT;
	
	buf[20] = 0x00;
	Iso15693Anticollision(&buf[20], 0);					// send Inventory request
	
	//Trf796xTurnRfOff();
	Trf796xResetIrqStatus();                         
	// clear any IRQs
	return viccFound;
}

//===============================================================
// NAME: void Iso15693Anticollision(uint8_t *mask, uint8_t length)
//
// BRIEF: Is used to perform a inventory cycle of 1 or 16
// timeslots.
//
// INPUTS:
//	Parameters:
//		uint8_t		*mask		mask value
//		uint8_t		length		number of significant bits of
//								mask value
//	
// OUTPUTS:
//
// PROCESS:	[1] send command
//			[2] receive respond
//			[3] send respond to host
//
// CHANGE:
// DATE  		WHO	DETAIL
// 23Nov2010	RP	Original Code
//  3May2011    AF  Bugfix
//===============================================================

void Iso15693Anticollision(uint8_t *mask, uint8_t length)		// host command 0x14
{
	uint8_t	i = 1, j = 1, command[2], no_slots, found = 0;
	
	uint8_t	*p_slot_no, slot_no[17];
	uint8_t	new_mask[8], new_length, mask_size;
	uint32_t	size;
        
	uint8_t	fifo_length = 0;	
	uint16_t k=0;

	memset(slot_no,0,17);
	
	// BUGFIX
	TRF796xCheckRXWaitTime();		
	
	if((flags & BIT5) == 0x00)							// flag bit5 is the number of slots indicator	
	{	
		no_slots = 16;									// 16 slots if bit is cleared
		Trf796xEnableSlotCounter();
	}
	else
		no_slots = 1;									// 1 slot if bit is set

	p_slot_no = slot_no;							// slot number pointer

	mask_size = (((length >> 2) + 1) >> 1);				// mask_size is 1 for length = 4 or 8

	buf[0] = 0x8F;
	buf[1] = 0x91;										// send with CRC
	buf[2] = 0x3D;										// write continuous from 1D
	buf[5] = flags;										// ISO15693 flags
	buf[6] = 0x01;										// anticollision command code

	//optional afi should be here
	if(flags & 0x10)
	{
		// mask_size is 2 for length = 12 or 16 ;
		// and so on

		size = mask_size + 4;							// mask value + mask length + afi + command code + flags
          
		buf[7] = afi;
		buf[8] = length;								// masklength
		if(length > 0)
//		{
			memcpy(buf+9,mask,mask_size);
//			for(i = 0; i < mask_size; i++) 
//            {	
//            	buf[9 + i] = *(mask + i);
//            }
//		}									
		fifo_length = 9;
	}
	else
	{ 
		// mask_size is 2 for length = 12 or 16
		// and so on

		size = mask_size + 3;							// mask value + mask length + command code + flags
          
		buf[7] = length;								// masklength
		if(length > 0)
//		{	
			memcpy(buf+8,mask,mask_size);
//			for(i = 0; i < mask_size; i++)
//			{	
//				buf[8 + i] = *(mask + i);
//			}
//		}									
		fifo_length = 8;
	}

	buf[3] = (uint8_t) (size >> 8);
	buf[4] = (uint8_t) (size << 4);
          
	Trf796xResetIrqStatus();
	IRQ_CLR;											// PORT2 interrupt flag clear
	IRQ_ON;

	Trf796xRawWrite(&buf[0], mask_size + fifo_length);	// writing to FIFO
	
	i_reg = 0x01;
	irq_flag = 0x00;
	START_TIMER(20);										//	start timer up mode
	while(irq_flag == 0x00)
	{
	}													// wait for end of TX interrupt

	for(j = 1; j <= no_slots; j++)						// 1 or 16 available timeslots
	{	
		rxtx_state = 1;									// prepare the extern counter
		// the first UID will be stored from buf[1] upwards
		irq_flag = 0x00;
		START_TIMER(30);
		while(irq_flag == 0x00)
		{
		}												// wait for interrupt
	
		while(i_reg == 0x01)							// wait for RX complete
		{
			k++;
			if(k == 0xFFF0)
			{
				i_reg = 0x00;
				rx_error_flag = 0x00;
			}
		}

		command[0] = RSSI_LEVELS;						// read RSSI levels
		Trf796xReadSingle(command, 1);
		switch(i_reg)
		{
			case 0xFF:									// if recieved UID in buffer
				memcpy(UID,buf+3,8);
				found = 1;
//				if(stand_alone_flag != 1)
//				{	
//					#ifdef ENABLE_HOST
//						UartPutCrlf();
//						UartPutChar('[');
//						for(i = 3; i < 11; i++)
//						{	
//							UartPutByte(buf[i]);		// send UID to host
//						}
//						UartPutChar(',');
//						UartPutByte(command[0]);		// RSSI levels
//						UartPutChar(']');
//						UartPutCrlf();
//					#endif
//				}
				break;
				
			case 0x02:									// collision occured	
//				if(stand_alone_flag == 0)
//				{	
//					#ifdef ENABLE_HOST
//						UartPutChar('[');
//						UartPutChar('z');
//						UartPutChar(',');
//						UartPutByte(command[0]);		// RSSI levels
//						UartPutChar(']');
//					#endif
//				}
	
				if (no_slots == 16)
				{
					p_slot_no++;							// remember a collision was detected
					*p_slot_no = j;
				}
				else
				{
					irq_flag = 0x00;
					rx_error_flag = 0x00;
					rxtx_state = 1;							// used for transmit recieve byte count
					host_control_flag = 0;
					stand_alone_flag = 1;
					Trf796xInitialSettings();
					DELAY(1000);
				}
				break;
				
			case 0x00:									// timer interrupt
//				if(stand_alone_flag == 0)
//				{	
//					#ifdef ENABLE_HOST
//						UartPutChar('[');				// send no-response massage to host
//						UartPutChar(',');
//						UartPutByte(command[0]);		// RSSI levels
//						UartPutChar(']');
//					#endif
//				}
				break;
			
			default:
				break;
		}
		
		Trf796xReset();									// FIFO has to be reset before recieving the next response
		if (no_slots == 16)
		{
			Trf796xStopDecoders();
			if (j<16)											// if 16 slots used send EOF(next slot)
			{
				Trf796xRunDecoders();
				Trf796xTransmitNextSlot();
			}
			else 	// at the end of slot 16 stop the slot counter
				Trf796xDisableSlotCounter();
		}
		else		// 1 slot is used
		{
			Trf796xStopDecoders();
			break;
		}

		if(stand_alone_flag == 0)
		{	
			#ifdef ENABLE_HOST
				//UartPutCrlf();
			#endif
		}
	}													// for

	if(host_control_flag == 0)
	{
		if(found == 1)									// LED on?
		{	
			//LED_15693_ON;								// LEDs indicate detected ISO15693 tag		
		}
		else
		{	
//			LED_15693_OFF;
//			LED_POWER_ON;		
		}
	}

	if (no_slots == 16)
	{
		new_length = length + 4; 							// the mask length is a multiple of 4 bits
		mask_size = (((new_length >> 2) + 1) >> 1);
		
		while((*p_slot_no != 0x00) && (new_length < 61) && (slot_no[16] != 16))
		{	
			*p_slot_no = *p_slot_no - 1;

			for(i = 0; i < 8; i++)
			{	
				new_mask[i] = *(mask + i);				//first the whole mask is copied
			}
			
			if((new_length & BIT2) == 0x00)
			{	
				*p_slot_no = *p_slot_no << 4;
			}
			else
			{
				for(i = 7; i > 0; i--)
				{
					new_mask[i] = new_mask[i - 1];
				}
				new_mask[0] &= 0x00;
			}
			new_mask[0] |= *p_slot_no;				// the mask is changed
			DELAY(10000);

			Iso15693Anticollision(&new_mask[0], new_length);	// recursive call with new Mask

			p_slot_no--;
		}
	}
	
	IRQ_CLR;
	IRQ_OFF;
	viccFound=found;
	if (!found)
		memset(UID,0,8);
}														// Iso15693Anticollision

uint8_t Iso15693Select(uint8_t *uid,uint8_t selected)
{
	int k=0;
	buf[0] = 0x8F;
	buf[1] = 0x91;										// send with CRC
	buf[2] = 0x3D;										// write continuous from 1D
	buf[3] = 0x00;
	buf[4] = 0xA0;
	buf[5] = 0x22;
	buf[6] = selected ? 0x25:0x26;
	memcpy(&buf[7],uid,8);
	
	Trf796xResetIrqStatus();
	IRQ_CLR;											// PORT2 interrupt flag clear
	IRQ_ON;
	
	Trf796xRawWrite(buf, 15);	// writing to FIFO
	irq_flag = 0x00;
	START_TIMER(20);										//	start timer up mode
	while(irq_flag == 0x00)
	{
	}
	irq_flag = 0x00;
	START_TIMER(20);										//	start timer up mode
	while(irq_flag == 0x00)
	{
	}

	rxtx_state = 100;
	irq_flag = 0x00;
	START_TIMER(30);
	while(irq_flag == 0x00)
	{
	}										
	while(i_reg == 0x01)							// wait for RX complete
	{
		k++;
		if(k == 0xFFF0)
		{
			i_reg = 0x00;
			rx_error_flag = 0x01;
		}
	}
	
	IRQ_OFF;
	Trf796xResetIrqStatus();    
	if (i_reg==0xff)
		return buf[100]==0?0:buf[101];
	return 0xff;
}

uint8_t Iso15693ReadSingleBlockWithAddress(uint8_t block,uint8_t *uid,uint8_t *data)
{
	int k=0;
//	if (!viccFound)
//		return 0xff;
	buf[0] = 0x8F;
	buf[1] = 0x91;										// send with CRC
	buf[2] = 0x3D;										// write continuous from 1D
	buf[3] = 0x00;
	buf[4] = 0xB0;
	buf[5] = 0x22;									// ISO15693 flags
	buf[6] = 0x20;
	memcpy(buf+7,uid,8);
	buf[15]= block;
	
	Trf796xResetIrqStatus();
	IRQ_CLR;											// PORT2 interrupt flag clear
	IRQ_ON;
	
	Trf796xRawWrite(buf, 16);	// writing to FIFO
	irq_flag = 0x00;
	START_TIMER(20);										//	start timer up mode
	while(irq_flag == 0x00)
	{
	}
	
	irq_flag = 0x00;
	START_TIMER(10);										//	start timer up mode
	while(irq_flag == 0x00)
	{
	}
	
	rxtx_state = 0;
	irq_flag = 0x00;
	START_TIMER(30);
	while(irq_flag == 0x00)
	{
	}										
	while(i_reg == 0x01)							// wait for RX complete
	{
		k++;
		if(k == 0xFFF0)
		{
			i_reg = 0x00;
			rx_error_flag = 0x01;
		}
	}
	
	IRQ_OFF;
	Trf796xResetIrqStatus();    
	if (i_reg==0xff)
	{
		if (buf[0]==0x00)
		{
			memcpy(data,buf+1,4);
			return 0;
		}
		else
			return buf[1];
	}
	return 0xff;
}

uint8_t Iso15693ReadSingleBlock(uint8_t block,uint8_t needSelected,uint8_t *data)
{
	int k=0;
//	if (!viccFound)
//		return 0xff;
	
	buf[0] = 0x8F;
	buf[1] = 0x91;										// send with CRC
	buf[2] = 0x3D;										// write continuous from 1D
	buf[3] = 0x00;
	buf[4] = 0x30;
	buf[5] = needSelected ? 0x12:0x02;									// ISO15693 flags
	buf[6] = 0x20;
	buf[7] = block;
	
	Trf796xResetIrqStatus();
	IRQ_CLR;											// PORT2 interrupt flag clear
	IRQ_ON;
	
	Trf796xRawWrite(buf, 8);	// writing to FIFO
	irq_flag = 0x00;
	START_TIMER(20);										//	start timer up mode
	while(irq_flag == 0x00)
	{
	}

	rxtx_state = 0;
	irq_flag = 0x00;
	START_TIMER(50);
	while(irq_flag == 0x00)
	{
	}										
	while(i_reg == 0x01)							// wait for RX complete
	{
		k++;
		if(k == 0xFFF0)
		{
			i_reg = 0x00;
			rx_error_flag = 0x01;
		}
	}
	
	IRQ_OFF;
	Trf796xResetIrqStatus();    
	if (i_reg==0xff)
	{
		if (buf[0]==0x00)
		{
			memcpy(data,buf+1,4);
			return 0;
		}
		else
			return buf[1];
	}
	return 0xff;
}

uint8_t Iso15693ReadMultipleBlockWithAddress(uint8_t block,uint8_t numbers,uint8_t *uid,uint8_t *data)
{
	int k=0;
//	if (!viccFound)
//		return 0xff;
	buf[0] = 0x8F;
	buf[1] = 0x91;										// send with CRC
	buf[2] = 0x3D;										// write continuous from 1D
	buf[3] = 0x00;
	buf[4] = 0xC0;
	buf[5] = 0x22;									// ISO15693 flags
	buf[6] = 0x23;
	memcpy(buf+7,uid,8);
	buf[15]= block;
	buf[16]= numbers-1;
	
	Trf796xResetIrqStatus();
	IRQ_CLR;											// PORT2 interrupt flag clear
	IRQ_ON;
	
	Trf796xRawWrite(buf, 17);	// writing to FIFO
	irq_flag = 0x00;
	START_TIMER(20);										//	start timer up mode
	while(irq_flag == 0x00)
	{
	}
	
	irq_flag = 0x00;
	START_TIMER(10);										//	start timer up mode
	while(irq_flag == 0x00)
	{
	}
	
	rxtx_state = 0;
	irq_flag = 0x00;
	START_TIMER(30);
	while(irq_flag == 0x00)
	{
	}										
	while(i_reg == 0x01)							// wait for RX complete
	{
		k++;
		if(k == 0xFFF0)
		{
			i_reg = 0x00;
			rx_error_flag = 0x01;
		}
	}
	
	IRQ_OFF;
	Trf796xResetIrqStatus();    
	if (i_reg==0xff)
	{
		if (buf[0]==0x00)
		{
			memcpy(data,buf+1,4*numbers);
			return 0;
		}
		else
			return buf[1];
	}
	return 0xff;
}


uint8_t Iso15693WriteSingleBlock(uint8_t block,uint8_t *uid,uint8_t *data)
{
	int k=0;
//	if (!viccFound)
//		return 0xff;
	//Trf796xTurnRfOn();
	buf[0] = 0x8F;
	buf[1] = 0x91;										// send with CRC
	buf[2] = 0x3D;										// write continuous from 1D
	buf[3] = 0x00;
	buf[4] = 0xF0;
	buf[5] = 0x62;										// ISO15693 flags
	buf[6] = 0x21;								
	memcpy(buf+7,uid,8);
	buf[15] = block;
	buf[16] = data[0];
	memcpy(buf+16,data,4);
	
	Trf796xResetIrqStatus();
	IRQ_CLR;										
	IRQ_ON;
	Trf796xRawWrite(buf, 17);	// writing to FIFO
	
	irq_flag = 0x00;
	START_TIMER(20);										//	start timer up mode
	while(irq_flag == 0x00)
	{
	}													// wait for end of TX interrupt
	
	
	buf[0]=0x3F;
	buf[1]=data[1];
	buf[2]=data[2];
	buf[3]=data[3];

	Trf796xRawWrite(buf, 4);	// writing to FIFO
	irq_flag = 0x00;
	START_TIMER(20);										//	start timer up mode
	while(irq_flag == 0x00)
	{
	}	

	rxtx_state = 0;
	irq_flag = 0x00;
	START_TIMER(30);
	while(irq_flag == 0x00)
	{
	}												// wait for interrupt

	while(i_reg == 0x01)							// wait for RX complete
	{
		k++;
		if(k == 0xFFF0)
		{
			i_reg = 0x00;
			rx_error_flag = 0x01;
		}
	}
	
	IRQ_OFF;
	Trf796xResetIrqStatus();   
	if (i_reg==0xff)
		return buf[100]==0?0:buf[101];
	return 0xff;
}

//===============================================================
// NAME: void TRF796xCheckRXWaitTime()
//
// BRIEF: a bug writes wrong value in register 0x07 "RX no response wait"
//        this causes ISO15693 Anticollision to fail
//
// INPUTS:
//	
// OUTPUTS:
//
// PROCESS:	[1] read ISO register to check if low or high data rate
//			[2] set correct value in register 0x07 "RX no response wait"
//			
//
// CHANGE:
// DATE  		WHO	DETAIL
// 3May2011	Andre Frantzke	Bugfix
//===============================================================

void TRF796xCheckRXWaitTime(void)
{
	uint8_t MyRegister[2],write[2];
	
	// [1] read ISO register to check if low or high data rate
	MyRegister[0] = ISO_CONTROL;
	MyRegister[1] = ISO_CONTROL;				
	Trf796xReadSingle (&MyRegister[1], 1);
	
	write[0] = RX_NO_RESPONSE_WAIT_TIME;
	//[2] set correct value in register 0x07 "RX no response wait"
	if((MyRegister[1] < 0x02) || (MyRegister[1] == 0x04) || (MyRegister[1] == 0x05)) // low data rate
		write[1] = 0x30;				// 1812 us
	else
		write[1] = 0x14;				// 755 us
	Trf796xWriteSingle(write, 2);
}


/*
	uint8_t MyRegisters[30], MyTemp[2];
	MyRegisters[0] = CHIP_STATE_CONTROL;
	MyRegisters[1] = CHIP_STATE_CONTROL;				// next slot counter
	Trf796xReadSingle (&MyRegisters[1], 1);
	
	MyRegisters[2] = ISO_CONTROL;
	MyRegisters[3] = ISO_CONTROL;				// next slot counter
	Trf796xReadSingle (&MyRegisters[3], 1);
	
	MyRegisters[4] = ISO_14443B_OPTIONS;
	MyRegisters[5] = ISO_14443B_OPTIONS;				// next slot counter
	Trf796xReadSingle (&MyRegisters[5], 1);
	
	MyRegisters[6] = ISO_14443A_OPTIONS;
	MyRegisters[7] = ISO_14443A_OPTIONS;				// next slot counter
	Trf796xReadSingle (&MyRegisters[7], 1);
	
	MyRegisters[8] = TX_TIMER_EPC_HIGH;
	MyRegisters[9] = TX_TIMER_EPC_HIGH;				// next slot counter
	Trf796xReadSingle (&MyRegisters[6], 1);
	
	MyRegisters[10] = TX_TIMER_EPC_LOW;
	MyRegisters[11] = TX_TIMER_EPC_LOW;				// next slot counter
	Trf796xReadSingle (&MyRegisters[11], 1);
	
	MyRegisters[12] = TX_PULSE_LENGTH_CONTROL;
	MyRegisters[13] = TX_PULSE_LENGTH_CONTROL;				// next slot counter
	Trf796xReadSingle (&MyRegisters[13], 1);
	
	MyRegisters[14] = RX_NO_RESPONSE_WAIT_TIME;
	MyRegisters[15] = RX_NO_RESPONSE_WAIT_TIME;				// next slot counter
	Trf796xReadSingle (&MyRegisters[15], 1);
	
	MyRegisters[16] = RX_WAIT_TIME;
	MyRegisters[17] = RX_WAIT_TIME;				// next slot counter
	Trf796xReadSingle (&MyRegisters[17], 1);
	
	MyRegisters[18] = MODULATOR_CONTROL;
	MyRegisters[19] = MODULATOR_CONTROL;				// next slot counter
	Trf796xReadSingle (&MyRegisters[19], 1);
	
	MyRegisters[20] = RX_SPECIAL_SETTINGS;
	MyRegisters[21] = RX_SPECIAL_SETTINGS;				// next slot counter
	Trf796xReadSingle (&MyRegisters[21], 1);
	
	MyRegisters[22] = REGULATOR_CONTROL;
	MyRegisters[23] = REGULATOR_CONTROL;				// next slot counter
	Trf796xReadSingle (&MyRegisters[23], 1);
			
	MyRegisters[24] = IRQ_STATUS;
	MyRegisters[25] = IRQ_STATUS;				// next slot counter
	Trf796xReadSingle (&MyRegisters[25], 1);
	
	MyRegisters[26] = IRQ_MASK;
	MyRegisters[27] = IRQ_MASK;				// next slot counter
	Trf796xReadSingle (&MyRegisters[27], 1);
	
	MyRegisters[28] = COLLISION_POSITION;
	MyRegisters[29] = COLLISION_POSITION;				// next slot counter
	Trf796xReadSingle (&MyRegisters[29], 1);
	*/
