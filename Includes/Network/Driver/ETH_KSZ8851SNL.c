/* ----------------------------------------------------------------------
 * Copyright (C) 2013-2014 ARM Limited. All rights reserved.
 *
 * $Date:        9. December 2014
 * $Revision:    V6.02
 *
 * Driver:       Driver_ETH_MACn (default: Driver_ETH_MAC0),
                 Driver_ETH_PHYn (default: Driver_ETH_PHY0)
 * Project:      Ethernet Media Access (MAC) Driver and
                 Ethernet Physical Layer Transceiver (PHY) Driver
                 for KSZ8851SNL
 * ---------------------------------------------------------------------- 
 * Use the following configuration settings in the middleware component
 * to connect to this driver.
 *
 *   Configuration Setting                     Value
 *   ---------------------                     -----
 *   Connect to hardware via Driver_ETH_MAC# = n (default: 0)
 *                           Driver_ETH_PHY# = n (default: 0)
 * -------------------------------------------------------------------- */

/* History:
 *  Version 6.02
      GetMacAddress function implemented in Ethernet driver
 *  Version 6.01
 *    Corrected invalid power status in MAC_PowerControl
 *  Version 6.00
 *    Based on API V2.00
 *  Version 5.01
 *    Based on API V1.10 (namespace prefix ARM_ added)
 *  Version 5.00
 *    Initial release
 */

#include <string.h>
#include "cmsis_os.h"

#include "ETH_KSZ8851SNL.h"

#include "Driver_SPI.h"
#include "Driver_ETH_MAC.h"
#include "Driver_ETH_PHY.h"

#define ARM_ETH_MAC_DRV_VERSION ARM_DRIVER_VERSION_MAJOR_MINOR(6,00) /* driver version */
#define ARM_ETH_PHY_DRV_VERSION ARM_DRIVER_VERSION_MAJOR_MINOR(6,00) /* driver version */

/* Default ethernet driver number */
#ifndef ETH_NUM
#define ETH_NUM                 0
#endif

/* SPI port number definition */
#ifndef ETH_SPI_PORT
#define ETH_SPI_PORT            0
#endif

/* Receive/transmit Checksum offload enable */
#ifndef ETH_CHECKSUM_OFFLOAD
#define ETH_CHECKSUM_OFFLOAD    1
#endif

/* SPI bus speed */
#ifndef ETH_SPI_BUS_SPEED
#define ETH_SPI_BUS_SPEED       10000000
#endif

#define _SPI_Driver_(n)     Driver_SPI##n
#define  SPI_Driver_(n)    _SPI_Driver_(n)
extern ARM_DRIVER_SPI       SPI_Driver_(ETH_SPI_PORT);
#define ptrSPI             (&SPI_Driver_(ETH_SPI_PORT))

/* Timeouts */
#define TX_TIMEOUT          3000        /* Packet Transmit timeout in us */

/* Memory Buffer configuration */
#define ETH_BUF_SIZE        1536        /* Packet Transmit buffer size   */

/* Driver Version */
static const ARM_DRIVER_VERSION MAC_DriverVersion = {
  ARM_ETH_MAC_API_VERSION,
  ARM_ETH_MAC_DRV_VERSION
};

/* Driver Capabilities */
static const ARM_ETH_MAC_CAPABILITIES MAC_DriverCapabilities = {
  (ETH_CHECKSUM_OFFLOAD) ? 1 : 0,   /* checksum_offload_rx_ip4  */
  (ETH_CHECKSUM_OFFLOAD) ? 1 : 0,   /* checksum_offload_rx_ip6  */
  (ETH_CHECKSUM_OFFLOAD) ? 1 : 0,   /* checksum_offload_rx_udp  */
  (ETH_CHECKSUM_OFFLOAD) ? 1 : 0,   /* checksum_offload_rx_tcp  */
  (ETH_CHECKSUM_OFFLOAD) ? 1 : 0,   /* checksum_offload_rx_icmp */
  (ETH_CHECKSUM_OFFLOAD) ? 1 : 0,   /* checksum_offload_tx_ip4  */
  (ETH_CHECKSUM_OFFLOAD) ? 1 : 0,   /* checksum_offload_tx_ip6  */
  (ETH_CHECKSUM_OFFLOAD) ? 1 : 0,   /* checksum_offload_tx_udp  */
  (ETH_CHECKSUM_OFFLOAD) ? 1 : 0,   /* checksum_offload_tx_tcp  */
  (ETH_CHECKSUM_OFFLOAD) ? 1 : 0,   /* checksum_offload_tx_icmp */
  0,                                /* media_interface          */
  0,                                /* mac_address              */
  0,                                /* event_rx_frame           */
  0,                                /* event_tx_frame           */
  0,                                /* event_wakeup             */
  0                                 /* precision_timer          */
};

/* Local variables */
static ETH_CTRL  eth_control = { 0 };
#define eth     (eth_control)
static uint8_t   tx_buf[ETH_BUF_SIZE];

/* Local functions */
static uint16_t reg_rd (uint8_t reg);
static void     reg_wr (uint8_t reg, uint16_t val);
static uint32_t crc32_8bit_rev (uint32_t crc32, uint8_t val);
static uint32_t crc32_data (const uint8_t *data, uint32_t len);

