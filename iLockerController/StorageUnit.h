#ifndef _STORAGE_UNIT_H
#define _STORAGE_UNIT_H

#include <string>
#include "Guid.h"
#include "CanDevice.h"

namespace IntelliStorage
{
	
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
			const uint8_t *memoryData;
		public:
			static const std::uint8_t CardArrival = 0x80;
			static const std::uint8_t CardLeft    = 0x81;
		
			StorageUnit(CANExtended::CanEx &ex, std::uint16_t id);
			virtual ~StorageUnit() {}
			
			void SetNotice(uint8_t level, bool force);
			
			bool IsEmpty()
			{
				return ((card.get()==NULL)||(card->PresId.empty()));
			}
			
			void OpenDoor()
			{
				boost::shared_ptr<std::uint8_t[]> data = boost::make_shared<std::uint8_t[]>(1);
				data[0]=1;
				WriteAttribute(DeviceAttribute::ControlDoor, data, 1);
			}
			
			void RequestRawData()
			{
				ReadAttribute(DeviceAttribute::RawData);
			}

			bool CardChanged() const { return cardChanged; }
			bool DoorChanged()
			{ 
				bool result = doorChanged;
				doorChanged = false;
				return result; 
			}
			
			boost::shared_ptr<RfidData> &GetCard() { return card; }
			uint8_t GetDoorState() { return lastDoorState; }
			
			void UpdateCard();
			
			std::string &GetPresId() { return card->PresId; }
			void SetPresId(std::string &pres);
			
			virtual void ProcessRecievedEvent(boost::shared_ptr<CANExtended::OdEntry> entry);
	};
}

#endif

