	/****************************************************************
* FILENAME: trf796x.c
*
* BRIEF: Contains functions to initialize and execute
* communication the TRF796x via the selected interface.
*
* Copyright (C) 2010 Texas Instruments, Inc.
*
* AUTHOR(S): Reiser Peter		DATE: 02 DEC 2010
*
* EDITED BY:
* *
*
****************************************************************/
#include "trf796x.h"
#include "ssp.h"

//===============================================================

uint8_t sspPort=1;

uint8_t	command[2];
uint8_t	direct_mode = 0;
extern uint8_t buf[300];
extern volatile uint8_t	i_reg;
#ifdef ENABLE14443A
	extern volatile uint8_t coll_poss;
#endif
extern volatile uint8_t	irq_flag;
extern volatile uint8_t	rx_error_flag;
extern volatile int8_t	rxtx_state;
extern volatile uint8_t	stand_alone_flag;

//===============================================================

#ifdef __cplusplus
extern "C" {
#endif


void SysTick_Handler()
{
	uint8_t irq_status[4];
	STOP_TIMER;
	irq_flag = 0x02;
	Trf796xReadIrqStatus(irq_status);
	*irq_status = *irq_status & 0xF7;				// set the parity flag to 0

	if(*irq_status == 0x00 || *irq_status == 0x80)
		i_reg = 0x00;								// timer interrupt
	else
		i_reg = 0x01;
}
	
void Trf796xISR(uint8_t *irq_status);
	
void PIOINT3_IRQHandler(void)
{
	uint8_t irq_status[4];	
	irq_flag = 0x02;
	STOP_TIMER;							// stop timer mode
	
	do
	{
		IRQ_CLR;							// PORT2 interrupt flag clear
		
		Trf796xReadIrqStatus(irq_status);

		// IRQ status register has to be read
		if(*irq_status == 0xA0)				// TX active and only 3 bytes left in FIFO
		{
			break;
		}
		Trf796xISR(&irq_status[0]);
	} while(GPIOIntStatus(PORT3,0));
}
#ifdef __cplusplus
}
#endif

//===============================================================
//                                                              ;
//===============================================================

void 
Trf796xCommunicationSetup()
{
	sspPort=RFID_SSP;
	
	//IRQ_ON;
	GPIOSetDir(PIN_RFID_IRQ,E_IO_INPUT);
	GPIOSetInterrupt(PIN_RFID_IRQ,0,0,1);
	
	//RFID_CS
	GPIOSetDir(PIN_RFID_CS, E_IO_OUTPUT);
	GPIOSetValue(PIN_RFID_CS, 1);
	
	//RFID_EN
	GPIOSetDir(PIN_RFID_EN, E_IO_OUTPUT);
	GPIOSetValue(PIN_RFID_EN, 0);
	
	NVIC_EnableIRQ(RFID_IRQn);
	
	SSP_IOConfig(RFID_SSP);
	SSP_Init(RFID_SSP,0,PIN_RFID_CS);
	
	
	GPIOSetValue(PIN_RFID_EN,1); //RFID_EN
	DELAY(1000);
}

//===============================================================
// NAME: void Trf796xDirectCommand (uint8_t *pbuf)
//
// BRIEF: Is used to transmit a Direct Command to TRF796x.
//
// INPUTS:
//	Parameters:
//		uint8_t		*pbuf		Direct Command
//
// OUTPUTS:
//
// PROCESS:	[1] transmit Direct Command
//
// CHANGE:
// DATE  		WHO	DETAIL
// 24Nov2010	RP	Original Code
//===============================================================

void 
Trf796xDirectCommand(uint8_t *pbuf,int needDummy)
{
	pbuf[0]|=PREFIX_COMMAND;
	pbuf[1]=0xff;
	SSP_BeginTransaction(sspPort);
	SSP_PhaseChange(sspPort,0);
	if (needDummy)
		SSP_Write(sspPort,pbuf,2);
	else
		SSP_Write(sspPort,pbuf,1);
	SSP_EndTransaction(sspPort);
	
//	SSP_Write(sspPort,pbuf,1);
//	if (needDummy)
//	{
//		DELAY(5);
//		SSP_CLK_Force(sspPort,1);
//		SSP_EndTransaction(sspPort);
//		SSP_CLK_Revert(sspPort);
//	}
//	else
//	{
//		SSP_EndTransaction(sspPort);
//	}
}