/**
  \fn          uint16_t reg_rd (uint8_t reg)
  \brief       Read value from ETH register.
  \param[in]   reg  Register to read
*/
static uint16_t reg_rd (uint8_t reg) {
  uint8_t buf[2];

  buf[0] = SPI_CMD_REG_READ;
  if (reg & 2) buf[0] |= 0x3 << 4;
  else         buf[0] |= 0x3 << 2;
  buf[0] |= reg >> 6;
  buf[1]  = reg << 2;

  ptrSPI->Control (ARM_SPI_CONTROL_SS, ARM_SPI_SS_ACTIVE);
  ptrSPI->Send (buf, 2);
  while (ptrSPI->GetStatus().busy);
  ptrSPI->Receive (buf, 2);
  while (ptrSPI->GetStatus().busy);
  ptrSPI->Control(ARM_SPI_CONTROL_SS, ARM_SPI_SS_INACTIVE);

  return ((buf[1] << 8) | buf[0]);
}

/**
  \fn          void reg_wr (uint8_t reg, uint16_t val)
  \brief       Write value to ETH register.
  \param[in]   reg  Register to write
  \param[in]   val  Value to write
*/
static void reg_wr (uint8_t reg, uint16_t val) {
  uint8_t buf[4];

  buf[0] = SPI_CMD_REG_WRITE;
  if (reg & 2) buf[0] |= 0x3 << 4;
  else         buf[0] |= 0x3 << 2;
  buf[0] |= reg >> 6;
  buf[1]  = reg << 2;
  buf[2]  = (uint8_t)val;
  buf[3]  = (uint8_t)(val >> 8);

  ptrSPI->Control (ARM_SPI_CONTROL_SS, ARM_SPI_SS_ACTIVE);
  ptrSPI->Send (buf, 4);
  while (ptrSPI->GetStatus().busy);
  ptrSPI->Control(ARM_SPI_CONTROL_SS, ARM_SPI_SS_INACTIVE);
}

/**
  \fn          uint32_t crc32_8bit_rev (uint32_t crc32, uint8_t val)
  \brief       Calculate 32-bit CRC (Polynom: 0x04C11DB7, data bit-reversed).
  \param[in]   crc32  CRC initial value
  \param[in]   val    Input value
  \return      Calculated CRC value
*/
static uint32_t crc32_8bit_rev (uint32_t crc32, uint8_t val) {
  uint32_t n;

  crc32 ^= __rbit (val);
  for (n = 8; n; n--) {
    if (crc32 & 0x80000000) {
      crc32 <<= 1;
      crc32  ^= 0x04C11DB7;
    } else {
      crc32 <<= 1;
    }
  }
  return (crc32);
}

/**
  \fn          uint32_t crc32_data (const uint8_t *data, uint32_t len)
  \brief       Calculate standard 32-bit Ethernet CRC.
  \param[in]   data  Pointer to buffer containing the data
  \param[in]   len   Data length in bytes
  \return      Calculated CRC value
*/
static uint32_t crc32_data (const uint8_t *data, uint32_t len) {
  uint32_t crc;

  for (crc = 0xFFFFFFFF; len; len--) {
    crc = crc32_8bit_rev (crc, *data++);
  }
  return (crc);
}


/* Ethernet Driver functions */

/**
  \fn          ARM_DRIVER_VERSION GetVersion (void)
  \brief       Get driver version.
  \return      \ref ARM_DRIVER_VERSION
*/
static ARM_DRIVER_VERSION MAC_GetVersion (void) {
  return MAC_DriverVersion;
}

/**
  \fn          ARM_ETH_MAC_CAPABILITIES GetCapabilities (void)
  \brief       Get driver capabilities.
  \return      \ref ARM_ETH_MAC_CAPABILITIES
*/
static ARM_ETH_MAC_CAPABILITIES MAC_GetCapabilities (void) {
  return MAC_DriverCapabilities;
}

/**
  \fn          int32_t Initialize (ARM_ETH_MAC_SignalEvent_t cb_event)
  \brief       Initialize Ethernet MAC Device.
  \param[in]   cb_event  Pointer to \ref ARM_ETH_MAC_SignalEvent
  \return      \ref execution_status
*/
static int32_t MAC_Initialize (ARM_ETH_MAC_SignalEvent_t cb_event) {

  if (eth.flags & ETH_FLAG_POWER) {
    /* Driver initialize not allowed */
    return ARM_DRIVER_ERROR;
  }

  if (eth.flags & ETH_FLAG_INIT) {
    /* Driver already initialized */
    return ARM_DRIVER_OK;
  }
  eth.flags = ETH_FLAG_INIT;

  /* Initialize SPI interface */
  ptrSPI->Initialize (NULL);

  return ARM_DRIVER_OK;
}

/**
  \fn          int32_t Uninitialize (void)
  \brief       De-initialize Ethernet MAC Device.
  \return      \ref execution_status
*/
static int32_t MAC_Uninitialize (void) {

  if (!(eth.flags & ETH_FLAG_INIT)) {
    /* Driver not initialized */
    return ARM_DRIVER_OK;
  }

  if (eth.flags & ETH_FLAG_POWER) {
    /* Driver needs POWER_OFF first */
    return ARM_DRIVER_ERROR;
  }
  eth.flags = 0;

  /* Un-initialize SPI interface */
  ptrSPI->Uninitialize ();

  return ARM_DRIVER_OK;
}

