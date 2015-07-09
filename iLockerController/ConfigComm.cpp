#include "ConfigComm.h"

using namespace std;

const std::uint8_t ConfigComm::dataHeader[5] = {0xfe, 0xfc, 0xfd, 0xfa, 0xfb};

boost::shared_ptr<ConfigComm> ConfigComm::instance;

boost::shared_ptr<ConfigComm> &ConfigComm::CreateInstance(ARM_DRIVER_USART &u)
{
	if (instance == nullptr || &instance->uart != &u)
		instance.reset(new ConfigComm(u));
	return instance;
}

extern "C"
{
	void ConfigComm::UARTCallback(uint32_t event)
	{
		switch (event)    
		{
			case ARM_USART_EVENT_RECEIVE_COMPLETE:
			case ARM_USART_EVENT_TRANSFER_COMPLETE:
				instance->tail = 0;
				instance->uart.Receive(instance->buffer.get(), instance->bufferSize);
				break;
			case ARM_USART_EVENT_SEND_COMPLETE:    
			case ARM_USART_EVENT_TX_COMPLETE:      
				break;     
			case ARM_USART_EVENT_RX_TIMEOUT:         
				break;     
			case ARM_USART_EVENT_RX_OVERFLOW:
				instance->dataState = StateDelimiter1;
				break;
			case ARM_USART_EVENT_TX_UNDERFLOW:        
				break;    
		}
	}
}

ConfigComm::ConfigComm(ARM_DRIVER_USART &u)
	:UartComm(u, UARTCallback, 115200, CONFIG_BUFFER_SIZE)
{
}

ConfigComm::~ConfigComm()
{
}

void ConfigComm::DataProcess(std::uint8_t byte)
{
	switch (dataState)
	{
		case StateDelimiter1:
			if (byte==dataHeader[0])
				dataState = StateDelimiter2;
			break;
		case StateDelimiter2:
			if (byte == dataHeader[1])
				dataState = StateCommand;
			else if (byte == dataHeader[3])
			{
				checksum = dataHeader[0] + dataHeader[3];
				dataState = StateFileCommand;
			}
			else
				dataState = StateDelimiter1;
			break;
		case StateCommand:
			command = byte;
			dataState = StateParameterLength;
			break;
		case StateParameterLength:
			parameterLen = byte;
			if (parameterLen == 0)
			{
				parameters.reset();
				dataState = StateDelimiter1;
				if (OnCommandArrivalEvent)
					OnCommandArrivalEvent(command, NULL, 0);
			}
			else
			{
				parameters = boost::make_shared<uint8_t[]>(parameterLen);
				parameterIndex = 0;
				dataState = StateParameters;
			}
			break;
		case StateParameters:
			parameters[parameterIndex++] = byte;
			if (parameterIndex >= parameterLen)
			{
				if (OnCommandArrivalEvent)
					OnCommandArrivalEvent(command,parameters.get(),parameterLen);
				parameters.reset();
				dataState = StateDelimiter1;
			}
			break;
		case StateFileCommand:
			checksum += (command = byte);
			lenIndex = 0;
			dataState = StateFileDataLength;
			parameterLen = 0;
			break;
		case StateFileDataLength:
			checksum += byte;
			parameterLen |= (byte<<(lenIndex<<3));
			if (++lenIndex>1)
			{
				if (parameterLen == 0)
				{
					parameters.reset();
					dataState = StateChecksum;
				}
				else
				{
					parameters = boost::make_shared<uint8_t[]>(parameterLen);
					parameterIndex = 0;
					dataState = StateFileData;
				}
			}
			break;
		case StateFileData:
			checksum += (parameters[parameterIndex++] = byte);
			if (parameterIndex >= parameterLen)
				dataState = StateChecksum;
			break;
		case StateChecksum:
			if ((checksum == byte) && OnFileCommandArrivalEvent)
					OnFileCommandArrivalEvent(command,parameters.get(),parameterLen);
			parameters.reset();
			dataState = StateDelimiter1;
			return;
		default:
			break;
	}
}

bool ConfigComm::SendData(const uint8_t *data, size_t len)
{
	uint8_t pre[3]={ dataHeader[0],dataHeader[2], static_cast<uint8_t>(len) };
	Sync();
	uart.Send(pre,3);
	Sync();
	uart.Send(data,len);
	Sync();
	return true;
}

bool ConfigComm::SendData(uint8_t command,const uint8_t *data,size_t len)
{
	uint8_t pre[4]={ dataHeader[0], dataHeader[2], command, static_cast<uint8_t>(len) };
	Sync();
	uart.Send(pre,4);
	if (len>0)
	{
		Sync();
		uart.Send(data,len);
	}
	Sync();
	return true;
}

bool ConfigComm::SendFileData(uint8_t command,const uint8_t *data, size_t len)
{
	if (len>0xffff)
		return false;
	uint8_t pre[5]={ dataHeader[0], dataHeader[4], command, 
									 static_cast<uint8_t>(len&0xff), static_cast<uint8_t>((len>>8)&0xff) };
	uint8_t checksum = 0;
	for(int i=0;i<5;i++)
		checksum += pre[i];
	for(int i=0;i<len;i++)
		checksum += data[i];
	Sync();
	uart.Send(pre, 5);
	if (len>0)
	{
		Sync();
		uart.Send(data, len);
	}
	Sync();
	uart.Send(&checksum, 1);
	Sync();
	return true;
}
