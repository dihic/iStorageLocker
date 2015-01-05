/****************************************************************************
 *   $Id:: ssp.c 3635 2010-06-02 00:31:46Z usb00423                         $
 *   Project: NXP LPC11xx SSP example
 *
 *   Description:
 *     This file contains SSP code example which include SSP 
 *     initialization, SSP interrupt handler, and APIs for SSP
 *     reading.
 *
 ****************************************************************************
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * products. This software is supplied "AS IS" without any warranties.
 * NXP Semiconductors assumes no responsibility or liability for the
 * use of the software, conveys no license or title under any patent,
 * copyright, or mask work right to the product. NXP Semiconductors
 * reserves the right to make changes in the software without
 * notification. NXP Semiconductors also make no representation or
 * warranty that such application will be suitable for the specified
 * use without further testing or modification.
****************************************************************************/
#include "LPC11xx.h"			/* LPC11xx Peripheral Registers */
#include "gpio.h"
#include "ssp.h"
#include "delay.h"

/* statistics of all the interrupts */
volatile uint32_t interruptRxStat0 = 0;
volatile uint32_t interruptOverRunStat0 = 0;
volatile uint32_t interruptRxTimeoutStat0 = 0;

volatile uint32_t interruptRxStat1 = 0;
volatile uint32_t interruptOverRunStat1 = 0;
volatile uint32_t interruptRxTimeoutStat1 = 0;

extern uint32_t SystemCoreClock;

uint32_t ssp0csPortNum=0;
uint32_t ssp1csPortNum=0;
uint32_t ssp0csBitPos=0;
uint32_t ssp1csBitPos=0;

/*****************************************************************************
** Function name:		SSP0_IRQHandler
**
** Descriptions:		SSP port is used for SPI communication.
**						SSP interrupt handler
**						The algorithm is, if RXFIFO is at least half full, 
**						start receive until it's empty; if TXFIFO is at least
**						half empty, start transmit until it's full.
**						This will maximize the use of both FIFOs and performance.
**
** parameters:			None
** Returned value:		None
** 
*****************************************************************************/
void SSP0_IRQHandler(void) 
{
  uint32_t regValue;

  regValue = LPC_SSP0->MIS;
  if ( regValue & SSPMIS_RORMIS )	/* Receive overrun interrupt */
  {
	interruptOverRunStat0++;
	LPC_SSP0->ICR = SSPICR_RORIC;	/* clear interrupt */
  }
  if ( regValue & SSPMIS_RTMIS )	/* Receive timeout interrupt */
  {
	interruptRxTimeoutStat0++;
	LPC_SSP0->ICR = SSPICR_RTIC;	/* clear interrupt */
  }

  /* please be aware that, in main and ISR, CurrentRxIndex and CurrentTxIndex
  are shared as global variables. It may create some race condition that main
  and ISR manipulate these variables at the same time. SSPSR_BSY checking (polling)
  in both main and ISR could prevent this kind of race condition */
  if ( regValue & SSPMIS_RXMIS )	/* Rx at least half full */
  {
	interruptRxStat0++;		/* receive until it's empty */		
  }
  return;
}

/*****************************************************************************
** Function name:		SSP1_IRQHandler
**
** Descriptions:		SSP port is used for SPI communication.
**						SSP interrupt handler
**						The algorithm is, if RXFIFO is at least half full, 
**						start receive until it's empty; if TXFIFO is at least
**						half empty, start transmit until it's full.
**						This will maximize the use of both FIFOs and performance.
**
** parameters:			None
** Returned value:		None
** 
*****************************************************************************/
void SSP1_IRQHandler(void) 
{
  uint32_t regValue;

  regValue = LPC_SSP1->MIS;
  if ( regValue & SSPMIS_RORMIS )	/* Receive overrun interrupt */
  {
	interruptOverRunStat1++;
	LPC_SSP1->ICR = SSPICR_RORIC;	/* clear interrupt */
  }
  if ( regValue & SSPMIS_RTMIS )	/* Receive timeout interrupt */
  {
	interruptRxTimeoutStat1++;
	LPC_SSP1->ICR = SSPICR_RTIC;	/* clear interrupt */
  }

  /* please be aware that, in main and ISR, CurrentRxIndex and CurrentTxIndex
  are shared as global variables. It may create some race condition that main
  and ISR manipulate these variables at the same time. SSPSR_BSY checking (polling)
  in both main and ISR could prevent this kind of race condition */
  if ( regValue & SSPMIS_RXMIS )	/* Rx at least half full */
  {
	interruptRxStat1++;		/* receive until it's empty */		
  }
  return;
}

