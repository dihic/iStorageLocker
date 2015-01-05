#ifndef _ISERIAL_COMM_H
#define _ISERIAL_COMM_H

#include <cstdint>
#include <cmsis_os.h>

class ISerialComm
{
	public:
		virtual ~ISerialComm() {}
		virtual void Start()=0;
		virtual void Stop()=0;
		virtual void DataReceiver() = 0;
		virtual bool SendData(const std::uint8_t *data,std::size_t len)=0;
		virtual bool SendData(std::uint8_t command, const std::uint8_t *data,std::size_t len)=0;
};

#endif
