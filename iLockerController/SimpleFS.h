#ifndef _SIMPLE_FS_H
#define _SIMPLE_FS_H

#include "s25fl064.h"
#include "ConfigComm.h"
#include <boost/smart_ptr.hpp>
#include <boost/make_shared.hpp>

class SimpleFS
{
	enum CommandType
	{
		CommandFormat 			= 0xc0,
		CommandCreateNew 		= 0xc1,
		CommandAccess 			= 0xc2,
		CommandErase 				= 0xc3,
		CommandWrite 				= 0xc4,
		CommandRead 				= 0xc5,
		CommandInfo 				= 0xcf,
	};
	private:
		boost::shared_ptr<Spansion::Flash> flash;
		//boost::shared_ptr<uint8_t []> buffer;
		bool available;
		bool formated;
		uint8_t count;
		uint8_t index;
		uint32_t currentHeader;
		uint32_t currentSize;
		ConfigComm &comm;
		void CommandArrival(std::uint8_t command,std::uint8_t *parameters,std::size_t len);
	public:
		SimpleFS(ARM_DRIVER_SPI *spi, GPIO_TypeDef *cs_port, uint16_t cs_pin, ConfigComm *u);
		~SimpleFS() {}
		bool IsAvailable() const { return available; }
		uint8_t Count() const { return count; }
		void Format();
		bool IsFormated() const { return formated; }
		uint8_t CreateNew(uint32_t size);
		uint32_t Access(uint8_t file);
		bool Erase();
		bool WriteFile(uint32_t offset, uint8_t *data, uint16_t size);
		bool ReadFile(uint32_t offset, uint8_t *data, uint16_t size);
};

#endif