/**
  \fn          int32_t PowerControl (ARM_POWER_STATE state)
  \brief       Control Ethernet MAC Device Power.
  \param[in]   state  Power state
  \return      \ref execution_status
*/
static int32_t MAC_PowerControl (ARM_POWER_STATE state) {
  uint16_t val;

  if (!(eth.flags & ETH_FLAG_INIT)) {
    /* Driver not initialized */
    return ARM_DRIVER_ERROR;
  }

  switch (state) {
    case ARM_POWER_OFF:
      if (!(eth.flags & ETH_FLAG_POWER)) {
        /* Driver not powered */
        break;
      }
      eth.flags = ETH_FLAG_INIT;

      /* Disable interrupts */
      reg_wr (REG_IER, 0);

      /* Disable Rx and flush Rx queue */
      reg_wr (REG_RXCR1, REG_RXCR1_FRXQ);
      reg_wr (REG_RXQCR, 0);
      reg_wr (REG_RXCR1, 0);

      /* Disable Tx and flush Tx queue */
      reg_wr (REG_TXCR,  REG_TXCR_FTXQ);
      reg_wr (REG_TXQCR, 0);
      reg_wr (REG_TXCR,  0);

      /* Enable Power Save Mode */
      reg_wr (REG_PMECR, REG_PMECR_PSAVE);

      ptrSPI->PowerControl(ARM_POWER_OFF);
      break;

    case ARM_POWER_LOW:
      return ARM_DRIVER_ERROR_UNSUPPORTED;

    case ARM_POWER_FULL:
      if (eth.flags & ETH_FLAG_POWER) {
        /* Driver already powered */
        break;
      }
      ptrSPI->PowerControl(ARM_POWER_FULL);
      ptrSPI->Control     (ARM_SPI_MODE_MASTER | ARM_SPI_CPOL0_CPHA0  |
                           ARM_SPI_MSB_LSB     | ARM_SPI_SS_MASTER_SW |
                           ARM_SPI_DATA_BITS(8), ETH_SPI_BUS_SPEED);

      /* Check Device Identification. */
      val = reg_rd (REG_CIDER);
      if ((val & 0xFFF0) != PHY_ID) {
        /* Wrong PHY device. */
        return ARM_DRIVER_ERROR_UNSUPPORTED;
      }

      /* Reset device */
      reg_wr (REG_GRR, REG_GRR_GSR);
      reg_wr (REG_GRR, 0);

      /* Enable Tx Frame Data Pointer Auto Increment */
      reg_wr (REG_TXFDPR, REG_TXFDPR_TXFPAI);

      /* Flush TX queue */
      reg_wr (REG_TXCR, REG_TXCR_FTXQ);

      /* Enable QMU transmit */
      eth.txcr  = REG_TXCR_TXCE | REG_TXCR_TXPE | REG_TXCR_TXFCE;
      reg_wr (REG_TXCR, eth.txcr);
      reg_wr (REG_TXQCR, REG_TXQCR_TXQMAM);

      /* Enable Rx Frame Data Pointer Auto Increment */
      reg_wr (REG_RXFDPR, REG_RXFDPR_RXFPAI);

      /* Configure Receive Frame Threshold for one frame */
      reg_wr (REG_RXFCTR, 1);

      /* Flush RX queue */
      reg_wr (REG_RXCR1, REG_RXCR1_FRXQ);

      /* Accept unicast packets and enable QMU receive */
      eth.rxcr1 = REG_RXCR1_RXPAFMA | REG_RXCR1_RXUE |
                  REG_RXCR1_RXMAFMA | REG_RXCR1_RXME | REG_RXCR1_RXFCE;
      reg_wr (REG_RXCR1, eth.rxcr1);

      eth.rxcr2 = REG_RXCR2_IUFFP | REG_RXCR2_UDPLFE | REG_RXCR2_SRDBL_FRM;
      reg_wr (REG_RXCR2, eth.rxcr2);

      /* Enable RX Frame Count Threshold and Auto-Dequeue */
      eth.rxqcr = REG_RXQCR_RXFCTE | REG_RXQCR_ADRFE;
      reg_wr (REG_RXQCR, eth.rxqcr);
      eth.rx_count = 0;

      eth.tx_id  = 0;
      eth.tx_len = 0;

      eth.flags |= ETH_FLAG_POWER;

      /* Enable receive interrupts */
      reg_wr (REG_IER, REG_IER_RXIE | REG_IER_RXOIE);

      /* Clear interrupt status */
      reg_wr (REG_ISR, 0xFFFF);
      break;

    default:
      return ARM_DRIVER_ERROR_UNSUPPORTED;
  }

  return ARM_DRIVER_OK;
}

