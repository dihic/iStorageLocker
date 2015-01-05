#include "CanEx.h"
#include <stm32f4xx.h> 

using namespace std;

#define CAN_SEND_TIMEOUT	100

namespace CANExtended
{
	//osMutexDef(CanbusMutex);
	
	CanEx::CanEx(ARM_DRIVER_CAN &bus, uint16_t id) : canBus(bus), DeviceId(id) 
	{
		//mutex_id = osMutexCreate(osMutex(CanbusMutex));
		canBus.Initialize(CAN_500Kbps);
		canBus.SetRxObject(1,    id , DATA_TYPE | EXTENDED_TYPE, 0xfffu);  /* Enable reception  */
		canBus.SetRxObject(2, 0x000 , DATA_TYPE | EXTENDED_TYPE, 0xfffu);  /* Enable reception  */
																															
		canBus.Start();                      /* Start controller 1                  */
	}
	
	CanEx::~CanEx()
	{
//		osMutexDelete(mutex_id);
	}

//extern "C"
//{
//	void CanEx::MessageReceiverAdapter(void const *argument)  
//	{
//		CanEx &comm= *const_cast<CanEx *>(reinterpret_cast<const CanEx *>(argument));
//		CAN_msg msg;
//		for(;;)
//		{
//			if (comm.canBus.Receive(&msg, 10) == CAN_OK)  
//				comm.MessageReceiver(msg);
//			//osThreadYield();
//			//osDelay(100);
//		}
//	}
//}

	void CanEx::Poll()
	{
		CAN_msg msg;
		if (canBus.Receive(&msg, 1) == CAN_OK)  
			MessageReceiver(msg);
	}
	
	void CanEx::Transmit(uint32_t command, std::uint16_t targetId, OdEntry &entry)
	{
		CAN_ERROR result;
		
		CAN_msg msg;
		msg.id = command | ((DeviceId & 0xfff)<<12) | (targetId & 0xfff);
		msg.format = 1;
		msg.type = 0;
		
		msg.data[0] = entry.Index & 0xff;
		msg.data[1] = entry.Index >> 8;
		msg.data[2] = entry.SubIndex;
		
		uint8_t entryLen = entry.GetLen();
		uint8_t *val = entry.GetVal().get();
		
		msg.data[3] = entryLen;
		
		if (entryLen<=4)
		{
			msg.len = 4 + entryLen;
			memcpy(msg.data+4, val, entryLen);
			//osMutexWait(mutex_id, osWaitForever);
			result = canBus.Send(&msg, CAN_SEND_TIMEOUT);
			if (result)
				cout<<"CAN tranmit error: "<<result<<endl;
			//osMutexRelease(mutex_id);
			return;
		}
		
		msg.len = 8;
		memcpy(msg.data+4, val, 4);
		int segment = (entryLen-4) >> 3;
		int remainder = (entryLen-4) & 7;
		int offset = 4;
		if (remainder != 0)
			++segment;
		//osMutexWait(mutex_id, osWaitForever);
		result = canBus.Send(&msg, CAN_SEND_TIMEOUT);
		if (result)
			cout<<"CAN tranmit error: "<<result<<endl;
		//osMutexRelease(mutex_id);
		for(int i=0; i<segment; ++i)
		{
			msg.id = command | Command::Extended | ((DeviceId & 0xfff)<<12) | (targetId & 0xfff);
			msg.format = EXTENDED_FORMAT;
			msg.type = DATA_FRAME;
			msg.len = (i < segment - 1 || remainder == 0) ? 8 : remainder;
			memcpy(msg.data, val+offset, msg.len);
			offset += msg.len;
			//osMutexWait(mutex_id, osWaitForever);
			result = canBus.Send(&msg, CAN_SEND_TIMEOUT);
			if (result)
				cout<<"CAN tranmit error: "<<result<<endl;
			//osMutexRelease(mutex_id);
		}
	}
	
