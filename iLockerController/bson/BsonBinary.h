#ifndef _BSON_BINARY_H
#define _BSON_BINARY_H

#include "INonterminal.h"
#include "Guid.h"

namespace BSON
{
	class BsonBinary : public INonterminal
	{
	private:
		BinarySubtype subtype;
		std::uint32_t len;
		boost::shared_ptr<std::uint8_t[]> data;
	public:
		BsonBinary() : subtype(BinarySubtypeGeneric) {}
		virtual ~BsonBinary() {}
		
		virtual void SetValue(const boost::shared_ptr<TypeInfoBase> &val);
		virtual boost::shared_ptr<TypeInfoBase> &GetValue();
		virtual boost::shared_ptr<std::uint8_t[]> Serialize(std::size_t &size);
		virtual bool Deserialize(const boost::shared_ptr<MemStream> &stream);
	};
}
#endif