//===============================================================
// NAME: void Trf796xDirectMode (void)
//
// BRIEF: Is used to start Direct Mode.
//
// INPUTS:
//
// OUTPUTS:
//
// PROCESS:	[1] start Direct Mode
//
// NOTE: No stop condition
//
// CHANGE:
// DATE  		WHO	DETAIL
// 24Nov2010	RP	Original Code
//===============================================================

//void
//Trf796xDirectMode(void)
//{
//	direct_mode = 1;
//	
//	if(SPIMODE)								// SPI mode given by jumper setting
//	{
//		SpiDirectMode();
//	}
//	else									// parallel mode
//	{
//		ParallelDirectMode();	
//	}
//}

//===============================================================
// 02DEC2010	RP	Original Code
//===============================================================

void 
Trf796xDisableSlotCounter(void)
{
	buf[40] = IRQ_MASK;
	buf[41] = IRQ_MASK;				// next slot counter
	Trf796xReadSingle(&buf[41], 1);
	buf[41] &= 0xfe;				// clear BIT0 in register 0x01
	Trf796xWriteSingle(&buf[40], 2);
}

//===============================================================
// 02DEC2010	RP	Original Code
//===============================================================

void
Trf796xEnableSlotCounter(void)
{
	buf[40] = IRQ_MASK;
	buf[41] = IRQ_MASK;				// next slot counter
	Trf796xReadSingle(&buf[41], 1);
	buf[41] |= BIT0;				// set BIT0 in register 0x01
	Trf796xWriteSingle(&buf[40], 2);
}

//===============================================================
// 02DEC2010	RP	Original Code
//===============================================================

void
Trf796xInitialSettings(void)
{	
	uint8_t control[6];
	control[0]=CHIP_STATE_CONTROL;
	control[1]=0x00; //RF off, ISO, Full power, main RX input, 3.3V operation
	control[2] = MODULATOR_CONTROL;
	control[3] = 0x21;						// 6.78MHz, OOK 100%
	control[4]=REGULATOR_CONTROL;
	control[5]= 0x03; //manual regulator 3.0V
	
	Trf796xWriteSingle(control, 6);
}

//===============================================================
// The Interrupt Service Routine determines how the IRQ should  ;
// be handled. The TRF796x IRQ status register is read to       ;
// determine the cause of the IRQ. Conditions are checked and   ;
// appropriate actions taken.                                   ;
//===============================================================

