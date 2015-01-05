//***************************************************************
//------------<02.Dec.2010 by Peter Reiser>----------------------
//***************************************************************

#ifndef _ISO15693_H_
#define _ISO15693_H_

//================================================================

#include "trf796x.h"
#include <stdint.h>


//===============================================================

#define SIXTEEN_SLOTS	0x06
#define ONE_SLOT		0x26

//===============================================================

// if disabled file ISO15693.c may be excluded from build
#define	ENABLE15693							// delete to disable standard

//===============================================================
#ifdef __cplusplus
extern "C" {
#endif
uint8_t Iso15693FindTag(void);
void Iso15693Anticollision(uint8_t *mask, uint8_t length);
uint8_t Iso15693Select(uint8_t *uid,uint8_t selected);
uint8_t Iso15693WriteSingleBlock(uint8_t block,uint8_t *uid,uint8_t *data);
uint8_t Iso15693ReadSingleBlock(uint8_t block,uint8_t needSelected,uint8_t *data);
uint8_t Iso15693ReadSingleBlockWithAddress(uint8_t block,uint8_t *uid,uint8_t *data);
uint8_t Iso15693ReadMultipleBlockWithAddress(uint8_t block,uint8_t numbers,uint8_t *uid,uint8_t *data);
void TRF796xCheckRXWaitTime(void);
#ifdef __cplusplus
}
#endif
//===============================================================

#endif
