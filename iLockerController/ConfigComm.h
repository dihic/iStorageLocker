#ifndef _CONFIG_COMM_H
#define _CONFIG_COMM_H

#include "ISerialComm.h"
#include "FastDelegate.h"

#include <boost/smart_ptr.hpp>
#include <boost/make_shared.hpp>
#include <Driver_USART.h>


using namespace fastdelegate;

class ConfigComm : public ISerialComm
{
	protected:
		const ARM_DRIVER_USART &uart;
		static void SignalEventHandler(uint32_t event);
		static const std::uint8_t dataHeader[5];
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
		
		uint8_t data[0x400];
		StateType dataState;
		uint8_t command;
		uint8_t checksum;
		uint8_t lenIndex;
		uint16_t parameterLen;
		uint16_t parameterIndex;
		boost::shared_ptr<uint8_t[]> parameters;
		uint32_t base;
	  uint8_t *dataOffset;
	public:
		typedef FastDelegate3<std::uint8_t, std::uint8_t *, std::size_t> CommandArrivalHandler;
		CommandArrivalHandler OnCommandArrivalEvent;
		CommandArrivalHandler OnFileSystemCommandArrivalEvent;
		ConfigComm(ARM_DRIVER_USART &u);
		virtual ~ConfigComm();
		virtual void Start();
		virtual void Stop();
		virtual void DataReceiver();
		virtual bool SendData(const std::uint8_t *data,std::size_t len);
		virtual bool SendData(std::uint8_t command, const std::uint8_t *data,std::size_t len);
		bool SendFSData(uint8_t command,const uint8_t *data, size_t len);
};

#endif