/*****************************************************************************
** Function name:		SSP_IOConfig
**
** Descriptions:		SSP port initialization routine
**				
** parameters:			None
** Returned value:		None
** 
*****************************************************************************/
void SSP_IOConfig( uint8_t portNum )
{
  if ( portNum == 0 )
  {
	LPC_SYSCON->PRESETCTRL |= (0x1<<0);
	LPC_SYSCON->SYSAHBCLKCTRL |= (0x1<<11);
	LPC_SYSCON->SSP0CLKDIV = 0x01;			/* Divided by 2 */
	LPC_IOCON->PIO0_8           &= ~0x07;	/*  SSP I/O config */
	LPC_IOCON->PIO0_8           |= 0x01;		/* SSP MISO */
	LPC_IOCON->PIO0_9           &= ~0x07;	
	LPC_IOCON->PIO0_9           |= 0x01;		/* SSP MOSI */
//#ifdef __JTAG_DISABLED
//	LPC_IOCON->SCK_LOC = 0x00;
//	LPC_IOCON->SWCLK_PIO0_10 &= ~0x07;
//	LPC_IOCON->SWCLK_PIO0_10 |= 0x02;		/* SSP CLK */
//#else
//#if 0
//	/* On HummingBird/Candiru 1(HB1/CD1), SSP CLK can be routed to different 
//	pins, other than JTAG TCK, it's either P2.11 func. 1 or P0.6 func. 2. */
//	LPC_IOCON->SCK_LOC = 0x01;
//	LPC_IOCON->PIO2_11 = 0x01;	/* P2.11 function 1 is SSP clock, need to 
//								combined with IOCONSCKLOC register setting */
//#else
	LPC_IOCON->SCK_LOC = 0x02;
	LPC_IOCON->PIO0_6 = 0x02;	/* P0.6 function 2 is SSP clock, need to 
								combined with IOCONSCKLOC register setting */
//#endif
//#endif	/* endif __JTAG_DISABLED */  

#if USE_CS
	LPC_IOCON->PIO0_2 &= ~0x07;	
	LPC_IOCON->PIO0_2 |= 0x01;		/* SSP SSEL */
#else
	/* Enable AHB clock to the GPIO domain. */
	LPC_SYSCON->SYSAHBCLKCTRL |= (1<<6);
//	LPC_IOCON->PIO0_2 &= ~0x07;		/* SSP SSEL is a GPIO pin */
//	/* port0, bit 2 is set to GPIO output and high */
//	GPIOSetDir( PORT0, 2, 1 );
//	GPIOSetValue( PORT0, 2, 1 );
#endif
  }
  else		/* port number 1 */
  {
	LPC_SYSCON->PRESETCTRL |= (0x1<<2);
	LPC_SYSCON->SYSAHBCLKCTRL |= (1<<18);
	LPC_SYSCON->SSP1CLKDIV = 0x01;			/* Divided by 2 */
	LPC_IOCON->PIO2_2 &= ~0x07;	/*  SSP I/O config */
	LPC_IOCON->PIO2_2 |= 0x02;		/* SSP MISO */
	LPC_IOCON->PIO2_3 &= ~0x07;	
	LPC_IOCON->PIO2_3 |= 0x02;		/* SSP MOSI */
	LPC_IOCON->PIO2_1 &= ~0x07;
	LPC_IOCON->PIO2_1 |= 0x02;		/* SSP CLK */
 
#if USE_CS
	LPC_IOCON->PIO2_0 &= ~0x07;	
	LPC_IOCON->PIO2_0 |= 0x02;		/* SSP SSEL */
#else
	/* Enable AHB clock to the GPIO domain. */
	LPC_SYSCON->SYSAHBCLKCTRL |= (1<<6);
//	LPC_IOCON->PIO2_0 &= ~0x07;		/* SSP SSEL is a GPIO pin */
//	/* port2, bit 0 is set to GPIO output and high */
//	GPIOSetDir( PORT2, 0, 1 );
//	GPIOSetValue( PORT2, 0, 1 );
#endif
  }
  return;		
}

void SSP_CLK_Force(uint8_t portNum,uint32_t level)
{
	if (portNum==0)
	{
		LPC_IOCON->PIO0_6 &= ~0x07;
		GPIOSetDir( PORT0, 6, E_IO_OUTPUT);
		GPIOSetValue( PORT0, 6, level);
	}
	else
	{
		LPC_IOCON->PIO2_1 &= ~0x07;
		GPIOSetDir( PORT2, 1, E_IO_OUTPUT);
		GPIOSetValue( PORT2, 1, level);
	}
}

