#include "TcpClient.h"
#include <cstring>
#include <Driver_ETH_PHY.h>

#define TCP_SEGMENT_SIZE   1460

#define TCP_WINDOW_SIZE    4380

using namespace std;

#define TCP_BUFFER_NUM 16

osMailQDef(MailSendTcp   , TCP_BUFFER_NUM*2, TcpBuffer);
osMailQDef(MailReceiveTcp, TCP_BUFFER_NUM  , TcpBuffer);
osSemaphoreDef(SemaphoreSendTcp);
osSemaphoreDef(SemaphoreReceiveTcp);

extern ARM_DRIVER_ETH_PHY Driver_ETH_PHY0;

const std::uint8_t TcpClient::DataHeader[2] = {0xAA, 0x44};

map<std::int32_t,TcpClient *> TcpClient::tcp_table;

//std::uint32_t TcpClient::windowSize = TCP_WINDOW_SIZE;
 
TcpClient::TcpClient(const std::uint8_t *endpoint)
	: serverPort(endpoint[4]|(endpoint[5]<<8)),
		tcpSocket(NULL), conn(false), isr(false), ack(true)
		//threadRx_id(NULL), threadTx_id(NULL), FirstSend(false)
{
	mailSid 		 = osMailCreate(osMailQ(MailSendTcp), NULL);
	mailRid 		 = osMailCreate(osMailQ(MailReceiveTcp), NULL);
	semaphoreSid = osSemaphoreCreate(osSemaphore(SemaphoreSendTcp), TCP_BUFFER_NUM*2);
	semaphoreRid = osSemaphoreCreate(osSemaphore(SemaphoreReceiveTcp), TCP_BUFFER_NUM);
	memcpy(serverIp, endpoint, 4);
	Driver_ETH_PHY0.SetMode(ARM_ETH_PHY_SPEED_10M|ARM_ETH_PHY_DUPLEX_FULL);
	//Driver_ETH_PHY0.SetMode(ARM_ETH_PHY_AUTO_NEGOTIATE);
	osDelay(50);
	txCount = TCP_BUFFER_NUM*2;
	rxCount = TCP_BUFFER_NUM;
	Start();
}

TcpClient::~TcpClient()
{
	Stop();
	if (tcpSocket)
	{
		tcp_release_socket(tcpSocket);
		tcp_table.erase(tcpSocket);
	}
	osSemaphoreDelete(semaphoreSid);
	osSemaphoreDelete(semaphoreRid);
}

void TcpClient::ChangeServiceEndpoint(const std::uint8_t *endpoint)
{
	memcpy(serverIp,endpoint,4);
	serverPort = endpoint[4]|(endpoint[5]<<8);
	if (tcpSocket)
		tcp_abort(tcpSocket);
//	FirstSend = false;
}


void TcpClient::DataReciever(const boost::shared_ptr<uint8_t[]> &data, uint32_t len)
{
	static uint8_t dataState = 0;
	static uint32_t readIndex = 0;
	static boost::shared_ptr<std::uint8_t[]> payload;
	static size_t payloadLen =0;
	static uint8_t sumcheck = 0;

	uint32_t index = 0;
	
	while (index<len)
	{
		switch (dataState)
		{
			case 0: //Header0
				if (data[index]==DataHeader[0])
				{
					dataState = 1;
					payloadLen = 0;
					readIndex = 0;
				}
				break;
			case 1:	//Header1
				sumcheck = DataHeader[0]+DataHeader[1];
				dataState = (data[index]==DataHeader[1]) ? 2:0;
				break;
			case 2: //Length
				sumcheck += data[index];
				payloadLen |= (data[index] << (readIndex<<3));
				if (++readIndex >=4)
				{
					dataState = 3;
					readIndex = 0;
					payload = boost::make_shared<uint8_t[]>(payloadLen);
				}
				break;
			case 3: //Payload
				sumcheck += data[index];
				payload[readIndex++] = data[index];
				if (readIndex>=payloadLen)
				{
					readIndex = 0;
					dataState = 4;
				}
				break;
			case 4: //Sumcheck
				dataState = 0;
				if (data[index]==sumcheck && CommandArrivalEvent)
					CommandArrivalEvent(payload, payloadLen); //Invoke in callback
				payload.reset();
				payloadLen = 0;
				break;
		}
		++index;
	}
}