void
Trf796xISR(uint8_t *irq_status)
{	
#ifdef ENABLE14443A
	uint8_t	len;	
#endif

	if(*irq_status == 0xA0)			// BIT5 and BIT7
	{								// TX active and only 3 bytes left in FIFO
		i_reg = 0x00;
	}
	else if(*irq_status == BIT7)
	{								// TX complete
		i_reg = 0x00;
		Trf796xReset();				// reset the FIFO after TX
	}
	else if((*irq_status & BIT1) == BIT1)
	{								// collision error
		i_reg = 0x02;				// RX complete

#ifdef ENABLE14443A
		coll_poss = COLLISION_POSITION;
		Trf796xReadSingle((uint8_t *)&coll_poss, 1);

		len = coll_poss - 0x20;		// number of valid bytes if FIFO

		if(stand_alone_flag == 0)
		{
//			#ifdef ENABLE_HOST
//				UartPutChar('{');
//				UartPutByte(coll_poss);
//				UartPutChar('}');
//			#endif
		}

		if((len & 0x0f) != 0x00) len = len + 0x10;	// add 1 byte if broken byte recieved
		len = len >> 4;


		if(len != 0x00)
		{
			buf[rxtx_state] = FIFO;					// write the recieved bytes to the correct place of the
													// buffer;

			Trf796xReadCont(&buf[rxtx_state], len);
			rxtx_state = rxtx_state + len;
		}
#endif

		Trf796xStopDecoders();						// reset the FIFO after TX
		Trf796xReset();
		Trf796xResetIrqStatus();
		IRQ_CLR;
	}
	else if(*irq_status == BIT6)
	{	// RX flag means that EOF has been recieved
		// and the number of unread bytes is in FIFOstatus regiter	
		if(rx_error_flag == 0x02)
		{
			i_reg = 0x02;
			return;
		}

		*irq_status = FIFO_STATUS;
		Trf796xReadSingle(irq_status, 1);			// determine the number of bytes left in FIFO
		*irq_status = (0x0F &*irq_status) + 0x01;
		buf[rxtx_state] = FIFO;				// write the recieved bytes to the correct place of the buffer													
												
		Trf796xReadCont(&buf[rxtx_state], *irq_status);
		rxtx_state = rxtx_state +*irq_status;

		*irq_status = TX_LENGTH_BYTE_2;			// determine if there are broken bytes
		Trf796xReadCont(irq_status, 1);

		if((*irq_status & BIT0) == BIT0)
		{
			*irq_status = (*irq_status >> 1) & 0x07;	// mask the first 5 bits
			*irq_status = 8 -*irq_status;
			buf[rxtx_state - 1] &= 0xFF << *irq_status;
		}								// if 

		Trf796xReset();					// reset the FIFO after last byte has been read out
		i_reg = 0xFF;					// signal to the recieve funnction that this are the last bytes
	}	
	else if(*irq_status == 0x60)
	{									// RX active and 9 bytes allready in FIFO
		i_reg = 0x01;
		buf[rxtx_state] = FIFO;
		Trf796xReadCont(&buf[rxtx_state], 9);	// read 9 bytes from FIFO
		rxtx_state = rxtx_state + 9;
		Trf796xReadIrqStatus(irq_status);
		if (IRQ_PIN_HIGH)			// if IRQ pin high
		{
			IRQ_CLR;

			if(*irq_status == 0x40)							// end of recieve
			{	
				*irq_status = FIFO_STATUS;
				Trf796xReadSingle(irq_status, 1);					// determine the number of bytes left in FIFO
				*irq_status = 0x0F & (*irq_status + 0x01);
				buf[rxtx_state] = FIFO;						// write the recieved bytes to the correct place of the buffer
				
				Trf796xReadCont(&buf[rxtx_state], *irq_status);
				rxtx_state = rxtx_state +*irq_status;

				*irq_status = TX_LENGTH_BYTE_2;					// determine if there are broken bytes
				Trf796xReadSingle(irq_status, 1);					// determine the number of bits

				if((*irq_status & BIT0) == BIT0)
				{
					*irq_status = (*irq_status >> 1) & 0x07;	// mask the first 5 bits
					*irq_status = 8 -*irq_status;
					buf[rxtx_state - 1] &= 0xFF << *irq_status;
				}											// if

				i_reg = 0xFF;			// signal to the recieve funnction that this are the last bytes
				Trf796xReset();		// reset the FIFO after last byte has been read out
			}
			else if(*irq_status == 0x50)	// end of recieve and error
			{
				i_reg = 0x02;
			}
		}
		else
		{                            
			if(irq_status[0] == 0x00)
				i_reg = 0xFF;
		}
	}
	else if((*irq_status & BIT4) == BIT4)				// CRC error
	{
		if((*irq_status & BIT5) == BIT5)
		{	
			i_reg = 0x01;	// RX active
			rx_error_flag = 0x02;
		}
		if((*irq_status & BIT6) == BIT6)			// 4 Bit receive
		{
			buf[200] = FIFO;						// write the recieved bytes to the correct place of the buffer
		
			Trf796xReadCont(&buf[200], 1);

			Trf796xReset();
		
			i_reg = 0x02;	// end of RX
			rx_error_flag = 0x02;
		}
		else
		{
			i_reg = 0x02;	// end of RX
		}
	}
	else if((*irq_status & BIT2) == BIT2)	// byte framing error
	{
		if((*irq_status & BIT5) == BIT5)
		{
			i_reg = 0x01;	// RX active
			rx_error_flag = 0x02;
		}
		else
			i_reg = 0x02;	// end of RX
	}
	else if(*irq_status == BIT0)
		{						// No response interrupt
			i_reg = 0x00;
		}
		else
		{						// Interrupt register not properly set
			if(stand_alone_flag == 0)
			{
				#ifdef ENABLE_HOST
					UartSendCString("Interrupt error");
					UartPutByte(*irq_status);
				#endif
			}

			i_reg = 0x02;

			Trf796xStopDecoders();	// reset the FIFO after TX
			Trf796xReset();
			Trf796xResetIrqStatus();
			IRQ_CLR;
		}
}							// Interrupt Service Routine


