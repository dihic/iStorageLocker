#include "MemStream.h"
#include <cstring>

using namespace std;

uint8_t MemStream::GetByte()
{
	uint8_t byte = (pos>=size) ? 0 : ptr[pos++];
	return  byte;
}

void MemStream::GetBytes(void *bytes, std::size_t count)
{
	size_t len = GetLength();
	if (len == 0)
		return;
	if (count <= len)
	{
		memcpy(bytes, ptr.get()+pos, count);
		pos += count;
	}
	else
	{
		if (len>0)
			memcpy(bytes, ptr.get()+pos, len);
		memset(reinterpret_cast<uint8_t *>(bytes)+len, 0, count-len);
		pos = size;
	}
} 
void MemStream::PutByte(const std::uint8_t &byte)
{
	if (posw < size)
		ptr[posw++] = byte;
}

void MemStream::PutBytes(const void *bytes, std::size_t count)
{
	int len = size - posw;
	if (len > count)
		len = count;
	if (len > 0)
	{
		memcpy(ptr.get() + posw, bytes, len);
		posw += len;
	}
}

