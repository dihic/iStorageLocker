#ifndef _S25FL064_H_
#define _S25FL064_H_

#include <cstdint>
#include "Driver_SPI.h"
#include "stm32f4xx_hal_gpio.h"

#define MAX_ADDRESS 0x007FFFFF

#define COMMAND_READ 0x03
#define COMMAND_FAST_READ 0x0B
#define COMMAND_DOR 0x3B
#define COMMAND_QOR 0x6B
#define COMMAND_DIOR 0xBB
#define COMMAND_QIOR 0xEB
#define COMMAND_RDID 0x9F
#define COMMAND_READ_ID 0x90

#define COMMAND_WREN 0x06
#define COMMAND_WRDI 0x04

#define COMMAND_P4E 0x20
#define COMMAND_P8E 0x40
#define COMMAND_SE 0xD8
#define COMMAND_BE 0x60

#define COMMAND_PP 0x02
#define COMMAND_QPP 0x32

#define COMMAND_RDSR 0x05
#define COMMAND_WRR 0x01
#define COMMAND_RCR 0x35
#define COMMAND_CLSR 0x30

#define COMMAND_DP 0xB9
#define COMMAND_RES 0xAB

#define COMMAND_OTPP 0x42
#define COMMAND_OTPR 0x4B

//#define ACC_PIN GPIOB, GPIO_PIN_5

using namespace std;

namespace Spansion
{
	class Flash
	{
		private:
			static void AccEnable(bool en);

			ARM_DRIVER_SPI *driver;
			GPIO_TypeDef *csPort;
			uint16_t csPin;
			bool acc;
			bool writing;
		
			void SpiSync();
			
			uint8_t ReadStatus();
			void WriteAccess(bool enable);
			uint8_t PageProgram(uint32_t address, uint8_t *data, uint32_t length);
			
		public:
			Flash(ARM_DRIVER_SPI *spi, GPIO_TypeDef *cs_port, uint16_t cs_pin, bool acc=false);
			~Flash() {}
			bool IsAvailable();
			uint8_t WaitForWriting();
			uint8_t SectorErase(uint8_t sector);
			uint8_t BulkErase();
			void FastReadMemory(uint32_t address, uint8_t *data, uint32_t length);
			uint8_t WriteMemory(uint32_t address, uint8_t *data, uint32_t length, bool erase);
	};
}

#endif