extern "C"
{
	// Notify the user application about TCP socket events.
	uint32_t TcpClient::tcp_cb_func(int32_t socket, tcpEvent event, const uint8_t *buf, uint32_t len) 
	{
		TcpClient *client = tcp_table[socket];
		if (client==NULL)
			return 0;
		client->isr = true;
		switch (event) {
			case tcpEventConnect:
				// Connect request received
				break;
			case tcpEventEstablished:
				// Connection established
#ifdef DEBUG_PRINT
				cout<<"Connected to server!"<<endl;
#endif
				client->ack = true;
				//osSignalSet(client->threadTx_id, 0xff);
				break;
			case tcpEventClosed:
				// Connection was properly closed
#ifdef DEBUG_PRINT
				cout<<"Close Tcp connection!"<<endl;
#endif
				break;
			case tcpEventAbort:
				// Connection is for some reason aborted
#ifdef DEBUG_PRINT
				cout<<"Aborted Tcp connection!"<<endl;
#endif
				break;
			case tcpEventACK:
				// Previously sent data acknowledged
#ifdef DEBUG_PRINT
				cout<<"ACK accepted!"<<endl;
#endif
				client->ack = true;
//				client->FirstSend = true;
//				osSignalSet(client->threadTx_id, 0xff);
				break;
			case tcpEventData:
				// Data received
				if (client->rxCount <=0)
					break;
				if (osSemaphoreWait	(client->semaphoreRid, 0) <= 0)
					break;
				__sync_fetch_and_sub(&(client->rxCount), 1);
#ifdef DEBUG_PRINT
				cout<<"RX Alloc "<<client->rxCount<<endl;
#endif
				TcpBuffer *mptr = reinterpret_cast<TcpBuffer *>(osMailAlloc(client->mailRid, 0)); 
				if (mptr)
				{
					mptr->data = boost::make_shared<uint8_t[]>(len);
					mptr->len = len;
					memcpy(mptr->data.get(), buf, len);
					osMailPut(client->mailRid, mptr);
				}
//				windowSize -= len;
//				if (windowSize < TCP_SEGMENT_SIZE)
//				{
//					tcp_reset_window(socket);
//					windowSize = TCP_WINDOW_SIZE;
//				}
				break;
		}
		client->isr = false;
		return 0;
	}
}

void TcpClient::DataReceiver()
{
	osEvent evt = osMailGet(mailRid, 0);        // if any mail to receive
	if (evt.status != osEventMail)
		return;
	TcpBuffer *rbuf = reinterpret_cast<TcpBuffer *>(evt.value.p);
	DataReciever(rbuf->data, rbuf->len);
	rbuf->data.reset();
	osMailFree(mailRid, rbuf);
	osSemaphoreRelease(semaphoreRid);
	__sync_fetch_and_add(&rxCount, 1);
#ifdef DEBUG_PRINT
	cout<<"RX Release "<<rxCount<<endl;
#endif
}

void TcpClient::TxProcessor()
{
	if (!ack && !tcp_check_send(tcpSocket))
		return;
	osEvent evt = osMailGet(mailSid, 0);        // wait for mail to send
	if (evt.status != osEventMail)
		return;
	TcpBuffer *	buffer = reinterpret_cast<TcpBuffer *>(evt.value.p);
	boost::shared_ptr<std::uint8_t[]> bufferData = buffer->data;
	std::size_t bufferLen = buffer->len;
	osMailFree(mailSid, buffer);	// free memory allocated for mail
	buffer = NULL;
	osSemaphoreRelease(semaphoreSid);
	__sync_fetch_and_add(&txCount, 1);
#ifdef DEBUG_PRINT
	cout<<"TX Release "<<txCount<<endl;
#endif
	if (bufferLen >  tcp_max_data_size(tcpSocket))
		return;
	
	uint8_t *sendbuf = tcp_get_buf(bufferLen);
	memcpy(sendbuf, bufferData.get(), bufferLen);	
	while (!tcp_check_send(tcpSocket)) 
		osDelay(5);
	netStatus st = tcp_send(tcpSocket, sendbuf, bufferLen); //send buffer to Tcp socket
	ack = false;
#ifdef DEBUG_PRINT
	cout<<"Send Tcp packet. status="<<(int)st<<endl;
#endif
//	evt = osSignalWait(0xff, 0);
//	if (evt.status != osEventSignal)  //send fail
//	{
//		//if send succussful before, it means connection lost and need to abort socket
//		if (FirstSend)
//		{
//			tcp_abort(tcpSocket);
//			FirstSend = false;
//		}
//	}
}

void TcpClient::TcpProcessor(void const *argument) 
{
	TcpClient &client= *reinterpret_cast<TcpClient *>(const_cast<void *>(argument));
	
//	if (client.threadTx_id==NULL)
//		client.threadTx_id = osThreadGetId();
	
	//Make sure Ethernet get linked
	if (Driver_ETH_PHY0.GetLinkState() == ARM_ETH_LINK_DOWN)
	{
		if (client.tcpSocket && client.conn)
		{
			client.conn = false;
			client.AbortConnection();
		}
		return;
	}
	
	//Make sure server exists
//	if (arp_cache_ip(NETIF_ETH, client.serverIp, arpCacheFixedIP) != netOK)
//	{
//		client.conn = false;
//		return;
//	}
	
	if (client.tcpSocket==NULL)
	{
		// Allocate a free TCP socket.
		client.tcpSocket = tcp_get_socket (TCP_TYPE_CLIENT, // | TCP_TYPE_KEEP_ALIVE, //| TCP_TYPE_FLOW_CTRL,
																			 0, 120, TcpClient::tcp_cb_func);
		tcp_table[client.tcpSocket] = &client;
		client.conn = false;
		return;
	}
	
	uint8_t tcpState = tcp_get_state(client.tcpSocket);
	
	switch (tcpState)
	{
		case tcpStateUNUSED:
		case tcpStateCLOSED:
			tcp_connect (client.tcpSocket, client.serverIp, client.serverPort, 0); // Connect to TCP server
			client.conn = false;
			break;
		case tcpStateESTABLISHED:
			client.conn = true;
			if (!client.isr)
			{
				client.TxProcessor();
				client.DataReceiver();
			}
			break;
		default:
			break;
	}
}

