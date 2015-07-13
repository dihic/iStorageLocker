#ifndef __ODENTRY_H
#define __ODENTRY_H

#include <cstdint>
#include <cstring>
#include <boost/smart_ptr.hpp>
#include <boost/make_shared.hpp>

namespace CANExtended
{
	class OdEntry
	{
		private:
			boost::shared_ptr<std::uint8_t[]> val;
			std::uint8_t size;
		public:
			const std::uint16_t Index;
			const std::uint8_t SubIndex;
		
			std::uint8_t GetLen() const { return size; }
			const boost::shared_ptr<std::uint8_t[]> &GetVal() { return val; } 
			
			void SetVal(const std::uint8_t *ptr, std::uint8_t count)
			{
				val = boost::make_shared<std::uint8_t[]>(count);
				size = count;
				std::memcpy(val.get(), ptr, count);
			}
			
			void SetVal(const boost::shared_ptr<std::uint8_t[]> &v, std::uint8_t count)
			{
				val = v;
				size = count;
//				val = boost::make_shared<std::uint8_t[]>(count);
//				size = count;
//				std::memcpy(val.get(), v.get(), count);
			}

			OdEntry(std::uint16_t index, std::uint8_t subindex, std::uint8_t count = 0)
				: size(count), Index(index), SubIndex(subindex)
			{
				if (count>0)
				{
					val = boost::make_shared<std::uint8_t[]>(count);
					std::memset(val.get(), 0, count);
				}
			}
			
			OdEntry(const OdEntry &entry)
				: size(entry.size), Index(entry.Index), SubIndex(entry.SubIndex)
			{
				val = entry.val;
			}
			
			~OdEntry() 
			{
			}
	};
}

#endif
