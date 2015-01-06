#include "Guid.h"
#include "System.h"


using namespace std;

void Guid::New()
{
	uint32_t *ptr = reinterpret_cast<uint32_t *>(raw);
	for(int i=0;i<4;++i)
		ptr[i] = GET_RANDOM_NUMBER;
	uint8_t y = (raw[8]&0x30) | 0x80;
	raw[8] = y | (raw[8] & 0x0f);
#ifdef GUID_BIGENDIAN
	raw[6] = 0x40 | (raw[6] & 0x0f);
#else
	raw[7] = 0x40 | (raw[7] & 0x0f);
#endif
}

const Guid Guid::Empty = Guid();

Guid::Guid(const uint8_t *data)
{
	memcpy(raw,data,16);
}

bool Guid::operator == (Guid& _test)  
{  
	return memcmp(raw, _test.raw, 16)==0; 
};

bool Guid::operator != (Guid& _test)  
{  
	return memcmp(raw, _test.raw ,16)!=0; 
};

string Guid::DigitToStringHelper(uint8_t *digit, int8_t size, bool bigEndian)
{
	string str;
	char temp;
	if (bigEndian)
	{
		for (int8_t i=0;i<size;++i)
		{
			temp = (digit[i] & 0xf0) >>4;
			if (temp>9)
				temp += 55;
			else
				temp += 48;
			str.push_back(temp);
			temp = digit[i] & 0x0f;
			if (temp>9)
				temp += 55;
			else
				temp += 48;
			str.push_back(temp);
		}
	}
	else
	{
		for (int8_t i=size-1;i>=0;--i)
		{
			temp = (digit[i] & 0xf0) >>4;
			if (temp>9)
				temp += 55;
			else
				temp += 48;
			str.push_back(temp);
			temp = digit[i] & 0x0f;
			if (temp>9)
				temp += 55;
			else
				temp += 48;
			str.push_back(temp);
		}
	}
	return str;
}

const string &Guid::ToString()
{
	str.clear();	
#ifdef GUID_BIGENDIAN
	str+=DigitToStringHelper(raw,4,true);
	str+='-';
	str+=DigitToStringHelper(raw+4,2,true);
	str+='-';
	str+=DigitToStringHelper(raw+6,2,true);
#else
	str+=DigitToStringHelper(raw,4,false);
	str+='-';
	str+=DigitToStringHelper(raw+4,2,false);
	str+='-';
	str+=DigitToStringHelper(raw+6,2,false);
#endif
	str+='-';
	str+=DigitToStringHelper(raw+8,2,true);
	str+='-';
	str+=DigitToStringHelper(raw+10,6,true);
	
	return str;
}

