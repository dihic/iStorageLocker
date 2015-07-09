#include "vs10xx.h"
#include <iostream>
#include <iomanip>

namespace Skewworks
{
	VS10XX::VS10XX(const VSConfig *init)
		:config(init)
	{
		workThread.pthread = PlayLoop;
		workThread.tpriority = osPriorityNormal;
		workThread.instances = 1;
		workThread.stacksize = 0;
		
		config->spi->Initialize(NULL);
		config->spi->PowerControl(ARM_POWER_FULL);
		
		GPIO_InitTypeDef gpioType = { config->cs_pin, 
																	GPIO_MODE_OUTPUT_PP,
																	GPIO_PULLUP,
																	GPIO_SPEED_LOW,
																	0 };
		
		HAL_GPIO_Init(config->cs_port, &gpioType);			
		HAL_GPIO_WritePin(config->cs_port, config->cs_pin, GPIO_PIN_SET);
																	
		gpioType.Pin = config->dcs_pin;
		HAL_GPIO_Init(config->dcs_port, &gpioType);			
		HAL_GPIO_WritePin(config->dcs_port, config->dcs_pin, GPIO_PIN_SET);
																	
		gpioType.Pin = config->dreq_pin;
		gpioType.Mode = GPIO_MODE_INPUT;
		HAL_GPIO_Init(config->dreq_port, &gpioType);
																	
		config->spi->Control(ARM_SPI_MODE_MASTER |
												ARM_SPI_CPOL0_CPHA0 |
												ARM_SPI_DATA_BITS(8) |
												ARM_SPI_MSB_LSB |
												ARM_SPI_SS_MASTER_UNUSED,
												5000000);
												
		duration = -1;      // Duration of file playing (in seconds)
    position = -1;      // Current Play Position (in seconds)
    busy = false;      // Playing a file when true
    pause = false;     // Paused when true
    rawSize = -1;
    readSoFar = -1;
    bitrate = 0;
    mpegLayer = 0;
    stop = false;
    lastPos = -1;
		
		Reset();
		uint16_t stat = CommandRead(SCI_STAT);
		stat = (stat << 8) >> 12;
		available = (stat == 4);
		if (available)
			SetVolume(100);
	}
	
	void VS10XX::OnHeaderReady(int duration, int bitRate, int layer)
	{
		if (HeaderReady != NULL)
			HeaderReady(duration, bitRate, layer);
	}

  void VS10XX::OnPlayComplete()
	{
		if (PlayComplete != NULL)
			PlayComplete();
	}

  void VS10XX::OnPositionChanged(int position)
	{
		if (PositionChanged != NULL)
			PositionChanged(position);
	}
	
	
	void VS10XX::ReadWorker(void const *arg)
	{
		ReadDataType *dataType = (ReadDataType *)arg;
		if (dataType->vs->ReadData != NULL)
			dataType->vs->ReadData(dataType->offset, dataType->data, dataType->size);
		osSignalSet(dataType->parentId, 0x01);
	}
	
	VS10XX::ReadDataType VS10XX::dataType;
	
	void VS10XX::OnReadData(uint32_t offset, uint8_t *data, uint32_t size, bool async)
	{
		if (async)
		{
			osThreadDef_t threadDef;
			threadDef.pthread = ReadWorker;
			threadDef.tpriority = osPriorityNormal;
			threadDef.instances = 1;
			threadDef.stacksize = 0;
			
			dataType.vs = this;
			dataType.offset = offset;			
			dataType.data = data;
			dataType.size = size;
			dataType.parentId = osThreadGetId();
			osThreadCreate(&threadDef, &dataType);
		}
		else
		{
			if (ReadData != NULL)
				ReadData(offset, data, size);
		}
	}		
	
