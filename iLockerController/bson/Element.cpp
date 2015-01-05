#include "Element.h"
#include "Guid.h"
#include <ctime>
#include <vector>
#include "BsonString.h"
#include "BsonDocument.h"

using namespace std;

namespace BSON
{
	Element::~Element()
	{
	}
	
	Element::Element(string key,const boost::shared_ptr<TypeInfoBase> &element)
	{
		ename.SetString(key);
		SetupValue(element);
	}
	
	void Element::SetupValue(const boost::shared_ptr<TypeInfoBase> &element)
	{
		uint8_t *temp;

		if (element.get()==NULL)
		{
			elementType=ElementTypeNull;
			return;
		}
		
		if (element->IsArray())
		{
			elementType =ElementTypeArray;
			//data = reinterpret_cast<uint8_t *>(const_cast<TypeInfoBase *>(element));
			array = boost::dynamic_pointer_cast<ArrayBase>(element);
			return;
		}
		if (element->IsObject())
		{
			elementType = ElementTypeObject;
			//data = reinterpret_cast<uint8_t *>(element->PtrObject.get());
			object = element->GetObject<DynamicObject>();
			return;
		}
		
		//const TypeInfo<uint8_t *> *eBinary=dynamic_cast<const TypeInfo<uint8_t *> *>(element.get());
		const TypeInfo<Guid> *eGuid=dynamic_cast<const TypeInfo<Guid> *>(element.get());
		if (element->IsBinary() || eGuid!=NULL)
		{
			elementType = ElementTypeBinary;
			binary.SetValue(element);
			return;
		}
		
		if (element->GetType() == typeid(time_t))
		{
			elementType = ElementTypeDateTime;
			temp = reinterpret_cast<uint8_t *>(element->GetObject().get());
			data = boost::make_shared<uint8_t[]>(8);
			memcpy(data.get(), temp, 4);
			return;
		}
		
		const TypeInfo<string> *eString=dynamic_cast<const TypeInfo<string> *>(element.get());
		if (eString!=NULL)
		{
			elementType=ElementTypeString;
			bstr.SetString(*(eString->GetObject()));
			return;
		}
		
		const TypeInfo<double> *eDouble=dynamic_cast<const TypeInfo<double> *>(element.get());
		if (eDouble!=NULL)
		{
			elementType = ElementTypeDouble;
			temp = reinterpret_cast<uint8_t *>(eDouble->GetObject().get());
			data = boost::make_shared<uint8_t[]>(8);
			memcpy(data.get(), temp, 8);
			return;
		}
		const TypeInfo<float> *eFloat=dynamic_cast<const TypeInfo<float> *>(element.get());
		if (eFloat!=NULL)
		{
			elementType = ElementTypeDouble;
			double f2d = double(*(eFloat->GetObject()));
			data = boost::make_shared<uint8_t[]>(8);
			memcpy(data.get(), &f2d, 8);
			return;
		}
		
		const TypeInfo<bool> *eBool=dynamic_cast<const TypeInfo<bool> *>(element.get());
		if (eBool!=NULL)
		{
			elementType = ElementTypeBoolean;
			temp = reinterpret_cast<uint8_t *>(element->GetObject().get());
			data = boost::make_shared<uint8_t[]>(1);
			data[0] = *temp;
			return;
		}
		
		const TypeInfo<int32_t> *eInt32=dynamic_cast<const TypeInfo<int32_t> *>(element.get());
		const TypeInfo<uint32_t> *eUInt32=dynamic_cast<const TypeInfo<uint32_t> *>(element.get());
		if (eInt32!=NULL || eUInt32!=NULL)
		{
			elementType = ElementTypeInt32;
			temp = reinterpret_cast<uint8_t *>(element->GetObject().get());
			data = boost::make_shared<uint8_t[]>(4);
			memcpy(data.get(), temp, 4);
			return;
		}
		
		const TypeInfo<int64_t> *eInt64=dynamic_cast<const TypeInfo<int64_t> *>(element.get());
		const TypeInfo<uint64_t> *eUInt64=dynamic_cast<const TypeInfo<uint64_t> *>(element.get());
		if (eInt64!=NULL || eUInt64!=NULL)
		{
			elementType = ElementTypeInt64;
			temp = reinterpret_cast<uint8_t *>(element->GetObject().get());
			data = boost::make_shared<uint8_t[]>(8);
			memcpy(data.get(), temp, 8);
			return;
		}
	
	}

	
	
