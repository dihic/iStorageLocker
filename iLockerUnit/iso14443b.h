//***************************************************************
//------------<02.Dec.2010 by Peter Reiser>----------------------
//***************************************************************

#ifndef _ISO14443B_H_
#define _ISO14443B_H_

//================================================================

#include <stdint.h>
#include "trf796x.h"

//===============================================================

// if disabled file ISO14443B.c may be excluded from build
#define	ENABLE14443B						// delete to disable standard

//===============================================================
#ifdef __cplusplus
extern "C" {
#endif
void iso14443bAnticollision(uint8_t command, uint8_t slots);
void Iso14443bFindTag(void);

#ifdef __cplusplus
}
#endif
	
//===============================================================

#endif
