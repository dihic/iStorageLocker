#include "tas5727.h"
#include "cmsis_os.h"

#define MAX_DB 0x00A8

PowerAmp::PowerAmp(ARM_DRIVER_I2C &driver)
	:i2c(driver)
{
	i2c.Initialize(NULL);
	i2c.PowerControl(ARM_POWER_FULL);
	i2c.Control(ARM_I2C_BUS_SPEED, ARM_I2C_BUS_SPEED_FAST);
	i2c.Control(ARM_I2C_BUS_CLEAR, 0);
}

bool PowerAmp::Available()
{
	uint8_t val;
	ReadRegister(0x01, &val, 1);
	return (val==0xC1);
}

void PowerAmp::Init(bool PBTL)
{
	uint8_t val;
	//uint16_t val16;
	
	WriteByte(0x1B, 0x00);	// Oscillator Trim
	osDelay(60); //wait >50ms
	ReadRegister(0x1B, &val, 1);
	
	WriteByte(0x04, 0x03);	// Set I2S 16bit word length
	
	WriteByte(0x0A, 0x30);	// DRC Volume
	WriteByte(0x0E, 0xF1);
	
	
#define DRC_SIZE 186
	
	static const uint8_t DRCdata[DRC_SIZE] = {
		//Scale
//		 5, 0x56, 0x00, 0x0C, 0xCC, 0xCC, //-20dB Post 5.23
		 5, 0x56, 0x00, 0x12, 0x14, 0x9A, //-17dB Post 5.23
//		 5, 0x57, 0x00, 0x14, 0x00, 0x00, //20dB Pre 9.17
//		 5, 0x56, 0x00, 0x04, 0x0C, 0x37, //-30dB Post 5.23
		 5, 0x57, 0x00, 0x0B, 0x3F, 0x30, //15dB Pre 9.17
//		 5, 0x57, 0x00, 0x03, 0x8E, 0x7A, //5dB Pre 9.17
		//Upper Band
		 9, 0x40, 0x02, 0x80, 0x00, 0x00, 0x02, 0x7F, 0xFF, 0xFF, //0x40, 0x04, 0x03, 0x00, 0x00, 0x04, 0x02, 0xFF, 0xFF,
		 9, 0x3B, 0x00, 0x00, 0xE2, 0x6A, 0x00, 0x7F, 0x1D, 0x96, //0x3B, 0x00, 0x03, 0x7E, 0x00, 0x00, 0x7C, 0x82, 0x00,
		 9, 0x3C, 0x00, 0x08, 0x90, 0xB4, 0xFF, 0xF7, 0x7F, 0x6A, //0x3C, 0x00, 0x01, 0xC5, 0x00, 0xFF, 0xFE, 0x3D, 0x00,
		//Lower Band
		 9, 0x40, 0x02, 0x80, 0x00, 0x00, 0x02, 0x7F, 0xFF, 0xFF,
		 9, 0x3E, 0x00, 0x00, 0xE2, 0x6A, 0x00, 0x7F, 0x1D, 0x96,
		 9, 0x3F, 0x00, 0x08, 0x90, 0xB4, 0xFF, 0xF7, 0x7F, 0x6A,
		//Out Mix
		 9, 0x51, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		 9, 0x52, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		//DRC Crossover
		21, 0x58, 0x00, 0x7C, 0x7E, 0x7B, 0x0F, 0x07, 0x03, 0x0A, 0x00, 0x7C, 0x7E, 0x7B, 0x00, 0xF8, 0xE4, 0x62, 0x0F, 0x86, 0xEA, 0x75, 
		21, 0x59, 0x00, 0x7C, 0x7E, 0x7B, 0x0F, 0x07, 0x03, 0x0A, 0x00, 0x7C, 0x7E, 0x7B, 0x00, 0xF8, 0xE4, 0x62, 0x0F, 0x86, 0xEA, 0x75,
		21, 0x5A, 0x00, 0x00, 0x0C, 0x4A, 0x00, 0x00, 0x18, 0x94, 0x00, 0x00, 0x0C, 0x4A, 0x00, 0xF8, 0xE4, 0x62, 0x0F, 0x86, 0xEA, 0x75,
		21, 0x5B, 0x00, 0x00, 0x0C, 0x4A, 0x00, 0x00, 0x18, 0x94, 0x00, 0x00, 0x0C, 0x4A, 0x00, 0xF8, 0xE4, 0x62, 0x0F, 0x86, 0xEA, 0x75,
		//Enable DRC 1&2
		 5, 0x46, 0x00, 0x00, 0x00, 0x03
	}; 

	uint8_t len;
	int index=0;
	while (index<DRC_SIZE)
	{
		len = DRCdata[index];
		WriteData(DRCdata+index+1, len);
		index += len+1;
	}
		
	//PWM Output MUX
	static const uint8_t pwmMUX[5] = {0x25, 0x01, 0x00, 0x22, 0x45}; //PBTL
	//static const uint8_t pwmMUX[5] = {0x25, 0x01, 0x02, 0x13, 0x45};
	if (PBTL)
	{
		WriteData(pwmMUX, 5);
		WriteByte(0x19, 0x3A);	//PBTL
	}
	//WriteByte(0x19, 0x30);	//BTL
	
	osDelay(200); //wait >240ms for exit shutdown
}

void PowerAmp::Start()
{
	WriteByte(0x05, 0x02);	// Start normal operation	and set fault output
	osDelay(200);
}

void PowerAmp::Shutdown()
{
	WriteByte(0x05, 0x42);	// Stop normal operation	and set fault output
	osDelay(200);
}

void PowerAmp::SetMainVolume(uint8_t val)
{
	val = (val>100)? 0: (100-val);
	uint16_t dB = 0x03ff;
	if (val<100)
		dB = MAX_DB + (val<<1);
	//Set Main Volume
	uint8_t volume[3] = {0x07, (uint8_t)(dB>>8), (uint8_t)(dB&0xff) };
	WriteData(volume, 3);
}

bool PowerAmp::ReadRegister(uint8_t addr, uint8_t *val, uint8_t size)
{
	i2c.MasterTransmit(I2CAddress, &addr, 1, true);
	while (i2c.GetStatus().busy);
	i2c.MasterReceive(I2CAddress, val, size, false);
	while (i2c.GetStatus().busy);
	return (i2c.GetDataCount () == size);
}

bool PowerAmp::WriteByte(uint8_t addr, uint8_t val)
{
	uint8_t buf[2] = { addr, val };
	i2c.MasterTransmit(I2CAddress, buf, 2, false);
	while (i2c.GetStatus().busy);
	return true;
}

bool PowerAmp::WriteData(const uint8_t *data, uint8_t len)
{
	i2c.MasterTransmit(I2CAddress, data, len, false);
	while (i2c.GetStatus().busy);
	return true;
}

