#include <cstring>
#include <cstdint>

#include "s25fl064.h"

using namespace std;

namespace Spansion
{
	void Flash::AccEnable(bool en)
	{
		//HAL_GPIO_WritePin(ACC_PIN, en?GPIO_PIN_SET:GPIO_PIN_RESET);
	}
	
	Flash::Flash(ARM_DRIVER_SPI *spi, GPIO_TypeDef *cs_port, uint16_t cs_pin, bool accen)
		:driver(spi), csPort(cs_port), csPin(cs_pin), acc(accen), writing(false)
	{
		driver->Initialize(NULL);
		driver->PowerControl(ARM_POWER_FULL);
		
		GPIO_InitTypeDef gpioType = { csPin, 
																	GPIO_MODE_OUTPUT_PP,
																	GPIO_PULLUP,
																	GPIO_SPEED_HIGH,
																	0 };
		
		HAL_GPIO_Init(csPort, &gpioType);			
		HAL_GPIO_WritePin(csPort, csPin, GPIO_PIN_SET);
																	
		driver->Control(ARM_SPI_MODE_MASTER |
										ARM_SPI_CPOL0_CPHA0 |
										ARM_SPI_DATA_BITS(8) |
										ARM_SPI_MSB_LSB |
										ARM_SPI_SS_MASTER_UNUSED,
										25000000);
																	
		//cout<<"SPI Speed: "<<driver->Control(ARM_SPI_GET_BUS_SPEED,0)<<endl;
	}
	
	bool Flash::IsAvailable()
	{
		static const uint8_t request[] = {COMMAND_READ_ID, 0, 0, 0, 0, 0};
		uint8_t *response=new uint8_t[6];
		HAL_GPIO_WritePin(csPort, csPin, GPIO_PIN_RESET);
		driver->Transfer(request,response,6);
		SpiSync();
		HAL_GPIO_WritePin(csPort, csPin, GPIO_PIN_SET);
		bool result = response[4]==0x01 && response[5]==0x16;
		delete[] response;
		return result;
	}
	
	void Flash::FastReadMemory(uint32_t address, uint8_t *data, uint32_t length)
	{
		WaitForWriting();
		address &= MAX_ADDRESS;
		uint8_t *command = new uint8_t[5];
		command[0] = COMMAND_FAST_READ;
		command[1] = (address>>16) & 0xff;
		command[2] = (address>>8) & 0xff;
		command[3] = address & 0xff;
		command[4] = 0;
		HAL_GPIO_WritePin(csPort, csPin, GPIO_PIN_RESET);
		driver->Send(command, 5);
		SpiSync();
		driver->Receive(data, length);
		SpiSync();
		HAL_GPIO_WritePin(csPort, csPin, GPIO_PIN_SET);
		delete[] command;
	}
	
  void Flash::SpiSync()
	{
		ARM_SPI_STATUS status;
		do
		{
			status = driver->GetStatus();
		} while (status.busy);
	}
	
	void Flash::WriteAccess(bool enable)
	{
		static const uint8_t command_wr[] = { COMMAND_WREN, COMMAND_WRDI};
		HAL_GPIO_WritePin(csPort, csPin, GPIO_PIN_RESET);
		if (enable)
			driver->Send(command_wr, 1);
		else
			driver->Send(command_wr+1, 1);
		SpiSync();
		HAL_GPIO_WritePin(csPort, csPin, GPIO_PIN_SET);
	}

	uint8_t Flash::ReadStatus()
	{
		uint8_t *buf = new uint8_t[4];
		buf[0] = COMMAND_RDSR;
		HAL_GPIO_WritePin(csPort, csPin, GPIO_PIN_RESET);
		driver->Transfer(buf, buf+2, 2);
		SpiSync();
		HAL_GPIO_WritePin(csPort, csPin, GPIO_PIN_SET);
		uint8_t result = buf[3];
		delete[] buf;
		return result;
	}
//	
//	void ReadMemory(uint32_t address,uint8_t *data,uint32_t length)
//	{
//		address&=MAX_ADDRESS;
//		uint8_t command[4]={COMMAND_READ,address>>16,(address&0xff00)>>8,address&0xff};
//		SSP_Init(FLASH_SPI,0,FLASH_CS_PIN);
//		SSP_BeginTransaction(FLASH_SPI);
//		SSP_Write(FLASH_SPI,command,4);
//		SSP_Read(FLASH_SPI,data,length);
//		SSP_EndTransaction(FLASH_SPI);
//	}
//	
	uint8_t Flash::WaitForWriting()
	{
		if (!writing)
			return 0;
		static const uint8_t command = COMMAND_RDSR;
		uint8_t *status = new uint8_t;
		if (acc)
			AccEnable(true);
		HAL_GPIO_WritePin(csPort, csPin, GPIO_PIN_RESET);
		driver->Send(&command, 1);
		SpiSync();
		do
		{
			driver->Receive(status, 1);
			SpiSync();
		} while (*status & 0x01);
		uint8_t result = *status;
		delete status;
		HAL_GPIO_WritePin(csPort, csPin, GPIO_PIN_SET);
		if (acc)
			AccEnable(false);
		writing = false;
		return result;
	}
	
