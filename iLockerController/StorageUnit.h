#ifndef _STORAGE_UNIT_H
#define _STORAGE_UNIT_H

#include <string>
#include "Guid.h"
#include "CanDevice.h"

namespace IntelliStorage
{
#define SYNC_DATA				0x0100
#define SYNC_LIVE				0x01ff
	
	struct RfidData
	{
			int NodeId;
			int State;
			std::string CardId;
			std::string PresId;
	};
	
	class DeviceAttribute
	{
		private:
			DeviceAttribute() {}
		public:
			static const std::uint16_t RawData 				= 0x8006;   //R
			static const std::uint16_t Notice 				= 0x9000;   //W
			static const std::uint16_t ControlLED			= 0x9001;	
			static const std::uint16_t ControlDoor		= 0x9002;
	};
	
	class StorageUnit : public CanDevice
	{
		private:
			static string GenerateId(const uint8_t *id, size_t len);
			boost::shared_ptr<RfidData> card;
			uint8_t lastCardType;
			uint8_t lastDoorState;
		  bool blinking;
			bool cardChanged;
			bool doorChanged;
			bool dataArrival;
			
			const uint8_t *memoryData;
		public:
			static const std::uint8_t CardArrival = 0x80;
			static const std::uint8_t CardLeft    = 0x81;
		
			StorageUnit(boost::shared_ptr<CANExtended::CanEx> &ex, std::uint16_t id);
			virtual ~StorageUnit() {}
			
			void SetNotice(uint8_t level, bool force);
			void OpenDoor();
			
			void RequestRawData()
			{
				ReadAttribute(DeviceAttribute::RawData);
			}
			
			bool DataFirstArrival() const { return dataArrival; }

			bool CardChanged() const { return cardChanged; }
			bool DoorChanged()
			{ 
				bool result = doorChanged;
				doorChanged = false;
				return result; 
			}
			
			boost::shared_ptr<RfidData> &GetCard() { return card; }
			uint8_t GetDoorState() const { return lastDoorState; }
			bool IsRfid() const { return !card->CardId.empty(); }
			bool IsEmpty() const { return card->PresId.empty(); }
			
			void UpdateCard();
			
			std::string &GetPresId() { return card->PresId; }
			void SetPresId(std::string &pres);
			
			void RequestData()
			{
				auto ex = canex.lock();
				if (ex != nullptr)
					ex->Sync(DeviceId, SYNC_DATA, CANExtended::Trigger);
			}
			
			virtual void ProcessRecievedEvent(boost::shared_ptr<CANExtended::OdEntry> &entry) override;
	};
}

#endif