//===============================================================
// NAME: void Trf796xRawWrite (uint8_t *pbuf, uint8_t length)
//
// BRIEF: Is used to write direct to the TRF796x.
//
// INPUTS:
//	Parameters:
//		uint8_t		*pbuf		raw data
//		uint8_t		length		number of data bytes
//
// OUTPUTS:
//
// PROCESS:	[1] send raw data to TRF796x
//
// CHANGE:
// DATE  		WHO	DETAIL
// 24Nov2010	RP	Original Code
//===============================================================

void 
Trf796xRawWrite(uint8_t *pbuf, uint8_t length)
{
	SSP_BeginTransaction(sspPort);
	SSP_PhaseChange(sspPort,0);
	SSP_Write(sspPort,pbuf,length);
	SSP_EndTransaction(sspPort);
}

//===============================================================
// 02DEC2010	RP	Original Code
//===============================================================

void
Trf796xReConfig(void)
{
//	if(SPIMODE)
//	{	
//		SpiUsciExtClkSet();       			// Set the USART
//    }
}

//===============================================================
// NAME: void Trf796xReadCont (uint8_t *pbuf, uint8_t length)
//
// BRIEF: Is used to read a specified number of TRF796x registers
// from a specified address upwards.
//
// INPUTS:
//	Parameters:
//		uint8_t		*pbuf		address of first register
//		uint8_t		length		number of registers
//
// OUTPUTS:
//
// PROCESS:	[1] read registers
//			[2] write contents to *pbuf
//
// CHANGE:
// DATE  		WHO	DETAIL
// 24Nov2010	RP	Original Code
//===============================================================

void
Trf796xReadCont(uint8_t *pbuf, uint8_t length)
{
	if (length==0)
			return;
	pbuf[0]&=0x1f;
//	if (pbuf[0]+length>0x1f)
//		length=0x20-pbuf[0];
	pbuf[0]|= ADDR_READ;
	if (length>1)
		pbuf[0]|=CONTINUOUS_ADDR_MODE;
	SSP_BeginTransaction(sspPort);
	SSP_PhaseChange(sspPort,0);
	SSP_Write(sspPort,pbuf,1);
	SSP_PhaseChange(sspPort,1);
	SSP_Read(sspPort,pbuf,length);
	SSP_EndTransaction(sspPort);
}

//===============================================================
// 02DEC2010	RP	Original Code
//===============================================================
uint8_t IRQStatus;
void
Trf796xReadIrqStatus(uint8_t *pbuf)
{
	*pbuf = IRQ_STATUS;
	*(pbuf + 1) = IRQ_MASK;
	Trf796xReadCont(pbuf, 2);			// read second reg. as dummy read
	IRQStatus=pbuf[0];
}

//===============================================================
// NAME: void Trf796xReadSingle (uint8_t *pbuf, uint8_t number)
//
// BRIEF: Is used to read specified TRF796x registers.
//
// INPUTS:
//	Parameters:
//		uint8_t		*pbuf		addresses of the registers
//		uint8_t		number		number of the registers
//
// OUTPUTS:
//
// PROCESS:	[1] read registers
//			[2] write contents to *pbuf
//
// CHANGE:
// DATE  		WHO	DETAIL
// 24Nov2010	RP	Original Code
//===============================================================

