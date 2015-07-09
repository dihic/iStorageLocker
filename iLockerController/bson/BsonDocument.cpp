#include "BsonDocument.h"
#include "Element.h"
#include <map>
#include <string>
#include <cmath>
//#include <iostream>

using namespace std;

namespace BSON
{
	void BsonDocument::SetDocumentType(DocumentType type)
	{
//		switch (type)
//		{
//			case DocumentTypeUnknown:
//				break;
//			case DocumentTypeObject:
//				break;
//			case DocumentTypeTable:
//				break;
//			case DocumentTypeArray:
//				break;
//		}
		docType=type;
	}
	
	void BsonDocument::SetValue(const boost::shared_ptr<TypeInfoBase> &val)
	{
	}
	
	boost::shared_ptr<TypeInfoBase> &BsonDocument::GetValue()
	{
		switch (docType)
		{
			case DocumentTypeUnknown:
				pVal.reset();
				break;
			case DocumentTypeObject:
				pVal.reset(new TypeInfo<DynamicObject>(object));
				break;
			case DocumentTypeTable:
				pVal.reset(new TypeInfo<DynamicObject>(object));
				break;
			case DocumentTypeArray:
				pVal = array;
				break;
		}
		return pVal;
	}
	
	template<DocumentType T>
	void BsonDocument::SerializeSub(std::vector<uint8_t> &list, size_t &size)
	{
		list.clear();
		size = 0;
	}
		
	
	template<>
	void BsonDocument::SerializeSub<DocumentTypeArray>(std::vector<uint8_t> &list, size_t &size) 
	{
		ArrayBase &arrayBase = *array;
		unsigned count = arrayBase.Count();
		string digit;
		for (unsigned i=0; i<count; i++)
		{
			digit.clear();
			int l = i==0? 0 : int(log10(float(i)));
			int div = 1;
			for (int j=0; j<l; ++j)
				div *= 10;
			for (int j=l; j>=0; --j)
			{
				digit += char(0x30+ i/div);
				div /= 10;
			}
			boost::shared_ptr<Element> e(new Element(digit, arrayBase[i]));
			size_t edlen=0;
			boost::shared_ptr<uint8_t[]> ed = e->Serialize(edlen);
			list.insert(list.end(), ed.get(), ed.get()+edlen);
		}
		size = list.size();
	}
	
	template<>
	void BsonDocument::SerializeSub<DocumentTypeObject>(std::vector<uint8_t> &list, size_t &size) 
	{
		//boost::shared_ptr<Element> e;
//		string name;
//		boost::shared_ptr<TypeInfoBase> p;
		
		DynamicObject::Table &map = object->GetTable();
		
		for (DynamicObject::Iterator  it = map.begin(); it != map.end(); ++it)
		{
//			name = it->first;
//			p = it->second;
			boost::shared_ptr<Element> e(new Element(it->first, it->second));
			size_t edlen=0;
			boost::shared_ptr<uint8_t[]> ed = e->Serialize(edlen);
			list.insert(list.end(), ed.get(), ed.get()+edlen);
		}
		size = list.size();
	}
	
	boost::shared_ptr<std::uint8_t[]> BsonDocument::Serialize(std::size_t &len)
	{
		len = 0;
		vector<uint8_t> list;

		switch (docType)
		{
			case DocumentTypeUnknown:
				SerializeSub<DocumentTypeUnknown>(list, len);
				break;
			case DocumentTypeTable:
				SerializeSub<DocumentTypeTable>(list, len);
				break;
			case DocumentTypeArray:
				SerializeSub<DocumentTypeArray>(list, len);
				break;
			case DocumentTypeObject:
				SerializeSub<DocumentTypeObject>(list, len);
		}
		len+=5;
		boost::shared_ptr<std::uint8_t[]> data = boost::make_shared<uint8_t[]>(len);
		memcpy(data.get(),&len,4);
		std::copy(list.begin(), list.end(), data.get()+4);
		data[len-1]=0;
		return data;
	}
	