void SSP_CLK_Revert(uint8_t portNum)
{
	if (portNum==0)
	{
		LPC_IOCON->PIO0_6 = 0x02;
	}
	else
	{
		LPC_IOCON->PIO2_1 &= ~0x07;
		LPC_IOCON->PIO2_1 |= 0x02;		/* SSP CLK */
	}
}


void SSP_PhaseChange(uint8_t portNum,uint32_t phase)
{
	if ( portNum == 0 )
  {
		if (phase==0)
			LPC_SSP0->CR0 &= ~(1<<7);
		else
			LPC_SSP0->CR0 |= 1<<7;
	}
	else
	{
		if (phase==0)
			LPC_SSP1->CR0 &= ~(1<<7);
		else
			LPC_SSP1->CR0 |= 1<<7;
	}
}

/*****************************************************************************
** Function name:		SSP_Init
**
** Descriptions:		SSP port initialization routine
**				
** parameters:			None
** Returned value:		None
** 
*****************************************************************************/
void SSP_Init(uint8_t portNum,uint32_t mode,uint32_t csPortNum, uint32_t csBitPos)
{
  uint8_t i, Dummy=Dummy;
	uint32_t modecode=((mode & 0x01)<<7) | (mode & 0x02)<<5;
	
	GPIOSetDir(csPortNum,csBitPos,E_IO_OUTPUT);
	
  if ( portNum == 0 )
  {
		ssp0csPortNum=csPortNum;
		ssp0csBitPos=csBitPos;
		/* Set DSS data to 8-bit, Frame format SPI, CPOL = 0, CPHA = 0, and SCR is 15 */
		LPC_SSP0->CR0 = 0x0507|modecode;

		/* SSPCPSR clock prescale register, master mode, minimum divisor is 0x02 */
		LPC_SSP0->CPSR = 0x2;

		for ( i = 0; i < FIFOSIZE; i++ )
		{
			Dummy = LPC_SSP0->DR;		/* clear the RxFIFO */
		}

		/* Enable the SSP Interrupt */
		NVIC_EnableIRQ(SSP0_IRQn);
		
		/* Device select as master, SSP Enabled */
	#if LOOPBACK_MODE
		LPC_SSP0->CR1 = SSPCR1_LBM | SSPCR1_SSE;
	#else
	#if SSP_SLAVE
		/* Slave mode */
		if ( LPC_SSP0->CR1 & SSPCR1_SSE )
		{
			/* The slave bit can't be set until SSE bit is zero. */
			LPC_SSP0->CR1 &= ~SSPCR1_SSE;
		}
		LPC_SSP0->CR1 = SSPCR1_MS;		/* Enable slave bit first */
		LPC_SSP0->CR1 |= SSPCR1_SSE;	/* Enable SSP */
	#else
		/* Master mode */
		LPC_SSP0->CR1 = SSPCR1_SSE;
	#endif
	#endif
		/* Set SSPINMS registers to enable interrupts */
		/* enable all error related interrupts */
		LPC_SSP0->IMSC = SSPIMSC_RORIM | SSPIMSC_RTIM;
  }
  else
  {
		ssp1csPortNum=csPortNum;
		ssp1csBitPos=csBitPos;
		/* Set DSS data to 8-bit, Frame format SPI, CPOL = 1, CPHA = 1, and SCR is 15 */
		LPC_SSP1->CR0 = 0x0507|modecode;

		/* SSPCPSR clock prescale register, master mode, minimum divisor is 0x02 */
		LPC_SSP1->CPSR = 0x4;

		for ( i = 0; i < FIFOSIZE; i++ )
		{
			Dummy = LPC_SSP1->DR;		/* clear the RxFIFO */
		}

		/* Enable the SSP Interrupt */
		NVIC_EnableIRQ(SSP1_IRQn);
		
		/* Device select as master, SSP Enabled */
	#if LOOPBACK_MODE
		LPC_SSP1->CR1 = SSPCR1_LBM | SSPCR1_SSE;
	#else
	#if SSP_SLAVE
		/* Slave mode */
		if ( LPC_SSP1->CR1 & SSPCR1_SSE )
		{
			/* The slave bit can't be set until SSE bit is zero. */
			LPC_SSP1->CR1 &= ~SSPCR1_SSE;
		}
		LPC_SSP1->CR1 = SSPCR1_MS;		/* Enable slave bit first */
		LPC_SSP1->CR1 |= SSPCR1_SSE;	/* Enable SSP */
	#else
		/* Master mode */
		LPC_SSP1->CR1 = SSPCR1_SSE;
	#endif
	#endif
		/* Set SSPINMS registers to enable interrupts */
		/* enable all error related interrupts */
		LPC_SSP1->IMSC = SSPIMSC_RORIM | SSPIMSC_RTIM;
  }
  return;
}

