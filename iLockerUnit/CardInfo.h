#ifndef _CARDINFO_H_
#define _CARDINFO_H_

#include <cstdint>

//enum CardType
//{
//	CardTypeNone,
//	CardTypeRaw,
//	CardTypeFormated,
//};

#define PRIORITY_NUM	3
#define NOTICE_NUM		3

class CardInfo
{
	private:
		const static std::uint8_t CardHeader[4];
		const static std::uint8_t Priority[PRIORITY_NUM][3];
		const static std::uint8_t Notice[NOTICE_NUM][3];
		std::uint8_t buffer[0x100];
		bool updateSignal;
	public:
		std::uint8_t UID[8];
		std::uint8_t *GetHeader() { return buffer; }
		std::uint8_t *GetData() { return buffer+4; }
		bool IsValid();
		const std::uint8_t *GetPriorityColor();
		const std::uint8_t *GetNoticeColor();
		void SetNotice(std::uint8_t level);
		bool NeedUpdate();
		std::uint8_t *GetPresId(std::uint8_t &len);
		std::uint8_t *GetQueueId(std::uint8_t &len);
		std::uint8_t *GetName(std::uint8_t &len);
		CardInfo();
		~CardInfo() {}
};

#endif
