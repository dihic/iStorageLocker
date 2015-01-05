#ifndef _INONTERMINAL_H
#define _INONTERMINAL_H

#include <cstdint>
#include "TypeInfoBase.h"
#include "MemStream.h"

using namespace std;

namespace BSON
{
	enum ElementType
	{
			ElementTypeDouble = 1,
			ElementTypeString = 2,
			ElementTypeObject = 3,
			ElementTypeArray = 4,
			ElementTypeBinary = 5,
			ElementTypeUndefined = 6,      //Deprecated
			ElementTypeObjectId = 7,
			ElementTypeBoolean = 8,
			ElementTypeDateTime = 9,
			ElementTypeNull = 0x0A,
			ElementTypeRegex = 0x0B,
			ElementTypeReference = 0x0C,   //Deprecated
			ElementTypeCode = 0x0D,
			ElementTypeSymbol = 0x0E,
			ElementTypeScopedCode = 0x0F,
			ElementTypeInt32 = 0x10,
			ElementTypeTimestamp = 0x11,
			ElementTypeInt64 = 0x12,
	};

	enum BinarySubtype //: uint8_t
	{
			BinarySubtypeGeneric=0,
			BinarySubtypeFunction=1,
			BinarySubtypeOldBinary=2,
			BinarySubtypeOldUuid=3,
			BinarySubtypeUuid=4,
			BinarySubtypeMd5=5,
			BinarySubtypeUserDefined=0x80,
	};
	
	enum DocumentType
	{
		DocumentTypeUnknown,
		DocumentTypeObject,
		DocumentTypeTable,
		DocumentTypeArray
	};

	class INonterminal
	{
		protected:
			boost::shared_ptr<TypeInfoBase> pVal;
		public:
			INonterminal() {}
			virtual ~INonterminal() {}
			virtual void SetValue(const boost::shared_ptr<TypeInfoBase> &element)=0;
			virtual boost::shared_ptr<TypeInfoBase> &GetValue()=0;
			virtual boost::shared_ptr<std::uint8_t[]> Serialize(std::size_t &size)=0;
			virtual bool Deserialize(const boost::shared_ptr<MemStream> &stream)=0;
	};
}
#endif