	void VS10XX::PlayLoop(void const *arg)
  {
		VS10XX *vs = (VS10XX *)arg;
		if (vs == NULL)
			return;
		vs->busy =true;
		vs->stop = false;
		vs->readSoFar = 0;
		vs->lastPos = -1;
		vs->bitrate = 0;
		vs->mpegLayer =0;
		
		vs->CommandWrite(SCI_DECODE_TIME, 0);	// Reset DECODE_TIME
		
		uint8_t *current = vs->block1;
		uint8_t *backup = vs->block2;
		
		uint32_t seg = vs->rawSize / AUDIO_BLOCK_SIZE;
		uint32_t dem = vs->rawSize % AUDIO_BLOCK_SIZE;
		if (dem>0)
			++seg;
		
		if (seg == 1 && dem>0)
		{
			vs->OnReadData(0, current, dem);
			vs->SendData(current, dem);
		}
		else
		{
			vs->OnReadData(0, current, AUDIO_BLOCK_SIZE);
			uint32_t offset = 0x100;
			for(int i=1; i<seg; ++i)
			{
				if (vs->stop)
					break;
				if (i==seg)
				{
					if (dem>0)
						vs->OnReadData(offset, backup, dem, true);
				}
				else
					vs->OnReadData(offset, backup, AUDIO_BLOCK_SIZE, true);
				
				vs->SendData(current, AUDIO_BLOCK_SIZE);
				//cout<<"Offset: 0x"<<hex<<offset<<dec<<endl;
				osSignalWait (0x01, osWaitForever);
				osSignalClear(osThreadGetId(), 0x01); //Thread syncing
				offset += AUDIO_BLOCK_SIZE;
				//Swap current and backup
				if (current == vs->block1)
				{
					current = vs->block2;
					backup = vs->block1;
				}
				else
				{
					current = vs->block1;
					backup = vs->block2;
				}
			}
			if (!vs->stop)
				vs->SendData(current, (dem>0) ? dem : AUDIO_BLOCK_SIZE);
		}
		
		//Read EndFillByte
		vs->CommandWrite(SCI_WRAMADDR, AddressEndFillByte); 
		uint16_t endFillByte = vs->CommandRead(SCI_WRAM);		// What byte value to send after file
		
		//Send EndFillBytes
		memset(current, endFillByte&0xff, AUDIO_BLOCK_SIZE);
		for(int i=0; i<SDI_END_FILL_BYTES; i+=AUDIO_BLOCK_SIZE)
			vs->SendData(current, AUDIO_BLOCK_SIZE);
		
		uint16_t control = vs->CommandRead(SCI_MODE);
		vs->CommandWrite(SCI_MODE, control | SM_CANCEL);
		do
		{
			vs->SendData(current, 32);
			control = vs->CommandRead(SCI_MODE);
		} while (control & SM_CANCEL);
		
		vs->busy = false;
		vs->readSoFar = -1;
		vs->position = vs->duration;
		vs->bitrate = 0;
		vs->mpegLayer = 0;
    vs->OnPositionChanged(vs->position);
    vs->OnPlayComplete();
		
		vs->CommandWrite(SCI_MODE, SM_SDINEW);
		// Reset Variables
		vs->duration = -1;
		vs->mpegLayer = 0;
		vs->position = -1;
		vs->bitrate = 0;
		vs->rawSize = 0;
		vs->readSoFar = 0;
		vs->lastPos = -1;
   }
	
	bool VS10XX::Play(uint32_t size)
	{
		if (busy)
			return false;
		rawSize = size;
		osThreadId tid;
		do
		{
			tid = osThreadCreate(&workThread, this);
		} while (tid==NULL);
		return true;
	}
	
	void VS10XX::SpiSync()
	{
		ARM_SPI_STATUS status;
		do
		{
			status = config->spi->GetStatus();
			if (status.data_lost)
			{
				cout<<"data lost!"<<endl;
				return;
			}
		} while (status.busy);
	}
	
	/// <summary>
	/// Checks if the chip is currently decoding
	/// </summary>
	bool VS10XX::IsDecoding()
	{
		uint16_t stat = CommandRead(SCI_STAT);
		return ((stat >> 15) == 0) ? false : true;
	}
	
	int VS10XX::GetDurationInSeconds()
	{
		if (IsDecoding())
				return -1;

		// Get Layer Data
		uint16_t val = CommandRead(SCI_HDAT1);

		if (val == 0)
				return -1;

		uint8_t layer = (val >> 1) & 0x3;
		uint8_t ID = (val >> 3) & 0x3;

		// Get Bitrate
		val = CommandRead(SCI_HDAT0);
		if (val == 0)
				return -1;
		val >>= 12;

		// Get REAL Bitrate
		switch (layer)
		{
			case 0:
				mpegLayer = 0;
				return -1;
			case 1:
				mpegLayer = 3;
				switch (val)
				{
					case 1:
						bitrate = (ID == 3) ? 32 : 8;
						break;
					case 2:
						bitrate = (ID == 3) ? 40 : 16;
						break;
					case 3:
						bitrate = (ID == 3) ? 48 : 24;
						break;
					case 4:
						bitrate = (ID == 3) ? 56 : 32;
						break;
					case 5:
						bitrate = (ID == 3) ? 64 : 40;
						break;
					case 6:
						bitrate = (ID == 3) ? 80 : 48;
						break;
					case 7:
						bitrate = (ID == 3) ? 96 : 56;
						break;
					case 8:
						bitrate = (ID == 3) ? 112 : 64;
						break;
					case 9:
						bitrate = (ID == 3) ? 128 : 80;
						break;
					case 10:
						bitrate = (ID == 3) ? 160 : 96;
						break;
					case 11:
						bitrate = (ID == 3) ? 192 : 112;
						break;
					case 12:
						bitrate = (ID == 3) ? 224 : 128;
						break;
					case 13:
						bitrate = (ID == 3) ? 256 : 144;
						break;
					case 14:
						bitrate = (ID == 3) ? 320 : 16;
						break;
				}
				break;
			case 2:
				mpegLayer = 2;
				return -1;
			case 3:
				mpegLayer = 1;
				return -1;
		}
			
		int dur = (int)(((rawSize - readSoFar) * 8) / bitrate / 1000);

		OnHeaderReady(dur, bitrate, mpegLayer);
		return dur;
	}
	
