#include "BsonString.h"

#include <cstring>

using namespace std;

namespace BSON
{
	void BsonString::SetValue(const boost::shared_ptr<TypeInfoBase> &v)
	{
		if (v.get()==NULL)
		{
			str.clear();
			pVal = v;
			return;
		}
		boost::shared_ptr<TypeInfo<string> > s =  boost::dynamic_pointer_cast<TypeInfo<string> >(v);
		if (s.get()==NULL) 
			return;
		str = *(s->GetObject());
		pVal = v;
	}
	
	boost::shared_ptr<std::uint8_t[]> BsonString::Serialize(std::size_t &size)
	{
		size_t len = str.length()+1;
		size = len+4;
		boost::shared_ptr<std::uint8_t[]> data = boost::make_shared<uint8_t[]>(size);
		memcpy(data.get(), &len, 4);
		memcpy(data.get()+4, str.c_str(), len-1);
		data[size-1] = 0;
		return data;
	}
	
	bool BsonString::Deserialize(const boost::shared_ptr<MemStream> &stream)
	{
		if (stream.get()==NULL)
			return false;
		size_t len;
		stream->GetBytes(&len,4);
		if (len==0)
			return false;
		char *s=new char[len+1];
		s[len]=0;
		stream->GetBytes(s,len);
		str.assign(s);
		delete [] s;
		pVal = boost::make_shared<TypeInfo<string> >(str);		
		return true;
	}
}