/**
  \fn          int32_t GetMacAddress (ARM_ETH_MAC_ADDR *ptr_addr)
  \brief       Get Ethernet MAC Address.
  \param[in]   ptr_addr  Pointer to address
  \return      \ref execution_status
*/
static int32_t MAC_GetMacAddress (ARM_ETH_MAC_ADDR *ptr_addr) {
  uint16_t val;

  if (!ptr_addr) {
    /* Invalid parameters */
    return ARM_DRIVER_ERROR_PARAMETER;
  }

  val = reg_rd (REG_MARL);
  ptr_addr->b[5] = (uint8_t)val;
  ptr_addr->b[4] = (uint8_t)(val >> 8);

  val = reg_rd (REG_MARM);
  ptr_addr->b[3] = (uint8_t)val;
  ptr_addr->b[2] = (uint8_t)(val >> 8);

  val = reg_rd (REG_MARH);
  ptr_addr->b[1] = (uint8_t)val;
  ptr_addr->b[0] = (uint8_t)(val >> 8);

  return ARM_DRIVER_OK;
}

/**
  \fn          int32_t SetMacAddress (const ARM_ETH_MAC_ADDR *ptr_addr)
  \brief       Set Ethernet MAC Address.
  \param[in]   ptr_addr  Pointer to address
  \return      \ref execution_status
*/
static int32_t MAC_SetMacAddress (const ARM_ETH_MAC_ADDR *ptr_addr) {

  if (!ptr_addr) {
    /* Invalid parameters */
    return ARM_DRIVER_ERROR_PARAMETER;
  }

  if (!(eth.flags & ETH_FLAG_POWER)) {
    /* Driver not yet powered */
    return ARM_DRIVER_ERROR;
  }

  /* Write host MAC address */
  reg_wr(REG_MARL, (ptr_addr->b[4] << 8) | ptr_addr->b[5]);
  reg_wr(REG_MARM, (ptr_addr->b[2] << 8) | ptr_addr->b[3]);
  reg_wr(REG_MARH, (ptr_addr->b[0] << 8) | ptr_addr->b[1]);

  return ARM_DRIVER_OK;
}

/**
  \fn          int32_t SetAddressFilter (const ARM_ETH_MAC_ADDR *ptr_addr,
                                               uint32_t          num_addr)
  \brief       Configure Address Filter.
  \param[in]   ptr_addr  Pointer to addresses
  \param[in]   num_addr  Number of addresses to configure
  \return      \ref execution_status
*/
static int32_t MAC_SetAddressFilter (const ARM_ETH_MAC_ADDR *ptr_addr, uint32_t num_addr) {
  uint16_t ht[4];
  uint32_t crc;

  if (!ptr_addr && num_addr) {
    /* Invalid parameters */
    return ARM_DRIVER_ERROR_PARAMETER;
  }

  if (!(eth.flags & ETH_FLAG_POWER)) {
    /* Driver not yet powered */
    return ARM_DRIVER_ERROR;
  }

  if (num_addr == 0) {
    /* Disable multicast hash filtering (Mode= Perfect)*/
    eth.rxcr1 |=  (REG_RXCR1_RXMAFMA | REG_RXCR1_RXPAFMA);
    eth.rxcr1 &= ~(REG_RXCR1_RXAE    | REG_RXCR1_RXINVF);
    reg_wr (REG_RXCR1, eth.rxcr1);
    return ARM_DRIVER_OK;
  }

  /* Calculate 64-bit Hash table for MAC addresses */
  ht[0] = 0;
  ht[1] = 0;
  ht[2] = 0;
  ht[3] = 0;
  
  for ( ; num_addr; ptr_addr++, num_addr--) {
    crc = crc32_data (&ptr_addr->b[0], 6) >> 26;
    ht[crc >> 4] |= (1 << (crc & 0x0F));
  }
  reg_wr (REG_MAHTR0, ht[0]);
  reg_wr (REG_MAHTR1, ht[1]);
  reg_wr (REG_MAHTR2, ht[2]);
  reg_wr (REG_MAHTR3, ht[3]);

  /* Enable multicast hash filtering (Mode= Hash perfect) */
  eth.rxcr1 |=   REG_RXCR1_RXPAFMA;
  eth.rxcr1 &= ~(REG_RXCR1_RXMAFMA | REG_RXCR1_RXAE | REG_RXCR1_RXINVF);

  reg_wr (REG_RXCR1, eth.rxcr1);

  return ARM_DRIVER_OK;
}