	boost::shared_ptr<TypeInfoBase> &Element::GetValue()
	{
		double doubleVal;
		time_t timeVal;
		int64_t int64Val;
		int32_t int32Val;
		bool boolVal;
		switch (elementType)
		{
			case ElementTypeString:
			case ElementTypeCode:
			case ElementTypeSymbol:
				pVal = bstr.GetValue();
				break;
			case ElementTypeBinary:
				pVal = binary.GetValue();
				break;
			case ElementTypeDouble:
				memcpy(&doubleVal, data.get(), 8);
				//pVal = TypeInfoBase::CreateConst(*const_cast<const double *>(&doubleVal));
				pVal.reset(new TypeInfo<double>(doubleVal));
				break;
			case ElementTypeDateTime:
				memcpy(&timeVal, data.get(), 4);
				//pVal = TypeInfoBase::CreateConst(*const_cast<const time_t *>(&timeVal));
				pVal.reset(new TypeInfo<time_t>(timeVal));
				break;
			case ElementTypeInt64:
			case ElementTypeTimestamp:
				memcpy(&int64Val, data.get(), 8);
				//pVal = TypeInfoBase::CreateConst(*const_cast<const int64_t *>(&int64Val));
				pVal.reset(new TypeInfo<int64_t>(int64Val));
				break;
			case ElementTypeInt32:
				memcpy(&int32Val, data.get(), 4);
				//pVal = TypeInfoBase::CreateConst(*const_cast<const int32_t *>(&int32Val));
				pVal.reset(new TypeInfo<int32_t>(int32Val));
				break;
			case ElementTypeBoolean:
				boolVal = *data.get();
				//pVal = TypeInfoBase::CreateConst(*const_cast<const bool *>(&boolVal));
				pVal.reset(new TypeInfo<bool>(boolVal));
				break;
			case ElementTypeObject:
				//pVal = boost::shared_ptr<TypeInfoBase>::Null;
				break;
      case ElementTypeArray:
				//pVal = boost::shared_ptr<TypeInfoBase>::Null;
				break;
			case ElementTypeObjectId:
			case ElementTypeRegex:
			case ElementTypeScopedCode:
			case ElementTypeReference:
			case ElementTypeNull:
			case ElementTypeUndefined:
				pVal.reset();
        break;
		}
		return pVal;
	}
	
	template<ElementType T>
	void Element::SerializeSub(boost::shared_ptr<std::uint8_t[]> &t, std::size_t &len)
	{
		t.reset();
		len=0;
	}

	template<>
	void Element::SerializeSub<ElementTypeArray>(boost::shared_ptr<std::uint8_t[]> &t, std::size_t &len)
	{
		BsonDocument doc(array);
		t = doc.Serialize(len);
	}
	
	template<>
	void Element::SerializeSub<ElementTypeObject>(boost::shared_ptr<std::uint8_t[]> &t, std::size_t &len)
	{
		BsonDocument doc(object);
		t = doc.Serialize(len);
	}
	
