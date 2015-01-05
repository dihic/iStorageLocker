#ifndef _BSON_CSTRING_H
#define _BSON_CSTRING_H

#include "INonterminal.h"
#include <string>

namespace BSON
{
	class BsonCString: public INonterminal
	{
	private:
		std::string str;
	public:
		std::string &GetString() { return str; }
		void SetString(const string &val) { str=val; }
		BsonCString() {}
		virtual ~BsonCString() {}
		virtual void SetValue(const boost::shared_ptr<TypeInfoBase> &v);
		virtual boost::shared_ptr<TypeInfoBase> &GetValue() { return pVal; }
		virtual boost::shared_ptr<std::uint8_t[]> Serialize(std::size_t &size);
		virtual bool Deserialize(const boost::shared_ptr<MemStream> &stream);
	};
}
#endif