void SSP_BeginTransaction(uint8_t portNum)
{
	#if !USE_CS
	if ( portNum == 0 )
		GPIOSetValue( ssp0csPortNum, ssp0csBitPos, 0 );
	else
		GPIOSetValue( ssp1csPortNum, ssp1csBitPos, 0 );
	DELAY(1);
	#endif
}

void SSP_EndTransaction(uint8_t portNum)
{
	#if !USE_CS
	DELAY(1);
	if ( portNum == 0 )
		GPIOSetValue( ssp0csPortNum, ssp0csBitPos, 1 );
	else
		GPIOSetValue( ssp1csPortNum, ssp1csBitPos, 1 );
	#endif
}

void SSP_WriteRead(uint8_t portNum, uint8_t *writebuf,uint8_t *readbuf,uint32_t Length )
{
  uint32_t i;
	uint8_t Dummy=Dummy;
	
//	#if !USE_CS
//	if ( portNum == 0 )
//		GPIOSetValue( ssp0csPortNum, ssp0csBitPos, 0 );
//	else
//		GPIOSetValue( ssp1csPortNum, ssp1csBitPos, 0 );
//	DELAY(1);
//	#endif
	
  for (i = 0; i < Length; i++)
  {
		/* As long as Receive FIFO is not empty, I can always receive. */
		/* If it's a loopback test, clock is shared for both TX and RX,
			 no need to write dummy byte to get clock to get the data */
		/* if it's a peer-to-peer communication, SSPDR needs to be written
			 before a read can take place. */
		if ( portNum == 0 )
		{
//	#if !LOOPBACK_MODE
//	#if SSP_SLAVE
//			while ( !(LPC_SSP0->SR & SSPSR_RNE) );
//	#else
			if (writebuf)
			{
				LPC_SSP0->DR = *writebuf;
				writebuf++;
			}
			else
				LPC_SSP0->DR = 0;
			/* Wait until the Busy bit is cleared */
			while ( (LPC_SSP0->SR & (SSPSR_BSY|SSPSR_RNE)) != SSPSR_RNE );
//	#endif
//	#else
//			while ( !(LPC_SSP0->SR & SSPSR_RNE) );
//	#endif
			if (readbuf)
			{
				*readbuf = LPC_SSP0->DR;
				readbuf++;
			}
			else
				Dummy=LPC_SSP0->DR;
		}
		else
		{
//	#if !LOOPBACK_MODE
//	#if SSP_SLAVE
//			while ( !(LPC_SSP1->SR & SSPSR_RNE) );
//	#else
			if (writebuf)
			{
				LPC_SSP1->DR = *writebuf;
				writebuf++;
			}
			else
				LPC_SSP1->DR = 0;
			/* Wait until the Busy bit is cleared */
			while ( (LPC_SSP1->SR & (SSPSR_BSY|SSPSR_RNE)) != SSPSR_RNE );
//	#endif
//	#else
//			while ( !(LPC_SSP1->SR & SSPSR_RNE) );
//	#endif
			if (readbuf)
			{
				*readbuf = LPC_SSP1->DR;
				readbuf++;
			}
			else
				Dummy=LPC_SSP1->DR;
		}
  }
//	#if !USE_CS
//	DELAY(1);
//	if ( portNum == 0 )
//		GPIOSetValue( ssp0csPortNum, ssp0csBitPos, 1);
//	else
//		GPIOSetValue( ssp1csPortNum, ssp1csBitPos, 1);
//	#endif
//  return; 
}

void SSP_Write(uint8_t portNum, uint8_t *buf, uint32_t Length )
{
	SSP_WriteRead(portNum, buf,0,Length);
}
void SSP_Read(uint8_t portNum, uint8_t *buf, uint32_t Length )
{
	SSP_WriteRead(portNum, 0,buf,Length);
}

/******************************************************************************
**                            End Of File
******************************************************************************/

