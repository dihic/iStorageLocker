#ifndef _TCP_CLIENT_H
#define _TCP_CLIENT_H

#include <cstdint>
#include <map>
#include <rl_net.h>
#include <cmsis_os.h>
#include "ISerialComm.h"
#include "FastDelegate.h"
#include <boost/smart_ptr.hpp>
#include <boost/make_shared.hpp>

using namespace fastdelegate;

class TcpClient : ISerialComm
{
	private:
		std::uint8_t serverIp[4];
		std::uint16_t serverPort;
		std::int32_t tcpSocket;
		//osThreadId threadRx_id;
		//osThreadId threadTx_id;
	
		osMailQId mailRid;
		osMailQId mailSid;
		osSemaphoreId semaphoreRid;
		osSemaphoreId semaphoreSid;
		int txCount;
		int rxCount;
		//static std::uint32_t windowSize;
	
		static std::map<std::int32_t,TcpClient *> tcp_table;
		static std::uint32_t tcp_cb_func(std::int32_t socket, tcpEvent event, const std::uint8_t *buf, std::uint32_t len);
		
		void TxProcessor();
		static void RxProcessor(void const *argument); 
	
		void DataReciever(const boost::shared_ptr<uint8_t[]> &data, uint32_t len);
		void AbortConnection();
		bool conn;
		bool isr;
		bool ack;
	public:
		static const std::uint8_t DataHeader[2];
		static void TcpProcessor(void const *argument);
	
		//bool FirstSend;
		
		typedef FastDelegate2<boost::shared_ptr<std::uint8_t[]>, std::size_t> CommandArrivalHandler;
		CommandArrivalHandler CommandArrivalEvent;
		TcpClient(const std::uint8_t *endpoint);
		virtual ~TcpClient();
		virtual void Start();
		virtual void Stop();
	  virtual void DataReceiver();
		virtual bool SendData(const std::uint8_t *buf, std::size_t len);
		virtual bool SendData(std::uint8_t command, const std::uint8_t *buf, std::size_t len);
		bool IsConnected() const { return conn; }

		void ChangeServiceEndpoint(const std::uint8_t *endpoint);
};


struct TcpBuffer
{
	boost::shared_ptr<std::uint8_t[]> data;
	std::size_t len;
};



#endif
