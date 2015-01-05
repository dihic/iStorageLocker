#ifndef _MEM_STREAM_H
#define _MEM_STREAM_H

//#include "IByteStream.h"
#include <cstdint>
#include <cstddef>
#include <boost/smart_ptr.hpp>
#include <boost/make_shared.hpp>

class MemStream //: public IByteStream
{
	private:
		boost::shared_ptr<std::uint8_t[]> ptr;
		const std::size_t size;
		std::size_t pos;
		std::size_t posw;
	public:
		MemStream(boost::shared_ptr<std::uint8_t[]> &stream, std::size_t count, std::size_t offset = 0)
			:ptr(stream), size(count), pos(offset), posw(offset) 
		{
		}
		virtual ~MemStream() {}
		virtual std::size_t GetLength() const { return (size-pos>0)?(size-pos):0; }
		virtual std::uint8_t GetByte();
		virtual void GetBytes(void *bytes, std::size_t count);
		virtual void PutByte(const std::uint8_t &byte);
		virtual void PutBytes(const void *bytes, std::size_t count);
};

#endif

