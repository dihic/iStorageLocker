//***************************************************************
//------------<02.Dec.2010 by Peter Reiser>----------------------
//***************************************************************

#ifndef _ISO14443A_H_
#define _ISO14443A_H_

//================================================================

//#include <stdio.h>							// standard input/output header
#include "trf796x.h"
#include <stdint.h>

//===============================================================

// if disabled file ISO14443A.c may be excluded from build
//#define	ENABLE14443A						// delete to disable standard

//===============================================================
#ifdef __cplusplus
extern "C" {
#endif

void Iso14443aAnticollision(uint8_t reqa);
void Iso14443aFindTag(void);
void Iso14443aLoop(uint8_t select, uint8_t nvb, uint8_t *UID);
uint8_t Iso14443aSelectCommand(uint8_t select, uint8_t *UID);

#ifdef __cplusplus
}
#endif
//===============================================================

#endif
