#include "Audio.h"
#include "cmsis_os.h"
#include <boost/smart_ptr.hpp>
#include <boost/make_shared.hpp>
#include <iostream>

using namespace std;

#define VS_CS_PIN			GPIOC, GPIO_PIN_9
#define VS_DCS_PIN		GPIOC, GPIO_PIN_8
#define VS_DREQ_PIN		GPIOC, GPIO_PIN_7
#define VS_DRIVER 		Driver_SPI3
#define AP_DRIVER			Driver_I2C1

namespace Audio
{
	const static VSConfig vsconfig = { 
		&VS_DRIVER, 
		VS_CS_PIN, 
		VS_DCS_PIN, 
		VS_DREQ_PIN,
	};
	
	boost::shared_ptr<VS10XX> codecs;
	boost::shared_ptr<PowerAmp> powerAmp;
	
	
	VS10XX::ReadDataCB ReadDataDump;
	VS10XX::ReadDataCB &ReadDataDelegate()
	{
		if (codecs.get()==NULL)
			return ReadDataDump;
		return codecs->ReadData;
	}
	
	AccessCB Access;
	AccessCB &AccessDelegate()
	{
		return Access;
	}
	
	void AudioPlayComplete()
	{
#ifdef DEBUG_PRINT
		cout<<"Play Completed!"<<endl;
#endif
		powerAmp->SetMainVolume(0);
		powerAmp->Shutdown();
	}

	void AudioHeaderReady(int duration, int bitRate, int layer)
	{
#ifdef DEBUG_PRINT
		cout<<"Audio Ready"<<endl;
	//	cout<<"MP Layer: "<<layer<<endl;
	//	cout<<"Bitrate: "<<bitRate<<endl;
	//	cout<<"Duration: "<<duration<<" seconds"<<endl<<endl;
#endif
		powerAmp->Start();
		powerAmp->SetMainVolume(100);
		osDelay(10);
	}

	void AudioPositionChanged(int position)
	{
		//cout<<"Play on: "<<position<<endl;
	}
	
	void Setup()
	{
		codecs.reset(new VS10XX(&vsconfig));
		//codecs->ReadData.bind(fileSystem.get(), &SimpleFS::ReadFile);										
		codecs->PlayComplete.bind(&AudioPlayComplete);
		codecs->HeaderReady.bind(&AudioHeaderReady);
		codecs->PositionChanged.bind(&AudioPositionChanged);
#ifdef DEBUG_PRINT	
		cout<<"Audio Chip: "<<codecs->GetChipVersion()<<endl;
#endif
		powerAmp.reset(new PowerAmp(AP_DRIVER));
		if (powerAmp->Available())
		{
			powerAmp->Init(false);
#ifdef DEBUG_PRINT			
			cout<<"PowerAmp Available and Inited"<<endl;
#endif
		}
#ifdef DEBUG_PRINT	
		else
			cout<<"PowerAmp Unavailable"<<endl;
#endif
	}
	
	void Play(uint8_t index)
	{
		if (codecs.get()==NULL)
			return;
		if (!codecs->Available())
			return;
		if (Access == NULL)
			return;
		codecs->Stop();
		while(codecs->IsBusy());
		uint32_t audioSize = Access(index);
		if (audioSize>0)
			codecs->Play(audioSize);
	}
	
}