/**
  \fn          int32_t SendFrame (const uint8_t *frame, uint32_t len, uint32_t flags)
  \brief       Send Ethernet frame.
  \param[in]   frame  Pointer to frame buffer with data to send
  \param[in]   len    Frame buffer length in bytes
  \param[in]   flags  Frame transmit flags (see ARM_ETH_MAC_TX_FRAME_...)
  \return      \ref execution_status
*/
static int32_t MAC_SendFrame (const uint8_t *frame, uint32_t len, uint32_t flags) {
  uint8_t  hdr[4];
  uint32_t tick;
  int32_t  err;

  if (!frame || !len) {
    /* Invalid parameters */
    return ARM_DRIVER_ERROR_PARAMETER;
  }

  if (!(eth.flags & ETH_FLAG_POWER)) {
    /* Driver not yet powered */
    return ARM_DRIVER_ERROR;
  }

  if (eth.tx_len == 0) {
    /* Start of a new transmit frame */
    if ((reg_rd (REG_TXMIR) & 0x1FFF) < ETH_BUF_SIZE) {
      /* Tx is busy, not enough memory free in Tx queue */
      return ARM_DRIVER_ERROR_BUSY;
    }
    if (flags & ARM_ETH_MAC_TX_FRAME_FRAGMENT) {
      /* Fragmented frame, copy to local buffer */
      memcpy (tx_buf, frame, len);
      eth.tx_len = len;
      return ARM_DRIVER_OK;
    }
  }
  else {
    /* Sending frame fragments in progress */
    memcpy (tx_buf + eth.tx_len, frame, len);
    len += eth.tx_len;
    if (flags & ARM_ETH_MAC_TX_FRAME_FRAGMENT) {
      /* More fragments to come */
      eth.tx_len = len;
      return ARM_DRIVER_OK;
    }
    frame = tx_buf;
  }

  /* Disable interrupts */
  reg_wr (REG_IER, 0);

  /* Set Start DMA Access bit in RXQCR */
  reg_wr (REG_RXQCR, eth.rxqcr | REG_RXQCR_SDA);

  ptrSPI->Control (ARM_SPI_CONTROL_SS, ARM_SPI_SS_ACTIVE);

  hdr[0] = SPI_CMD_TXQ_FIFO_WR;
  ptrSPI->Send (hdr, 1);
  while (ptrSPI->GetStatus().busy);

  /* Send TX frame header */
  hdr[0] = eth.tx_id++ & 0x3F;
  hdr[1] = 0;
  hdr[2] = (uint8_t)len;
  hdr[3] = (uint8_t)(len >> 8);
  ptrSPI->Send (hdr, 4);
  while (ptrSPI->GetStatus().busy);

  /* Send frame data, add dummy bytes to align to 4 bytes */
  ptrSPI->Send (frame, (len + 3) & ~3);
  while (ptrSPI->GetStatus().busy);

  ptrSPI->Control (ARM_SPI_CONTROL_SS, ARM_SPI_SS_INACTIVE);

  /* Clear Start DMA Access bit in RXQCR */
  reg_wr (REG_RXQCR, eth.rxqcr);
  reg_wr (REG_TXQCR, REG_TXQCR_TXQMAM | REG_TXQCR_METFE);

  /* Wait until transmit done */
  err  = ARM_DRIVER_ERROR_TIMEOUT;
  tick = osKernelSysTick();
  do {
    if (!(reg_rd (REG_TXQCR) & REG_TXQCR_METFE)) {
      /* Enqueue bit cleared, frame sent */
      err = ARM_DRIVER_OK;
      break;
    }
  }
  while ((osKernelSysTick() - tick) < osKernelSysTickMicroSec(TX_TIMEOUT));

  /* Enable interrupts */
  reg_wr (REG_IER, REG_IER_RXIE | REG_IER_RXOIE);

  return err;
}

/**
  \fn          int32_t ReadFrame (uint8_t *frame, uint32_t len)
  \brief       Read data of received Ethernet frame.
  \param[in]   frame  Pointer to frame buffer for data to read into
  \param[in]   len    Frame buffer length in bytes
  \return      number of data bytes read or execution status
                 - value >= 0: number of data bytes read
                 - value < 0: error occurred, value is execution status as defined with \ref execution_status 
*/
static int32_t MAC_ReadFrame (uint8_t *frame, uint32_t len) {
  uint8_t buf[8];

  if (!frame && len) {
    /* Invalid parameters */
    return ARM_DRIVER_ERROR_PARAMETER;
  }

  if (!(eth.flags & ETH_FLAG_POWER)) {
    /* Driver not yet powered */
    return ARM_DRIVER_ERROR;
  }

  eth.rx_count--;
  if (len == 0) {
    /* Release inconsistent frame from the RXQ */
    reg_wr (REG_RXQCR, eth.rxqcr | REG_RXQCR_RRXEF);
    while (reg_rd (REG_RXQCR) & REG_RXQCR_RRXEF);

    return 0;
  }

  /* Clear receive frame pointer */
  reg_wr (REG_RXFDPR, REG_RXFDPR_RXFPAI);

  /* Set Start DMA Access bit in RXQCR */
  reg_wr (REG_RXQCR, eth.rxqcr | REG_RXQCR_SDA);

  ptrSPI->Control (ARM_SPI_CONTROL_SS, ARM_SPI_SS_ACTIVE);

  /* Send RXQ FIFO Read command */
  buf[0] = SPI_CMD_RXQ_FIFO_RD;
  ptrSPI->Send (buf, 1);
  while (ptrSPI->GetStatus().busy);

  /* Read dummy bytes of frame header */
  ptrSPI->Receive (buf, 8);
  while (ptrSPI->GetStatus().busy);

  /* Read received frame */
  ptrSPI->Receive (frame, len);
	while (ptrSPI->GetStatus().busy);

  /* Read dummy bytes for alignment */
  if (len & 3) 
	{
		ptrSPI->Receive (buf, 4 - (len & 3));
		while (ptrSPI->GetStatus().busy);
	}

  ptrSPI->Control (ARM_SPI_CONTROL_SS, ARM_SPI_SS_INACTIVE);

  /* Clear Start DMA Access bit in RXQCR */
  reg_wr (REG_RXQCR, eth.rxqcr);

  return len;
}

