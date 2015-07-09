#ifndef _CONFIG_COMM_H
#define _CONFIG_COMM_H

#include "UartComm.h"

using namespace fastdelegate;

#define CONFIG_BUFFER_SIZE	0x400

class ConfigComm : public UartComm
{
	protected:
		static const std::uint8_t dataHeader[5];
		static boost::shared_ptr<ConfigComm> instance;
		static void UARTCallback(uint32_t event);
		ConfigComm(ARM_DRIVER_USART &u);
	private:
		enum StateType
		{
			StateDelimiter1,
			StateDelimiter2,
			StateCommand,
			StateParameterLength,
			StateParameters,
			StateFileCommand,
			StateFileDataLength,
			StateFileData,
			StateChecksum
		};
		
		volatile StateType dataState = StateDelimiter1;
		uint8_t checksum;
		uint8_t lenIndex;
		uint8_t command;
		uint16_t parameterLen;
		uint16_t parameterIndex;
		boost::shared_ptr<uint8_t[]> parameters;
	public:
		static boost::shared_ptr<ConfigComm> &CreateInstance(ARM_DRIVER_USART &u);
		static boost::shared_ptr<ConfigComm> &Instance() { return instance; }
	
		CommandArrivalHandler OnFileCommandArrivalEvent;
		
		virtual ~ConfigComm();

		virtual void DataProcess(std::uint8_t byte) override;
		virtual bool SendData(const std::uint8_t *data,std::size_t len) override;
		virtual bool SendData(std::uint8_t command, const std::uint8_t *data,std::size_t len) override;
		bool SendFileData(uint8_t command,const uint8_t *data, size_t len);
};

#endif