	bool BsonDocument::InstantiateObject(DynamicObject::Table &t, DynamicObject &obj)
	{
		for(DynamicObject::Iterator it = t.begin();it != t.end();++it)
		{
			boost::shared_ptr<TypeInfoBase> member = obj.GetMember(it->first);
			if (member.get()==NULL)
				continue;
			if (member->IsArray())
			{
				boost::shared_ptr<ArrayBase> dest 	= boost::dynamic_pointer_cast<ArrayBase>(member);
				boost::shared_ptr<ArrayBase> source = boost::dynamic_pointer_cast<ArrayBase>(it->second);
				if (source.get()==NULL || dest.get()==NULL)
					continue;
				size_t count = source->Count();
				for(size_t i=0; i<count;++i)
				{
					boost::shared_ptr<TypeInfoBase> ps = (*source)[i];
					if (ps.get()==NULL) 	//invalid member 
						continue;
					if (ps->IsObject())
					{
						DynamicObject::Table &h = ps->GetObject<DynamicObject>()->GetTable();
						boost::shared_ptr<DynamicObject> o = ClassFactory::CreateObject(dest->ClassName);
						if (o.get() == NULL)
							continue;
						InstantiateObject(h, *o);
						boost::shared_ptr<TypeInfoBase> pBase(new TypeInfo<DynamicObject>(o));
						dest->Add(pBase);
					}
					else
					{
						dest->Add(ps);
					}
				}
				it->second.reset();
				continue;
			} 
			else if (member->IsObject())
			{
				boost::shared_ptr<TypeInfoBase> info = it->second;
				if (!info->IsObject())
					continue;
				
				DynamicObject::Table &h = info->GetObject<DynamicObject>()->GetTable();
				boost::shared_ptr<DynamicObject> o = member->GetObject<DynamicObject>();
				if (o.get() != NULL)
					InstantiateObject(h,*o);
				continue;
			}
			//else 
			
			boost::shared_ptr<TypeInfoBase> p = it->second;
			
			if (p->GetType()!=member->GetType())
			{
				if (p->GetType()==typeid(double) && member->GetType()==typeid(float))
				{
					boost::shared_ptr<double> d = p->GetObject<double>();
					boost::shared_ptr<float> f = member->GetObject<float>();
					*f=float(*d);
				}
				else if (p->GetType()==typeid(int64_t) && member->GetType()==typeid(uint64_t))
				{
					boost::shared_ptr<int64_t> d = p->GetObject<int64_t>();
					boost::shared_ptr<uint64_t> f = member->GetObject<uint64_t>();
					*f=uint64_t(*d);
				}
				else if (p->GetType()==typeid(int) && member->GetType()==typeid(uint32_t))
				{
					boost::shared_ptr<int> d = p->GetObject<int>();
					boost::shared_ptr<uint32_t> f = member->GetObject<uint32_t>();
					*f=uint32_t(*d);
				}
			}
			else
			{
				if (p->IsBinary())
				{
					boost::shared_ptr<TypeInfo<Binary> > binarySource = boost::dynamic_pointer_cast<TypeInfo<Binary> >(p);
					boost::shared_ptr<TypeInfo<Binary> > binaryDest = boost::dynamic_pointer_cast<TypeInfo<Binary> >(member);
					if (binarySource.get()!=NULL && binaryDest.get()!=NULL)
						*binaryDest->GetObject() = *binarySource->GetObject();
				}
				else if (p->GetType() == typeid(string))
				{
					boost::shared_ptr<string> strSource = p->GetObject<string>();
					boost::shared_ptr<string> strDest = member->GetObject<string>();
					if (strSource.get()!=NULL && strDest.get()!=NULL)
						*strDest = *strSource;
				}
				else
				{
					if (member->GetObject().get() !=NULL && p->GetObject().get() != NULL)
						memcpy(member->GetObject().get(), p->GetObject().get(), p->GetSize());
				}
			}
		}
		return true;
	}
	
	bool BsonDocument::Deserialize(const boost::shared_ptr<MemStream> &stream)
	{
		if (docType==DocumentTypeUnknown)
			return false;
		size_t num;
		stream->GetBytes(&num,4);
		
		boost::shared_ptr<DynamicObject::Table> pTable;
		if (object->IsRaw())
			pTable.reset(&object->GetTable(), boost::null_deleter());
		else
			pTable.reset(new DynamicObject::Table);
		
		//boost::shared_ptr<TypeInfoBase> base;
		boost::shared_ptr<Element> e;
		
		switch (docType)
		{
			case DocumentTypeUnknown:
				break;
			case DocumentTypeTable:
				e.reset(new Element());
				while(e->Deserialize(stream))
				{
					pTable->insert(pair<string, boost::shared_ptr<TypeInfoBase> >(e->GetKey(), e->GetValue()));
					e.reset(new Element());
				}
				break;
			case DocumentTypeArray:
				array.reset(new ArrayBase);
				e.reset(new Element());
				while(e->Deserialize(stream))
				{
					array->Add(e->GetValue());
					e.reset(new Element());
				}
				break;
			case DocumentTypeObject:
				e.reset(new Element());
				while(e->Deserialize(stream))
				{
					pTable->insert(pair<string, boost::shared_ptr<TypeInfoBase> >(e->GetKey(), e->GetValue()));
					e.reset(new Element());
				}
				if (!object->IsRaw())
					InstantiateObject(*pTable,*object);
				break;
			default:
				stream->GetByte();
				break;
		}
		return true;
	}
	
}