	uint16_t VS10XX::CommandRead(uint8_t address)
	{
		while (HAL_GPIO_ReadPin(config->dreq_port, config->dreq_pin) == GPIO_PIN_RESET);
		HAL_GPIO_WritePin(config->cs_port, config->cs_pin, GPIO_PIN_RESET);
		uint8_t *command = new uint8_t[8];
		command[0] = 0x03;
		command[1] = address;
		command[2] = command[3] = 0;
		config->spi->Transfer(command, command+4, 4);
		SpiSync();
		HAL_GPIO_WritePin(config->cs_port, config->cs_pin, GPIO_PIN_SET);
		uint16_t temp = command[6]<<8;
		temp |= command[7];
		delete[] command;
		return temp;
	}
	
	void VS10XX::CommandWrite(uint8_t address, uint16_t data)
	{
		while (HAL_GPIO_ReadPin(config->dreq_port, config->dreq_pin) == GPIO_PIN_RESET);
		HAL_GPIO_WritePin(config->cs_port, config->cs_pin, GPIO_PIN_RESET);
		uint8_t *command = new uint8_t[4];
		command[0] = 0x02;
		command[1] = address;
		command[2] = data >> 8;
		command[3] = data & 0xff;
		config->spi->Send(command, 4);
		SpiSync();
		HAL_GPIO_WritePin(config->cs_port, config->cs_pin, GPIO_PIN_SET);
	}
	
	void VS10XX::Reset()
	{
		while (HAL_GPIO_ReadPin(config->dreq_port, config->dreq_pin) == GPIO_PIN_RESET);
		
		CommandWrite(SCI_MODE, CommandRead(SCI_MODE) | SM_RESET);
		osDelay(1);

		// Reset Variables
		duration = -1;
		mpegLayer = 0;
		position = -1;
		bitrate = 0;
		rawSize = 0;
		readSoFar = 0;
		lastPos = -1;

		while (HAL_GPIO_ReadPin(config->dreq_port, config->dreq_pin) == GPIO_PIN_RESET);
		
		CommandWrite(SCI_MODE, SM_SDINEW);
		CommandWrite(SCI_CLOCKF, 0x8800);
		EnableI2S();
		osDelay(100);
	}
	
	void VS10XX::EnableI2S()
	{
		CommandWrite(SCI_WRAMADDR, 0xC017); //Init GPIO for I2S
		CommandWrite(SCI_WRAM, 0xF0);
		CommandWrite(SCI_WRAMADDR, 0xC040); //I2S_CONFIG
		CommandWrite(SCI_WRAM, 0x0C);
	}
	
	string VS10XX::GetChipVersion()
	{
		uint16_t stat = CommandRead(SCI_STAT);
		int v = stat << 8;
		v >>= 12;
		switch (v)
		{
				case 0:
						return "VS1001";
				case 1:
						return "VS1011";
				case 2:
						return "VS1002";
				case 3:
						return "VS1003";
				case 4:
						return "VS1053";
				case 5:
						return "VS1033";
				case 7:
						return "VS1103";
				default:
						return "Unknown";
		}
	}
	
	int VS10XX::GetVolume()
	{
		return (int)((float)(VOL_MUTE - CommandRead(SCI_VOL)) / 652.78f); 
	}
	
	void VS10XX::SetVolume(int value)
	{
		value = (value < 0) ? 0: ((value > 100) ? 100 : value);
		CommandWrite(SCI_VOL, (uint16_t)(VOL_MUTE - (value * 652.78f)));
	}
	
	void VS10XX::SendData(uint8_t *data, int len)
	{
		for (int i = 0; i < len; i += 32)
		{
			// Update Information
			if (duration == -1)
				duration = GetDurationInSeconds();
			position = CommandRead(SCI_DECODE_TIME);

			if (position != lastPos)
			{
				OnPositionChanged(position);
				lastPos = position;
			}

			while (HAL_GPIO_ReadPin(config->dreq_port, config->dreq_pin) == GPIO_PIN_RESET);
//				osDelay(5);    // wait till done
			//enable data config
			HAL_GPIO_WritePin(config->dcs_port, config->dcs_pin, GPIO_PIN_RESET);
			if (i+32<=len)
			{
				config->spi->Send(data+i, 32);
				readSoFar += 32;	 // We need to know how many bytes we've read to calculate header size to get duration
			}
			else
			{
				config->spi->Send(data+i, len % 32);
				readSoFar += len % 32;
			}   
			SpiSync();
			HAL_GPIO_WritePin(config->dcs_port, config->dcs_pin, GPIO_PIN_SET);
			while (pause)
				osThreadYield();    // Wait while paused
		}
	}
}
