#ifndef _TAS5727_H
#define _TAS5727_H

#include "Driver_I2C.h"                 // ::CMSIS Driver:I2C

class PowerAmp
{
	private:
		static const uint8_t I2CAddress = 0x2A; //0x54
		ARM_DRIVER_I2C &i2c;
		bool ReadRegister(uint8_t addr, uint8_t *val, uint8_t size);
		bool WriteByte(uint8_t addr, uint8_t val);
		bool WriteData(const uint8_t *data, uint8_t len);
	public:
		PowerAmp(ARM_DRIVER_I2C &driver);
		~PowerAmp() {}
		bool Available();
		void Init(bool PBTL=false);
		void Start();
		void Shutdown();
		void SetMainVolume(uint8_t val);
};

#endif