/**
  \fn          uint32_t GetRxFrameSize (void)
  \brief       Get size of received Ethernet frame.
  \return      number of bytes in received frame
*/
static uint32_t MAC_GetRxFrameSize (void) {
  uint16_t reg_val;
  uint16_t err;

  if (eth.rx_count == 0) {
    /* Read interrupt status */
    reg_val = reg_rd (REG_ISR);
    if (!(reg_val & (REG_ISR_RXIS | REG_ISR_RXOIS))) {
      return (0);
    }
    /* Clear interrupt status */
    reg_wr (REG_ISR, reg_val);

    /* Read receive frame count */
    eth.rx_count = reg_rd (REG_RXFCTR) >> 8;

    if (eth.rx_count == 0) {
      /* Receive overrun, flush RXQ */
      reg_val = eth.rxcr1 & ~REG_RXCR1_RXE;
      reg_wr (REG_RXCR1, reg_val);
      reg_wr (REG_RXCR1, reg_val | REG_RXCR1_FRXQ);
      reg_wr (REG_RXCR1, eth.rxcr1);
      return (0);
    }
  }

  /* Check receive frame header status */
  reg_val = reg_rd (REG_RXFHSR);

  /* Check if frame is valid */
  if (!(reg_val & REG_RXFHSR_RXFV)) {
    reg_rd (REG_RXFHBCR);
    return (0);
  }

  err  = REG_RXFHSR_RXMR | REG_RXFHSR_RXFTL | REG_RXFHSR_RXRF | REG_RXFHSR_RXCE;
#if (ETH_CHECKSUM_OFFLOAD)
  err |= REG_RXFHSR_RXICMPFCS | REG_RXFHSR_RXIPFCS | REG_RXFHSR_RXTCPFCS | REG_RXFHSR_RXUDPFCS;
#endif
  if (reg_val & err) {
    /* Error, this frame is invalid */
    return (0xFFFFFFFF);
  }

  /* Read byte count (no CRC) */
  reg_val = reg_rd (REG_RXFHBCR);
  return ((reg_val & 0x0FFF) - 4);
}

/**
  \fn          int32_t GetRxFrameTime (ARM_ETH_MAC_TIME *time)
  \brief       Get time of received Ethernet frame.
  \param[in]   time  Pointer to time structure for data to read into
  \return      \ref execution_status
*/
static int32_t MAC_GetRxFrameTime (ARM_ETH_MAC_TIME *time) {
  return ARM_DRIVER_ERROR_UNSUPPORTED;
}

/**
  \fn          int32_t GetTxFrameTime (ARM_ETH_MAC_TIME *time)
  \brief       Get time of transmitted Ethernet frame.
  \param[in]   time  Pointer to time structure for data to read into
  \return      \ref execution_status
*/
static int32_t MAC_GetTxFrameTime (ARM_ETH_MAC_TIME *time) {
  return ARM_DRIVER_ERROR_UNSUPPORTED;
}

/**
  \fn          int32_t ControlTimer (uint32_t control, ARM_ETH_MAC_TIME *time)
  \brief       Control Precision Timer.
  \param[in]   control  Operation
  \param[in]   time     Pointer to time structure
  \return      \ref execution_status
*/
static int32_t MAC_ControlTimer (uint32_t control, ARM_ETH_MAC_TIME *time) {
  return ARM_DRIVER_ERROR_UNSUPPORTED;
}