void
Trf796xReadSingle(uint8_t *pbuf, uint8_t number)
{
	int i;
	SSP_BeginTransaction(sspPort);
	for(i=0;i<number;++i)
	{
		pbuf[i]&=0x1f;
		pbuf[i]|= ADDR_READ;
		SSP_PhaseChange(sspPort,0);
		SSP_Write(sspPort,&pbuf[i],1);
		SSP_PhaseChange(sspPort,1);
		SSP_Read(sspPort,&pbuf[i],1);
	}
	SSP_EndTransaction(sspPort);
}

//===============================================================
// resets FIFO                                                  ;
// 02DEC2010	RP	Original Code
//===============================================================

void
Trf796xReset(void)
{
	command[0] = RESET;
	Trf796xDirectCommand(command,1);
}

//===============================================================
// resets IRQ Status                                            ;
// 02DEC2010	RP	Original Code
//===============================================================

void
Trf796xResetIrqStatus(void)
{
	uint8_t irq_status[4];
	irq_status[0] = IRQ_STATUS;
	irq_status[1] = IRQ_MASK;
	
	Trf796xReadCont(irq_status, 2);		// read second reg. as dummy read
}

//===============================================================
//                                                              ;
// 02DEC2010	RP	Original Code
//===============================================================

void
Trf796xRunDecoders(void)
{
	command[0] = RUN_DECODERS;
	Trf796xDirectCommand(command,1);
}

//===============================================================
// 02DEC2010	RP	Original Code
//===============================================================

void
Trf796xStopDecoders(void)
{
	command[0] = STOP_DECODERS;
	Trf796xDirectCommand(command,1);
}

//===============================================================
// 02DEC2010	RP	Original Code
//===============================================================

void
Trf796xTransmitNextSlot(void)
{
	command[0] = TRANSMIT_NEXT_SLOT;
	Trf796xDirectCommand(command,1);
}

//===============================================================
// 02DEC2010	RP	Original Code
//===============================================================

void
Trf796xTurnRfOff(void)
{
	command[0] = CHIP_STATE_CONTROL;
	command[1] = CHIP_STATE_CONTROL;
	Trf796xReadSingle(&command[1], 1);
	command[1] &= 0x1F;
	Trf796xWriteSingle(command, 2);
}

//===============================================================
// 02DEC2010	RP	Original Code
//===============================================================

void
Trf796xTurnRfOn(void)
{
	command[0] = CHIP_STATE_CONTROL;
	command[1] = CHIP_STATE_CONTROL;
	Trf796xReadSingle(&command[1], 1);
	command[1] &= 0x3F;
	command[1] |= 0x20;
	Trf796xWriteSingle(command, 2);
}

//===============================================================
// NAME: void SpiWriteCont (uint8_t *pbuf, uint8_t length)
//
// BRIEF: Is used to write to a specific number of reader chip
// registers from a specific address upwards.
//
// INPUTS:
//	uint8_t	*pbuf	address of first register followed by the
//					contents to write
//	uint8_t	length	number of registers + 1
//
// OUTPUTS:
//
// PROCESS:	[1] write to the registers
//
// CHANGE:
// DATE  		WHO	DETAIL
//===============================================================

void
Trf796xWriteCont(uint8_t *pbuf, uint8_t length)
{
	if (length==0)
			return;
	pbuf[0]&=0x1f;
	pbuf[0]|= ADDR_WRITE;
	if (length>2)
		pbuf[0]|=CONTINUOUS_ADDR_MODE;
	SSP_BeginTransaction(sspPort);
	SSP_PhaseChange(sspPort,0);
	SSP_Write(sspPort,pbuf,length);
	SSP_EndTransaction(sspPort);
}

//===============================================================
// 02DEC2010	RP	Original Code
//===============================================================