	void CanEx::MessageReceiver(CAN_msg &msgReceived)
	{
		uint16_t targetId = msgReceived.id & 0xfff;
    if (targetId != DeviceId && targetId != 0)
			return;
    uint16_t sourceId = (msgReceived.id >> 12) & 0xfff;
    uint32_t command = msgReceived.id & 0x7000000;
    bool isExt = ((msgReceived.id & Command::Extended) != 0);
		
		boost::shared_ptr<OdEntry> rxEntry;
		uint8_t *rxData;
		uint8_t rxLen;
		
		std::map<std::uint16_t, RxStruct>::iterator it;
		
    switch (command)
    {
			case Command::Request:
			case Command::Response:
			case Command::Broadcast:
				if (!isExt)
				{
					if (msgReceived.len >= 4)
					{
						rxEntry.reset( new OdEntry((msgReceived.data[1] << 8) | msgReceived.data[0],
																	 msgReceived.data[2], 
																	 msgReceived.data[3]));
						rxLen = rxEntry->GetLen();
						rxData = rxEntry->GetVal().get();
						if (rxLen <= 4)
						{
							memcpy(rxData, msgReceived.data+4, rxLen);
							switch (command)
							{
									case Command::Request:
											//ServiceAck(sourceId, rxEntry);
											break;
									case Command::Response:
											OnResponseRecieved(sourceId, rxEntry);
											break;
									case Command::Broadcast:
											OnProcessRecieved(sourceId, rxEntry);
											break;
									default:
										break;
							}
						}
						else
						{
							memcpy(rxData, msgReceived.data+4, 4);
							RxStruct rx;
							rx.Entry = rxEntry;
							rx.Index = 4;
							it = rxTable.find(sourceId);
							if (it == rxTable.end())
								rxTable.insert(pair<std::uint16_t, RxStruct>(sourceId, rx));
							else
								it->second = rx;
						}
					}
				}
				else
				{
					it = rxTable.find(sourceId);
					if (it== rxTable.end())
						return;
					RxStruct &rx = it->second;
					rxLen = rx.Entry->GetLen();
					rxData = rx.Entry->GetVal().get();
					if (rx.Index + msgReceived.len <= rxLen)
						memcpy(rxData + rx.Index, msgReceived.data, msgReceived.len);
					else
					{
						rxTable.erase(it);
						return;
					}
					
					rx.Index += msgReceived.len;
					if (rx.Index >= rxLen)
					{
						switch (command)
						{
								case Command::Request:
										//ServiceAck(sourceId, rx.Entry);
										break;
								case Command::Response:
										OnResponseRecieved(sourceId, rx.Entry);
										break;
								case Command::Broadcast:
										OnProcessRecieved(sourceId, rx.Entry);
										break;
								default:
									break;
						}
						rxTable.erase(it);
					}
				}
				break;
			case Command::Sync:
//				SyncReceived((ushort) ((msgReceived.data[1] << 8) | msgReceived.data[0]),
//						(SyncMode) msgReceived.data[3]);
				break;
			case Command::Heartbeat:
				if (HeartbeatArrivalEvent)
						HeartbeatArrivalEvent(sourceId, (DeviceState) msgReceived.data[0]);
				break;
      default:
        break;
		}
	}
	
	void CanEx::OnResponseRecieved(uint16_t sourceId, boost::shared_ptr<OdEntry> &entry)
	{
		if (DeviceNetwork.size()==0)
			return;
		std::map<std::uint16_t, boost::shared_ptr<ICanDevice> >::iterator it = DeviceNetwork.find(sourceId);
		if (it != DeviceNetwork.end())
			(it->second)->ResponseRecievedEvent(entry);
	}
	
	void CanEx::OnProcessRecieved(uint16_t sourceId, boost::shared_ptr<OdEntry> &entry)
	{
		if (DeviceNetwork.size()==0)
			return;
		std::map<std::uint16_t, boost::shared_ptr<ICanDevice> >::iterator it = DeviceNetwork.find(sourceId);
		if (it != DeviceNetwork.end())
			(it->second)->ProcessRecievedEvent(entry);
	}
	
	void CanEx::Request(std::uint16_t serviceId, OdEntry &entry)
	{
		Transmit(Command::Request, serviceId, entry);
	}
	
	void CanEx::Response(std::uint16_t clientId, OdEntry &entry)
	{
		Transmit(Command::Response, clientId, entry);
	}
	
	void CanEx::Broadcast(OdEntry &entry)
	{
		Transmit(Command::Broadcast, 0, entry);
	}
	
	void CanEx::SyncAll(std::uint16_t index, SyncMode mode)
	{
		Sync(0, index, mode);
	}
  
	void CanEx::Sync(std::uint16_t syncId, std::uint16_t index, SyncMode mode)
	{
		syncId &= 0xfff;
		CAN_msg msg;
		msg.id = Command::Sync | (DeviceId<<12) | syncId;
		msg.format = EXTENDED_FORMAT;
		msg.type = DATA_FRAME;
		msg.len = 4;
		msg.data[0] = index & 0xff;
		msg.data[1] = index >> 8;
		msg.data[2] = 0;
		msg.data[3] = mode;
		//osMutexWait(mutex_id, osWaitForever);
		canBus.Send(&msg, CAN_SEND_TIMEOUT);
		//osMutexRelease(mutex_id);
	}
}

