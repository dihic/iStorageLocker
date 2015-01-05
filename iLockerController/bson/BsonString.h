#ifndef _BSON_STRING_H
#define _BSON_STRING_H

#include "INonterminal.h"
#include <string>

namespace BSON
{
	class BsonString: public INonterminal
	{
	private:
		std::string str;
	public:
		BsonString() {}
		void SetString(const string &s) { str=s; }
		virtual ~BsonString() {}
		virtual void SetValue(const boost::shared_ptr<TypeInfoBase> &v);
		virtual boost::shared_ptr<TypeInfoBase> &GetValue() { return pVal; }
		virtual boost::shared_ptr<std::uint8_t[]> Serialize(std::size_t &size);
		virtual bool Deserialize(const boost::shared_ptr<MemStream> &stream);
	};
}
#endif