	boost::shared_ptr<std::uint8_t[]> Element::Serialize(std::size_t &len)
	{
		int i;
		
		boost::shared_ptr<std::uint8_t[]> d2;
		boost::shared_ptr<std::uint8_t[]> result;
	
		len = 0;
		size_t d1len = 0;
		size_t d2len = 0;
		
		boost::shared_ptr<std::uint8_t[]> d1 = ename.Serialize(d1len);
		
		switch (elementType)
		{
			case ElementTypeString:
			case ElementTypeCode:
			case ElementTypeSymbol:
				d2 = bstr.Serialize(d2len);
				len = d1len+d2len+1;
				result = boost::make_shared<uint8_t[]>(len);
				result[0] = elementType;
				memcpy(result.get()+1, d1.get(), d1len);
				memcpy(result.get()+d1len+1, d2.get(), d2len);
				break;
			case ElementTypeBinary:
				d2 = binary.Serialize(d2len);
				len = d1len+d2len+1;
				result = boost::make_shared<uint8_t[]>(len);
				result[0] = elementType;
				memcpy(result.get()+1, d1.get(), d1len);
				memcpy(result.get()+d1len+1, d2.get(), d2len);
				break;
			case ElementTypeDouble:
				len = d1len+9;
				result = boost::make_shared<uint8_t[]>(len);
				result[0] = elementType;
				memcpy(result.get()+1, d1.get(), d1len);
				memcpy(result.get()+d1len+1, data.get(), 8);
				break;
			case ElementTypeBoolean:
				len = d1len+2;
				result = boost::make_shared<uint8_t[]>(len);
				result[0] = elementType;
				memcpy(result.get()+1, d1.get(), d1len);
				result[len-1]= data[0];
				break;
			case ElementTypeDateTime:
				len = d1len+9;
				result = boost::make_shared<uint8_t[]>(len);
				result[0] = elementType;
				memcpy(result.get()+1, d1.get(), d1len);
				for(i=0;i<8;++i)
					if (i>=sizeof(time_t))
						result[d1len+1+i] = 0;
					else
						result[d1len+1+i] = data[i];
				break;
			case ElementTypeInt32:
				len=d1len+5;
				result = boost::make_shared<uint8_t[]>(len);
				result[0] = elementType;
				memcpy(result.get()+1,d1.get(),d1len);
				memcpy(result.get()+d1len+1,data.get(),4);
				break;
			case ElementTypeInt64:
			case ElementTypeTimestamp:
				len=d1len+9;
				result = boost::make_shared<uint8_t[]>(len);
				result[0] = elementType;
				memcpy(result.get()+1,d1.get(),d1len);
				memcpy(result.get()+d1len+1,data.get(),8);
				break;
			case ElementTypeObject:
				Element::SerializeSub<ElementTypeObject>(d2, d2len);
				len=d1len+d2len+1;
				result = boost::make_shared<uint8_t[]>(len);
				result[0] = elementType;
				memcpy(result.get()+1,d1.get(),d1len);
				memcpy(result.get()+d1len+1,d2.get(),d2len);
				break;
      case ElementTypeArray:
				Element::SerializeSub<ElementTypeArray>(d2, d2len);
				len=d1len+d2len+1;
				result = boost::make_shared<uint8_t[]>(len);
				result[0] = elementType;
				memcpy(result.get()+1,d1.get(),d1len);
				memcpy(result.get()+d1len+1,d2.get(),d2len);
				break;
			case ElementTypeObjectId:
				break;
			case ElementTypeRegex:
				break;
			case ElementTypeReference:
        break;
      case ElementTypeScopedCode:
				break;
			case ElementTypeNull:
        break;
			case ElementTypeUndefined:
        break;
		}
		return result;
	}
	
	bool Element::Deserialize(const boost::shared_ptr<MemStream> &stream)
	{
		uint8_t typenode=stream->GetByte();
		if (!typenode)
			return false;
		boost::shared_ptr<BsonDocument> doc;
		elementType= (ElementType)typenode;
		
		ename.Deserialize(stream);

		switch (elementType)
		{
			case ElementTypeString:
			case ElementTypeCode:
			case ElementTypeSymbol:
				bstr.Deserialize(stream);
				break;
			case ElementTypeDouble:
			case ElementTypeDateTime:
			case ElementTypeInt64:
			case ElementTypeTimestamp:
				data = boost::make_shared<uint8_t[]>(8);
			  datalen = 8;
				stream->GetBytes(data.get(),8);
				break;
			case ElementTypeInt32:
				data= boost::make_shared<uint8_t[]>(4);
				datalen = 4;
				stream->GetBytes(data.get(),4);
				break; 
			case ElementTypeBinary:
				binary.Deserialize(stream);
				break;
			case ElementTypeBoolean:
				data = boost::make_shared<uint8_t[]>(1);
				datalen = 1;
				data[0] = stream->GetByte()!=0;
				break;
			case ElementTypeObject:
				doc.reset(new BsonDocument);
				doc->SetDocumentType(DocumentTypeTable);
				doc->Deserialize(stream);
				pVal = doc->GetValue();
				break;
      case ElementTypeArray:
				doc.reset(new BsonDocument);
				doc->SetDocumentType(DocumentTypeArray);
				doc->Deserialize(stream);
				pVal = doc->GetValue();
				break;
			case ElementTypeObjectId:
			case ElementTypeRegex:
			case ElementTypeScopedCode:
				break;
			case ElementTypeReference:
			case ElementTypeNull:
			case ElementTypeUndefined:
        break;
			default:
				break;
		}
		return true;
	}
}
