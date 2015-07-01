#ifndef _VS10XX_H
#define _VS10XX_H

#include <cstdint>
#include <string>

#include "cmsis_os.h"

#include "Driver_SPI.h"
#include "stm32f407xx.h"
#include "stm32f4xx_hal_gpio.h"

#include "FastDelegate.h"

#define SDI_END_FILL_BYTES_FLAC 12288
#define SDI_END_FILL_BYTES       2052

using namespace fastdelegate;

namespace Skewworks
{
	struct VSConfig
	{
		ARM_DRIVER_SPI *spi;
		GPIO_TypeDef *cs_port;
		uint16_t cs_pin;
		GPIO_TypeDef *dcs_port;
		uint16_t dcs_pin;
		GPIO_TypeDef *dreq_port;
		uint16_t dreq_pin;
	};
	
	class VS10XX
  {
		private:
			struct ReadDataType
			{
				VS10XX *vs;
				uint32_t offset;
				uint8_t *data;
				uint32_t size;
				osThreadId parentId;
			};
		
			enum Values
			{
				SM_RESET  = 0x0004,
				SM_CANCEL = 0x0008,
				SM_SDINEW = 0x0800,
				
				//SC_MULT_7 = 0xE000,
				VOL_MUTE = 0xFEFE
			};
			
			static const uint16_t AddressEndFillByte = 0x1e06;
			
			enum Registers
			{
				SCI_MODE = 0x00,
				SCI_STAT = 0x01,
				SCI_BASS = 0x02,
				SCI_CLOCKF = 0x03,
				SCI_DECODE_TIME = 0x04,
				SCI_AUDATA = 0x05,
				SCI_WRAM = 0x06,
				SCI_WRAMADDR = 0x07,
				SCI_HDAT0 = 0x08,
				SCI_HDAT1 = 0x09,
				SCI_VOL = 0x0B,
			};
			const VSConfig *config;
			int duration;      // Duration of file playing (in seconds)
      int position;      // Current Play Position (in seconds)
      volatile bool busy;      // Playing a file when true
      volatile bool pause;     // Paused when true
      long rawSize;
      long readSoFar;
      int bitrate;
      int mpegLayer;
      volatile bool stop;
      int lastPos;
			
			volatile bool available;
      uint8_t cmdBuffer[4];
			
			osThreadDef_t workThread;
			
			static ReadDataType dataType;
			static void PlayLoop(void const *arg);
			static void ReadWorker(void const *arg);
			
			void OnHeaderReady(int duration, int bitRate, int layer);
			void OnPlayComplete();
			void OnPositionChanged(int position);
			void OnReadData(uint32_t offset, uint8_t *data, uint32_t size, bool async=false); 
			
			void SpiSync();
			bool IsDecoding();
			uint16_t CommandRead(uint8_t address);
			void CommandWrite(uint8_t address, uint16_t data);
			void EnableI2S();
			void SendData(uint8_t *data, int len);
		public:
			typedef FastDelegate3<int,int, int> HeaderReadyCB;
			typedef FastDelegate0<> PlayCompleteCB;
			typedef FastDelegate1<int> PositionChangedCB;
			typedef FastDelegate3<uint32_t, uint8_t *, uint16_t, bool> ReadDataCB;
			HeaderReadyCB HeaderReady;
			PlayCompleteCB PlayComplete;
			PositionChangedCB PositionChanged;
			ReadDataCB ReadData;
		
			VS10XX(const VSConfig *init);
			~VS10XX() {}
			bool Play(uint32_t size);
			void Stop() { stop = true; }
			void Reset();
			int GetDurationInSeconds();
			string GetChipVersion();
			int GetVolume();
			void SetVolume(int value);
			bool IsBusy() { return busy; }
			bool Available() { return available; }
	};
}

#endif
