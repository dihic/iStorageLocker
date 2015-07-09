#ifndef __CAN_EX_H
#define __CAN_EX_H

#include <cstdint>
#include <cstring>
#include <map>
#include <vector>
#include "Driver_CAN.h"
#include "OdEntry.h"
#include "FastDelegate.h"

using namespace fastdelegate;

namespace CANExtended
{
	class CanEx;
	
	class ICanDevice
	{
		protected:
			ICanDevice(std::uint16_t deviceId)
				: DeviceId(deviceId)
			{}
		public:
			const std::uint16_t DeviceId;
			virtual void ResponseRecievedEvent(boost::shared_ptr<OdEntry> &entry) = 0;
			virtual void ProcessRecievedEvent(boost::shared_ptr<OdEntry> &entry) = 0;
			virtual ~ICanDevice() {}
	};
	
	struct Command
	{
		static const std::uint32_t Request   = 0x1000000u;
		static const std::uint32_t Response  = 0x2000000u;
		static const std::uint32_t Broadcast = 0x3000000u;
		static const std::uint32_t Sync 		 = 0x4000000u;
		static const std::uint32_t Heartbeat = 0x7000000u;
		static const std::uint32_t Extended  = 0x8000000u;
	};
	
	enum SyncMode
	{
			Trigger  = 0,
			AutoSync = 1
	};
	
	enum DeviceState
	{
		Bootup = 0x00,
		Stopped = 0x04,
		Operational = 0x05,
		Preoperational = 0x7F
	};
	
	class RxStruct
	{
		private:
			uint32_t tag[2];
		public:
			boost::shared_ptr<OdEntry> Entry;
			
			RxStruct(boost::shared_ptr<OdEntry> &entry);
			~RxStruct() {}
			
			void SetSegment(uint8_t segmentIndex, const uint8_t *data, uint8_t len);
			bool IsComplete();
			
			RxStruct(const RxStruct &ref)
			{
				Entry = ref.Entry;
				memcpy(tag, ref.tag, 8);
			}
	};
	
	class CanEx
	{
		private:
			ARM_DRIVER_CAN &canBus;
			std::uint16_t DeviceId;
		
			void MessageReceiver(CAN_msg &msgReceived);
			
			std::map<std::uint16_t, RxStruct> rxTable;
			std::map<std::uint16_t, boost::shared_ptr<ICanDevice> > DeviceNetwork;
		
			osMutexId mutex_id;
		
			void OnResponseRecieved (std::uint16_t sourceId, boost::shared_ptr<OdEntry> &entry);
			void OnProcessRecieved  (std::uint16_t sourceId, boost::shared_ptr<OdEntry> &entry);
			
			void Transmit(std::uint32_t command,std::uint16_t targetId,OdEntry &entry);
		public:
			typedef FastDelegate3<std::uint16_t, const std::uint8_t *, std::uint8_t> HeartbeatArrivalHandler;
			HeartbeatArrivalHandler HeartbeatArrivalEvent;
			CanEx(ARM_DRIVER_CAN &bus, std::uint16_t id);
			~CanEx();
			void Poll();
//			std::uint16_t HeartbeatInterval;
//			std::uint16_t SyncInterval;
			void Request(std::uint16_t serviceId, OdEntry &entry);
			void Response(std::uint16_t clientId, OdEntry &entry);
			void Broadcast(OdEntry &entry);
			void SyncAll(std::uint16_t index, SyncMode mode);
      void Sync(std::uint16_t syncId, std::uint16_t index, SyncMode mode);
			void RegisterDevice(const boost::shared_ptr<ICanDevice> &device) { DeviceNetwork[device->DeviceId] = device; }
	};
}
#endif