/**
  \fn          int32_t Control (uint32_t control, uint32_t arg)
  \brief       Control Ethernet Interface.
  \param[in]   control  Operation
  \param[in]   arg      Argument of operation (optional)
  \return      \ref execution_status
*/
static int32_t MAC_Control (uint32_t control, uint32_t arg) {

  if (!(eth.flags & ETH_FLAG_POWER)) {
    /* Driver not powered */
    return ARM_DRIVER_ERROR;
  }

  switch (control) {
    case ARM_ETH_MAC_CONFIGURE:
      /* Duplex and speed not used here */
      switch (arg & ARM_ETH_MAC_SPEED_Msk) {
        case ARM_ETH_MAC_SPEED_10M:
        case ARM_ETH_SPEED_100M:
          /* Not used in this driver */
          break;
        default:
          return ARM_DRIVER_ERROR_UNSUPPORTED;
      }

#if (ETH_CHECKSUM_OFFLOAD)
      /* Enable rx checksum verification */
      eth.rxcr1 &= ~(REG_RXCR1_RXIPFCC | REG_RXCR1_RXTCPFCC | REG_RXCR1_RXUDPFCC);
      eth.rxcr2 &= ~REG_RXCR2_RXICMPFCC;

      if (arg & ARM_ETH_MAC_CHECKSUM_OFFLOAD_RX) {
        eth.rxcr1 |= (REG_RXCR1_RXIPFCC | REG_RXCR1_RXTCPFCC | REG_RXCR1_RXUDPFCC);
        eth.rxcr2 |= REG_RXCR2_RXICMPFCC;
      }

      /* Enable tx checksum generation */
      eth.txcr &= ~(REG_TXCR_TCGIP  | REG_TXCR_TCGTCP |
                    REG_TXCR_TCGUDP | REG_TXCR_TCGICMP);

      if (arg & ARM_ETH_MAC_CHECKSUM_OFFLOAD_TX) {
        eth.txcr |= (REG_TXCR_TCGIP  | REG_TXCR_TCGTCP |
                     REG_TXCR_TCGUDP | REG_TXCR_TCGICMP);
      }
      reg_wr (REG_RXCR2, eth.rxcr2);
      reg_wr (REG_TXCR, eth.txcr);
#else
      if ((arg & ARM_ETH_MAC_CHECKSUM_OFFLOAD_RX) ||
          (arg & ARM_ETH_MAC_CHECKSUM_OFFLOAD_TX)) {
        /* Checksum offload is disabled in the driver */
        return ARM_DRIVER_ERROR_UNSUPPORTED;
      }
#endif

      /* Configure loopback mode */
      if (arg & ARM_ETH_MAC_LOOPBACK) {
        /* Local Loopback in MAC not possible  */
        /* Far-end loopback in PHY Port 1 only */
      }

      /* Configure address filtering */
      /* Ref. KSZ8851SNL datasheet Rev. 2.0 */
      /* Table 3. Address Filtering Scheme  */
      eth.rxcr1 &= ~(REG_RXCR1_RXBE | REG_RXCR1_RXAE | REG_RXCR1_RXINVF);
      eth.rxcr1 |= (REG_RXCR1_RXPAFMA | REG_RXCR1_RXMAFMA);

      /* Enable broadcast frame receive */
      if (arg & ARM_ETH_MAC_ADDRESS_BROADCAST) {
        eth.rxcr1 |= REG_RXCR1_RXBE;
      }

      /* Enable all multicast frame receive */
      /* (Mode= Perfect with Multicast address passed) */
      if (arg & ARM_ETH_MAC_ADDRESS_MULTICAST) {
        eth.rxcr1 |= REG_RXCR1_RXAE;
      }

      /* Enable all frame receive (Mode= Promiscuous) */
      if (arg & ARM_ETH_MAC_ADDRESS_ALL) {
        eth.rxcr1 &= ~(REG_RXCR1_RXPAFMA | REG_RXCR1_RXMAFMA);
        eth.rxcr1 |=  (REG_RXCR1_RXAE    | REG_RXCR1_RXINVF);
      }
      reg_wr (REG_RXCR1, eth.rxcr1);
      break;

    case ARM_ETH_MAC_CONTROL_TX:
      /* Enable/disable MAC transmitter */
      if (arg != 0) eth.txcr |=  REG_TXCR_TXE;
      else          eth.txcr &= ~REG_TXCR_TXE;
      reg_wr (REG_TXCR, eth.txcr);
      break;

    case ARM_ETH_MAC_CONTROL_RX:
      /* Enable/disable MAC receiver */
      if (arg != 0) eth.rxcr1 |=  REG_RXCR1_RXE;
      else          eth.rxcr1 &= ~REG_RXCR1_RXE;
      reg_wr (REG_RXCR1, eth.rxcr1);
      break;

    case ARM_ETH_MAC_FLUSH:
      /* Flush Tx and Rx buffers */
      if (arg & ARM_ETH_MAC_FLUSH_RX) {
        reg_wr (REG_RXCR1, eth.rxcr1 | REG_RXCR1_FRXQ);
        reg_wr (REG_RXCR1, eth.rxcr1);
        eth.tx_len = 0;
      }
      if (arg & ARM_ETH_MAC_FLUSH_TX) {
        reg_wr (REG_TXCR, eth.txcr | REG_TXCR_FTXQ);
        reg_wr (REG_TXCR, eth.txcr);
        eth.rx_count = 0;
      }
      break;

    default:
      return ARM_DRIVER_ERROR_UNSUPPORTED;
  }
  return ARM_DRIVER_OK;
}

/* MAC Driver Control Block */
ARM_DRIVER_ETH_MAC ARM_Driver_ETH_MAC_(ETH_NUM) = {
  MAC_GetVersion,
  MAC_GetCapabilities,
  MAC_Initialize,
  MAC_Uninitialize,
  MAC_PowerControl,
  MAC_GetMacAddress,
  MAC_SetMacAddress,
  MAC_SetAddressFilter,
  MAC_SendFrame,
  MAC_ReadFrame,
  MAC_GetRxFrameSize,
  MAC_GetRxFrameTime,
  MAC_GetTxFrameTime,
  MAC_ControlTimer,
  MAC_Control,
  NULL,
  NULL
};


/* Driver Version */
static const ARM_DRIVER_VERSION PHY_DriverVersion = {
  ARM_ETH_PHY_API_VERSION,
  ARM_ETH_PHY_DRV_VERSION
};

/**
  \fn          ARM_DRV_VERSION GetVersion (void)
  \brief       Get driver version.
  \return      \ref ARM_DRV_VERSION
*/
static ARM_DRIVER_VERSION PHY_GetVersion (void) {
  return PHY_DriverVersion;
}

