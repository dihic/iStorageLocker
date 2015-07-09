#include "StorageUnit.h"
#include "System.h"
#include "stm32f4xx_hal_conf.h" 
#include <cstring>

using namespace std;

namespace IntelliStorage
{

	StorageUnit::StorageUnit(boost::shared_ptr<CANExtended::CanEx> &ex, uint16_t id)
		:	CanDevice(ex, id), lastCardType(0), 
			blinking(false),cardChanged(false),doorChanged(false),dataArrival(false)
	{
		card.reset(new RfidData);
		card->NodeId = id;
		card->State = 0;
		card->CardId.clear();
		card->PresId.clear();
		memoryData = (const uint8_t *)(CARD_ADDR + ((id&0xff)<<8));
		if (memoryData[0]!=0 && memoryData[0]!=0xff)
		{
			card->PresId.append(reinterpret_cast<const char *>(memoryData+1), memoryData[0]);
			SetNotice(1, true);
		}
	}
	
	void StorageUnit::SetPresId(std::string &pres)
	{
		card->PresId = pres;
		uint8_t len = pres.length();
		uint32_t offset = CARD_ADDR + ((card->NodeId & 0xff)<<8);
		HAL_FLASH_Unlock();
		if (memoryData[0]!=0xff)
			PrepareWriteFlash(offset&0xffff, 0x100);
		HAL_FLASH_Program(TYPEPROGRAM_BYTE, offset, len);
		for(int i=0;i<len;++i)
			HAL_FLASH_Program(TYPEPROGRAM_BYTE, offset+i+1, pres[i]);
		HAL_FLASH_Lock();
	}
	
	void StorageUnit::OpenDoor()
	{
		boost::shared_ptr<std::uint8_t[]> data = boost::make_shared<std::uint8_t[]>(1);
		data[0]=1;
		WriteAttribute(DeviceAttribute::ControlDoor, data, 1);
		doorChanged = true;
	}

	void StorageUnit::UpdateCard()
	{
		cardChanged = false;
		if (card->State == CardLeft)
		{
			card->CardId.clear();
			card->PresId.clear();
		}
	}
	
	string StorageUnit::GenerateId(const uint8_t *id, size_t len)
	{
		string result;
		char temp;
		for(int i=len-1;i>=0;--i)
		{
			temp = (id[i] & 0xf0) >>4;
			temp += (temp>9 ? 55 : 48);
			result+=temp;
			temp = id[i] & 0x0f;
			temp += (temp>9 ? 55 : 48);
			result+=temp;
		}
		return result;
	}
	
	void StorageUnit::SetNotice(uint8_t level, bool force)
	{
		if (force || (blinking == false))
		{
			blinking = (level>=2);
			boost::shared_ptr<std::uint8_t[]> data = boost::make_shared<std::uint8_t[]>(1);
			data[0]=level;
			WriteAttribute(DeviceAttribute::ControlLED, data, 1);
		}
	}

	void StorageUnit::ProcessRecievedEvent(boost::shared_ptr<CANExtended::OdEntry> &entry)
	{
		dataArrival = true;
		
		std::uint8_t *rawData = entry->GetVal().get();
		uint8_t offset = (rawData[0]==2)? (10+rawData[9]) : 9;

		if (lastCardType != rawData[0])
		{
			lastCardType = rawData[0];
			cardChanged = true;
			switch (rawData[0])
			{
				case 0:
					card->State = CardLeft;
					SetNotice(0, true);
					break;
				case 1:
					break;
				case 2:
					card->State = CardArrival;
					card->CardId.clear();
					card->PresId.clear();
					
					card->CardId = GenerateId(rawData+1, 8);
					card->PresId.append(reinterpret_cast<char *>(rawData+10), rawData[9]);
					SetNotice(1, true);
					break;
				default:
					break;
			}
		}
		
		if (offset >= entry->GetLen())
			return;
		if (lastDoorState != rawData[offset])
		{
			lastDoorState = rawData[offset];
			doorChanged = true;
		}
	}
}

