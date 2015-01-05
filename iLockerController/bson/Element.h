#ifndef _ELEMENT_H
#define _ELEMENT_H

#include <string>
#include "INonterminal.h"
#include "BsonBinary.h"
#include "BsonCString.h"
#include "BsonString.h"

namespace BSON
{
	class Element : public INonterminal
	{
		private:
			ElementType elementType;
			boost::shared_ptr<std::uint8_t[]> data;
			std::size_t datalen;

			BsonCString ename;

			BsonBinary binary;
			BsonString bstr;
			boost::shared_ptr<ArrayBase> array;
			boost::shared_ptr<DynamicObject> object;
		
			void SetupValue(const boost::shared_ptr<TypeInfoBase> &element);
		
			template<ElementType T>
			void SerializeSub(boost::shared_ptr<std::uint8_t[]> &t, std::size_t &len);
		public:
			std::string Key;
			Element() : elementType(ElementTypeNull) { }
			Element(string key,const boost::shared_ptr<TypeInfoBase> &element);
			void SetKey(std::string key) { ename.SetString(key); };
			std::string &GetKey() { return ename.GetString(); }
			virtual ~Element();
			virtual void SetValue(const boost::shared_ptr<TypeInfoBase> &element)
			{
				SetupValue(element);
			}
			virtual boost::shared_ptr<TypeInfoBase> &GetValue();
			virtual boost::shared_ptr<std::uint8_t[]> Serialize(std::size_t &size);
			virtual bool Deserialize(const boost::shared_ptr<MemStream> &stream);
	};
}

#endif