/**
  \fn          int32_t Initialize (ARM_ETH_PHY_Read_t  fn_read,
                                   ARM_ETH_PHY_Write_t fn_write)
  \brief       Initialize Ethernet PHY Device.
  \param[in]   fn_read   Pointer to \ref ARM_ETH_MAC_PHY_Read
  \param[in]   fn_write  Pointer to \ref ARM_ETH_MAC_PHY_Write
  \return      \ref execution_status
*/
static int32_t PHY_Initialize (ARM_ETH_PHY_Read_t fn_read, ARM_ETH_PHY_Write_t fn_write) {
  return ARM_DRIVER_OK;
}

/**
  \fn          int32_t Uninitialize (void)
  \brief       De-initialize Ethernet PHY Device.
  \return      \ref execution_status
*/
static int32_t PHY_Uninitialize (void) {
  return ARM_DRIVER_OK;
}

/**
  \fn          int32_t PowerControl (ARM_POWER_STATE state)
  \brief       Control Ethernet PHY Device Power.
  \param[in]   state  Power state
  \return      \ref execution_status
*/
static int32_t PHY_PowerControl (ARM_POWER_STATE state) {
  uint16_t reg_val;

  reg_val = reg_rd (REG_PMECR) & ~REG_PMECR_PMASK;
  switch (state) {
    case ARM_POWER_OFF:
      /* Select Power Saving Mode */
      reg_val |= REG_PMECR_PSAVE;
      break;
    case ARM_POWER_FULL:
      /* Select Normal Operation Mode */
      reg_val |= REG_PMECR_PNORMAL;
      break;
    default:
      return ARM_DRIVER_ERROR_UNSUPPORTED;
  }
  reg_wr (REG_PMECR, reg_val);

  return ARM_DRIVER_OK;
}

/**
  \fn          int32_t SetInterface (uint32_t interface)
  \brief       Set Ethernet Media Interface.
  \param[in]   interface  Media Interface type
  \return      \ref execution_status
*/
static int32_t PHY_SetInterface (uint32_t interface) {
  /* Not used in this driver */
  return ARM_DRIVER_OK;
}

/**
  \fn          int32_t SetMode (uint32_t mode)
  \brief       Set Ethernet PHY Device Operation mode.
  \param[in]   mode  Operation Mode
  \return      \ref execution_status
*/
static int32_t PHY_SetMode (uint32_t mode) {
  uint16_t val;

  val  = reg_rd (REG_P1MBCR);
  val &= ~(REG_P1MBCR_FORCE100 | REG_P1MBCR_FORCEFD | REG_P1MBCR_ANEN |
           REG_P1MBCR_DISTX    | REG_P1MBCR_LLB);

  switch (mode & ARM_ETH_PHY_SPEED_Msk) {
    case ARM_ETH_PHY_SPEED_10M:
      break;
    case ARM_ETH_PHY_SPEED_100M:
      val |=  REG_P1MBCR_FORCE100;
      break;
    default:
      return ARM_DRIVER_ERROR_UNSUPPORTED;
  }

  switch (mode & ARM_ETH_PHY_DUPLEX_Msk) {
    case ARM_ETH_PHY_DUPLEX_HALF:
      break;
    case ARM_ETH_PHY_DUPLEX_FULL:
      val |=  REG_P1MBCR_FORCEFD;
      break;
  }

  if (mode & ARM_ETH_PHY_AUTO_NEGOTIATE) {
    val |= REG_P1MBCR_ANEN;
  }

  if (mode & ARM_ETH_PHY_LOOPBACK) {
    val |= REG_P1MBCR_LLB;
  }

  if (mode & ARM_ETH_PHY_ISOLATE) {
    val |= REG_P1CR_TXIDS;
  }

  reg_wr (REG_P1MBCR, val);

  return ARM_DRIVER_OK;
}

/**
  \fn          ARM_ETH_LINK_STATE GetLinkState (void)
  \brief       Get Ethernet PHY Device Link state.
  \return      current link status \ref ARM_ETH_LINK_STATE
*/
static ARM_ETH_LINK_STATE PHY_GetLinkState (void) {

  if (reg_rd (REG_P1SR) & REG_P1SR_LINKGOOD) {
    /* Link Good bit is set */
    return ARM_ETH_LINK_UP;
  }
  return ARM_ETH_LINK_DOWN;
}

/**
  \fn          ARM_ETH_LINK_INFO GetLinkInfo (void)
  \brief       Get Ethernet PHY Device Link information.
  \return      current link parameters \ref ARM_ETH_LINK_INFO
*/
static ARM_ETH_LINK_INFO PHY_GetLinkInfo (void) {
  ARM_ETH_LINK_INFO info;
  uint16_t val;

  val = reg_rd (REG_P1SR);
  info.speed  = (val & REG_P1SR_OPSPEED)  ? ARM_ETH_SPEED_100M  : ARM_ETH_SPEED_10M;
  info.duplex = (val & REG_P1SR_OPDUPLEX) ? ARM_ETH_DUPLEX_FULL : ARM_ETH_DUPLEX_HALF;

  return (info);
}

/* PHY Driver Control Block */
ARM_DRIVER_ETH_PHY ARM_Driver_ETH_PHY_(ETH_NUM) = {
  PHY_GetVersion,
  PHY_Initialize,
  PHY_Uninitialize,
  PHY_PowerControl,
  PHY_SetInterface,
  PHY_SetMode,
  PHY_GetLinkState,
  PHY_GetLinkInfo
};
