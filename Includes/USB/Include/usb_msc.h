/*------------------------------------------------------------------------------
 * MDK Middleware - Component ::USB
 * Copyright (c) 2004-2014 ARM Germany GmbH. All rights reserved.
 *------------------------------------------------------------------------------
 * Name:    usb_msc.h
 * Purpose: USB Mass Storage Device Class Definitions
 * Rev.:    V6.2
 *----------------------------------------------------------------------------*/

#ifndef __USB_MSC_H__
#define __USB_MSC_H__

#include <stdint.h>

/* MSC Subclass Codes */
#define MSC_SUBCLASS_RBC                0x01
#define MSC_SUBCLASS_SFF8020I_MMC2      0x02
#define MSC_SUBCLASS_QIC157             0x03
#define MSC_SUBCLASS_UFI                0x04
#define MSC_SUBCLASS_SFF8070I           0x05
#define MSC_SUBCLASS_SCSI               0x06

/* MSC Protocol Codes */
#define MSC_PROTOCOL_CBI_INT            0x00
#define MSC_PROTOCOL_CBI_NOINT          0x01
#define MSC_PROTOCOL_BULK_ONLY          0x50


/* MSC Request Codes */
#define MSC_REQUEST_RESET               0xFF
#define MSC_REQUEST_GET_MAX_LUN         0xFE


/* MSC Bulk-only Stage */
#define MSC_BS_CBW                      0       /* Command Block Wrapper */
#define MSC_BS_DATA_OUT                 1       /* Data Out Phase */
#define MSC_BS_DATA_IN                  2       /* Data In Phase */
#define MSC_BS_DATA_IN_LAST             3       /* Data In Last Phase */
#define MSC_BS_DATA_IN_LAST_STALL       4       /* Data In Last Phase with Stall */
#define MSC_BS_CSW                      5       /* Command Status Wrapper */
#define MSC_BS_ERROR                    6       /* Error */

#ifdef __GNUC__
#pragma push
#pragma pack(1)

/* Bulk-only Command Block Wrapper */
typedef struct _MSC_CBW {
  uint32_t dSignature;
  uint32_t dTag;
  uint32_t dDataLength;
  uint8_t  bmFlags;
  uint8_t  bLUN;
  uint8_t  bCBLength;
  uint8_t  CB[16];
} MSC_CBW;

/* Bulk-only Command Status Wrapper */
typedef struct _MSC_CSW {
  uint32_t dSignature;
  uint32_t dTag;
  uint32_t dDataResidue;
  uint8_t  bStatus;
} MSC_CSW;

#pragma pop

#else

/* Bulk-only Command Block Wrapper */
typedef __packed struct _MSC_CBW {
  uint32_t dSignature;
  uint32_t dTag;
  uint32_t dDataLength;
  uint8_t  bmFlags;
  uint8_t  bLUN;
  uint8_t  bCBLength;
  uint8_t  CB[16];
} MSC_CBW;

/* Bulk-only Command Status Wrapper */
typedef __packed struct _MSC_CSW {
  uint32_t dSignature;
  uint32_t dTag;
  uint32_t dDataResidue;
  uint8_t  bStatus;
} MSC_CSW;

#endif

#define MSC_CBW_Signature               0x43425355
#define MSC_CSW_Signature               0x53425355


/* CSW Status Definitions */
#define CSW_CMD_PASSED                  0x00
#define CSW_CMD_FAILED                  0x01
#define CSW_PHASE_ERROR                 0x02


/* SCSI Commands */
#define SCSI_TEST_UNIT_READY            0x00
#define SCSI_REQUEST_SENSE              0x03
#define SCSI_FORMAT_UNIT                0x04
#define SCSI_INQUIRY                    0x12
#define SCSI_MODE_SELECT6               0x15
#define SCSI_MODE_SENSE6                0x1A
#define SCSI_START_STOP_UNIT            0x1B
#define SCSI_MEDIA_REMOVAL              0x1E
#define SCSI_READ_FORMAT_CAPACITIES     0x23
#define SCSI_READ_CAPACITY              0x25
#define SCSI_READ10                     0x28
#define SCSI_WRITE10                    0x2A
#define SCSI_VERIFY10                   0x2F
#define SCSI_SYNC_CACHE10               0x35
#define SCSI_READ12                     0xA8
#define SCSI_WRITE12                    0xAA
#define SCSI_MODE_SELECT10              0x55
#define SCSI_MODE_SENSE10               0x5A
#define SCSI_SYNC_CACHE16               0x91
#define SCSI_ATA_COMMAND_PASS_THROUGH12 0xA1
#define SCSI_ATA_COMMAND_PASS_THROUGH16 0x85
#define SCSI_SERVICE_ACTION_IN12        0xAB
#define SCSI_SERVICE_ACTION_IN16        0x9E
#define SCSI_SERVICE_ACTION_OUT12       0xA9
#define SCSI_SERVICE_ACTION_OUT16       0x9F
#define SCSI_REPORT_ID_INFO             0xA3

#endif
