#ifndef _AUDIO_H
#define _AUDIO_H

#include "System.h"
#include "vs10xx.h"
#include "tas5727.h"
#include "FastDelegate.h"

using namespace Skewworks;

namespace Audio
{
	typedef FastDelegate1<uint8_t, uint32_t> AccessCB;
	void Setup();
	void Play(uint8_t index);
	bool IsBusy();
	VS10XX::ReadDataCB &ReadDataDelegate();
	AccessCB &AccessDelegate();
}

#endif
