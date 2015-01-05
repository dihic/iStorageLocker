#include "CardInfo.h"

#include <cstring>

#define INDEX_PRIORITY	4
#define INDEX_NOTICE		5
#define INDEX_PRESID		8
#define INDEX_QUEUEID		28
#define INDEX_NAME  		48

//Valid formated header
const std::uint8_t CardInfo::CardHeader[] = { 0x44, 0x49, 0x48, 0x4D }; //DIHM

const std::uint8_t CardInfo::Priority[PRIORITY_NUM][3] = { 
	{0xff, 0xff, 0xff},
	{0xff, 0x00, 0xff},
	{0x00, 0x00, 0xff} };

const std::uint8_t CardInfo::Notice[NOTICE_NUM][3] = { 
	{0x00, 0x00, 0x00},
	{0x00, 0xff, 0x00},
	{0xff, 0xff, 0x00},
};

CardInfo::CardInfo(): updateSignal(false)
{
	std::memset(buffer,0,0x100);
}

bool CardInfo::IsValid()
{
	return std::memcmp(buffer, CardHeader, 4)==0;
}

const std::uint8_t *CardInfo::GetPriorityColor()
{
	return Priority[buffer[INDEX_PRIORITY]>=PRIORITY_NUM ? 0 : buffer[INDEX_PRIORITY]];
}

const std::uint8_t *CardInfo::GetNoticeColor()
{
	return Notice[buffer[INDEX_NOTICE]>=NOTICE_NUM ? 0 : buffer[INDEX_NOTICE]];
}

void CardInfo::SetNotice(std::uint8_t level)
{
	buffer[INDEX_NOTICE]=level;
	updateSignal = true;
}

bool CardInfo::NeedUpdate()
{
	bool result = updateSignal;
	updateSignal = false;
	return result;
}

std::uint8_t *CardInfo::GetPresId(std::uint8_t &len)
{
	len = 0;
	std::uint8_t *p = buffer+INDEX_PRESID;
	while (len<20 && p[len]!=0)
		++len;
	return p;
}

std::uint8_t *CardInfo::GetQueueId(std::uint8_t &len)
{
	len = 0;
	std::uint8_t *p = buffer+INDEX_QUEUEID;
	while (len<10 && (p[len<<1]!=0 || p[(len<<1)+1]!=0))
		++len;
	return p;
}

std::uint8_t *CardInfo::GetName(std::uint8_t &len)
{
	len = 0;
	std::uint8_t *p = buffer+INDEX_NAME;
	while (len<10 && (p[len<<1]!=0 || p[(len<<1)+1]!=0))
		++len;
	return p;
}