void
Trf796xWriteIsoControl(uint8_t iso_control)
{
	uint8_t write[4];
	
	write[0] = ISO_CONTROL;
	write[1] = iso_control;
	write[1] &= ~BIT5;
	Trf796xWriteSingle(write, 2);

	iso_control &= 0x1F;
	
	write[0] = IRQ_MASK;
	write[1] = 0x3E;
	Trf796xWriteSingle(write, 2);
	
	if(iso_control < 0x0C)					// ISO14443A or ISO15693
	{
		write[0] = MODULATOR_CONTROL;
		write[1] = 0x21;					// OOK 100% 6.78 MHz
		Trf796xWriteSingle(write, 2);
	}
	else									// ISO14443B
	{
		write[0] = MODULATOR_CONTROL;
		write[1] = 0x20;					// ASK 10% 6.78 MHz
		Trf796xWriteSingle(write, 2);
	}
	
	if(iso_control < 0x08)					// ISO15693
	{	
		write[0] = TX_PULSE_LENGTH_CONTROL;
		write[1] = 0x80;					// 9.44 us
		Trf796xWriteSingle(write, 2);
		
		if((iso_control < 0x02) || (iso_control == 0x04) || (iso_control == 0x05)) // low data rate
		{
			write[0] = RX_NO_RESPONSE_WAIT_TIME;
			write[1] = 0x30;				// 1812 us
			Trf796xWriteSingle(write, 2);
		}
		else
		{
			write[0] = RX_NO_RESPONSE_WAIT_TIME;
			write[1] = 0x14;				// 755 us
			Trf796xWriteSingle(write, 2);
		}
		
		write[0] = RX_WAIT_TIME;
		write[1] = 0x1F;					// 293 us
		Trf796xWriteSingle(write, 2);
		
		write[0] = RX_SPECIAL_SETTINGS;
		write[1] = RX_SPECIAL_SETTINGS;
		Trf796xReadSingle(&write[1], 1);
		write[1] &= 0x0F;
		write[1] |= 0x40;					// bandpass 200 kHz to 900 kHz 
		Trf796xWriteSingle(write, 2);
		
		write[0] = SPECIAL_FUNCTION;
		write[1] = SPECIAL_FUNCTION;
		Trf796xReadSingle(&write[1], 1);
		write[1] |= BIT4;
		Trf796xWriteSingle(write, 2);
	}
	else									// ISO14443
	{
		if(iso_control < 0x0C)				// ISO14443A
		{			
			write[0] = TX_PULSE_LENGTH_CONTROL;
			write[1] = 0x20;					// 2.36 us
			Trf796xWriteSingle(write, 2);
		}
		else
		{
			write[0] = TX_PULSE_LENGTH_CONTROL;
			write[1] = 0x00;					// 2.36 us
			Trf796xWriteSingle(write, 2);
		}
		write[0] = RX_NO_RESPONSE_WAIT_TIME;
		write[1] = 0x0E;					// 529 us
		Trf796xWriteSingle(write, 2);
		
		write[0] = RX_WAIT_TIME;
		write[1] = 0x07;					// 66 us
		Trf796xWriteSingle(write, 2);
		
		write[0] = RX_SPECIAL_SETTINGS;
		write[1] = RX_SPECIAL_SETTINGS;
		Trf796xReadSingle(&write[1], 1);
		write[1] &= 0x0F;					// bandpass 450 kHz to 1.5 MHz
		write[1] |= 0x20;
		Trf796xWriteSingle(write, 2);
		
		write[0] = SPECIAL_FUNCTION;
		write[1] = SPECIAL_FUNCTION;
		Trf796xReadSingle(&write[1], 1);
		write[1] &= ~BIT4;
		Trf796xWriteSingle(write, 2);
	}
}

//===============================================================
// NAME: void Trf796xWriteSingle (uint8_t *pbuf, uint8_t length)
//
// BRIEF: Is used to write to specified reader chip registers.
//
// INPUTS:
//	uint8_t	*pbuf	addresses of the registers followed by the
//					contents to write
//	uint8_t	length	number of registers * 2
//
// OUTPUTS:
//
// PROCESS:	[1] write to the registers
//
// CHANGE:
// DATE  		WHO	DETAIL
// 24Nov2010	RP	Original Code
//===============================================================

void
Trf796xWriteSingle(uint8_t *pbuf, uint8_t length)
{
	int i=0;
	SSP_BeginTransaction(sspPort);
	SSP_PhaseChange(sspPort,0);
	while(i<length)
	{
		pbuf[i]&=0x1f;
		pbuf[i]|= ADDR_WRITE;
		SSP_Write(sspPort,&pbuf[i],2);
		i+=2;
	}
	SSP_EndTransaction(sspPort);
}