void TcpClient::Start()
{
}

void TcpClient::Stop()
{
	if (tcpSocket)
	{
		tcp_close(tcpSocket);
	}
}
 
// Send the data to TCP client
bool TcpClient::SendData(const uint8_t *buf, size_t len)
{
	if (!IsConnected())
		return false;

	return true;
}

void TcpClient::AbortConnection()
{
#ifdef DEBUG_PRINT
	cout<<"Connection abort!"<<endl;
#endif
	osEvent evt = osMailGet(mailSid, 0);        // wait for mail to send
	while (evt.status == osEventMail)
	{
		osMailFree(mailSid, evt.value.p);	// free memory allocated for mail
		osSemaphoreRelease(semaphoreSid);
		evt = osMailGet(mailSid, 0);
	}
	txCount = TCP_BUFFER_NUM*2;
#ifdef DEBUG_PRINT
	cout<<"TX all Released "<<txCount<<endl;
#endif
	if (tcpSocket)
	{
		conn = false;
		// This TCP connection needs to close immediately
		tcp_abort (tcpSocket);
//		// Socket will not be needed any more 
		tcp_release_socket (tcpSocket);
		tcp_table.erase(tcpSocket);
		tcpSocket = NULL;
	}
}

bool TcpClient::SendData(uint8_t command, const uint8_t *buf, size_t len)
{
	if (!IsConnected())
		return false;
	
	++len;
	size_t payloadLen = len+7;
	boost::shared_ptr<uint8_t[]> payload = boost::make_shared<uint8_t[]>(payloadLen);
	
	uint8_t sum = 0;
	payload[0] = DataHeader[0];
	payload[1] = DataHeader[1];
	payload[2] = len & 0xff;
	payload[3] = (len>>8) & 0xff;
	payload[4] = (len>>16) & 0xff;
	payload[5] = (len>>24) & 0xff;
	payload[6] = command;
	for (int i=0;i<7;++i)
		sum += payload[i];
	for (int i=0;i<len-1;++i)
		sum += buf[i];
	memcpy(payload.get()+7, buf, len-1);
	payload[len+6] = sum;
	
	//Segment payload into pack up to 1024 bytes
	int segment = payloadLen >> 10;
	int rem = payloadLen & 0x3ff;
	if (rem > 0) ++segment;
	
	TcpBuffer *mptr;
	if (segment == 1)
	{
		if (txCount > 0)	//if out of resource, reset Tcp connection
			__sync_fetch_and_sub(&txCount, 1);
		if (osSemaphoreWait(semaphoreSid, 0) <= 0)
		{
#ifdef DEBUG_PRINT
			cout<<"Out of resources!"<<endl;
#endif
			AbortConnection();
			return false;
		}
#ifdef DEBUG_PRINT
		cout<<"TX Alloc "<<txCount<<endl;
#endif
		mptr = reinterpret_cast<TcpBuffer *>(osMailAlloc(mailSid, osWaitForever)); // Allocate memory
		mptr->data = payload;
		mptr->len = payloadLen;
		osMailPut(mailSid, mptr);
	}
	else
		for(int i=0; i<segment; ++i)
		{
			if (txCount > 0)	//if out of resource, reset Tcp connection
				__sync_fetch_and_sub(&txCount, 1);
			if (osSemaphoreWait(semaphoreSid, 0) <= 0)
			{
#ifdef DEBUG_PRINT
				cout<<"Out of resources!"<<endl;
#endif
				AbortConnection();
				return false;
			}
			int segmentLen = (i<segment-1 || rem==0) ? 1024 : rem;
#ifdef DEBUG_PRINT
			cout<<"TX Alloc "<<txCount<<endl;
#endif
			mptr = reinterpret_cast<TcpBuffer *>(osMailAlloc(mailSid, osWaitForever)); // Allocate memory
			mptr->data = boost::make_shared<uint8_t[]>(segmentLen);
			mptr->len = segmentLen;
			memcpy(mptr->data.get(), payload.get()+(i<<10), segmentLen);
			osMailPut(mailSid, mptr);
		}	
	return true;
}