	uint8_t Flash::BulkErase()
	{
		WaitForWriting();
		static const uint8_t val = COMMAND_BE;
		WriteAccess(true);
		HAL_GPIO_WritePin(csPort, csPin, GPIO_PIN_RESET);
		driver->Send(&val, 1);
		SpiSync();
		HAL_GPIO_WritePin(csPort, csPin, GPIO_PIN_SET);
		writing = true;
		return 0;
	}
	
	uint8_t Flash::SectorErase(uint8_t sector)
	{
		WaitForWriting();
		uint8_t *buf = new uint8_t[4];
		buf[0] = COMMAND_SE;
		buf[1] = sector;
		buf[2] = buf[3] = 0;
		WriteAccess(true);
		HAL_GPIO_WritePin(csPort, csPin, GPIO_PIN_RESET);
		driver->Send(buf, 4);
		SpiSync();
		HAL_GPIO_WritePin(csPort, csPin, GPIO_PIN_SET);
		writing = true;
		delete[] buf;
		return 0;
	}
	
	uint8_t	Flash::ParameterSectorErase(uint16_t sector)
	{
		WaitForWriting();
		uint8_t *buf = new uint8_t[4];
		buf[0] = COMMAND_P4E;
		buf[1] = (sector >> 4) & 0xff;
		buf[2] = (sector & 0xf) << 4;
		buf[3] = 0;
		WriteAccess(true);
		HAL_GPIO_WritePin(csPort, csPin, GPIO_PIN_RESET);
		driver->Send(buf, 4);
		SpiSync();
		HAL_GPIO_WritePin(csPort, csPin, GPIO_PIN_SET);
		writing = true;
		delete[] buf;
		return 0;
	}
	
	uint8_t Flash::PageProgram(uint32_t address, uint8_t *data, uint32_t length)
	{
		uint8_t *buf = new uint8_t[4];
		uint8_t *temp = NULL;
		uint8_t offset = address&0xff;
		
		if (offset!=0)
		{
			temp = new uint8_t[0x100];
			memset(temp,0xff,0x100);
			if (length>0x100-offset)
				memcpy(temp+offset,data,0x100-offset);
			else
				memcpy(temp+offset,data,length);
			length+=offset;
		}
		
		buf[0]=COMMAND_PP;
		buf[3]=0;
		address &= 0x00FFFF00u;
		
		uint16_t pages=length>>8;
		uint16_t remainder=length&0xff;
		
		if (remainder)
			pages++;
		
		for(int16_t i=0; i<pages; ++i)
		{
			WaitForWriting();
			buf[1]=(address&0xff0000)>>16;
			buf[2]=(address&0x00ff00)>>8;
			WriteAccess(true);
			if (acc)
				AccEnable(true);
			HAL_GPIO_WritePin(csPort, csPin, GPIO_PIN_RESET);
			driver->Send(buf, 4);
			if (i==0 && offset!=0)
				driver->Send(temp, 0x100);
			else
				driver->Send(data+(i<<8)-offset, (i==pages-1 && remainder!=0)?remainder:0x100);
			SpiSync();
			HAL_GPIO_WritePin(csPort, csPin, GPIO_PIN_SET);
			if (acc)
				AccEnable(false);
			writing = true;
			address+=0x100;
		if (temp) delete[] temp;
		}
		delete[] buf;
		return 0;
	}
	
	uint8_t Flash::WriteMemory(uint32_t address, uint8_t *data, uint32_t length, bool erase)
	{
		if (length > 0x10000)
			return 0xff;
		uint8_t result=0;
		if (erase)
		{
			if (length >= 0x1000)
				result = SectorErase(address>>16);
			else
				result = ParameterSectorErase(address>>12);
			if (result)
				return result;
		}
		result = PageProgram(address, data, length);
		return result;
	}
}
