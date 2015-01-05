#include "BsonCString.h"

using namespace std;

namespace BSON
{
	void BsonCString::SetValue(const boost::shared_ptr<TypeInfoBase> &v)
	{
		if (v.get()==NULL)
		{
			str.clear();
			pVal=v;
			return;
		}
		//const TypeInfo<string> *s=dynamic_cast<const TypeInfo<string> *>(v.get());
		boost::shared_ptr<TypeInfo<string> > s = boost::dynamic_pointer_cast<TypeInfo<string> >(v);
		if (s.get()==NULL)
			return;
		str = *(s->GetObject());
		pVal=v;
	}
	
	boost::shared_ptr<std::uint8_t[]> BsonCString::Serialize(std::size_t &size)
	{
		size=str.length()+1;
		boost::shared_ptr<std::uint8_t[]> data = boost::make_shared<uint8_t[]>(size);
		memcpy(data.get(), str.c_str(), size-1);
		data[size-1] = 0;
		return data;
	}
	
	bool BsonCString::Deserialize(const boost::shared_ptr<MemStream> &stream)
	{
		if (stream.get()==NULL)
			return false;
		str.clear();
		char b = stream->GetByte();
		while (b!=0)
		{
			str.push_back(b);
			b = stream->GetByte();
		}
		//TypeInfo<string> *info = new TypeInfo<string>(str);
		pVal = boost::make_shared<TypeInfo<string> >(str);
		return true;
	}
}
