#include "BsonBinary.h"


namespace BSON
{
	void BsonBinary::SetValue(const boost::shared_ptr<TypeInfoBase> &val)
	{
		if (val.get()==NULL)
			return;
		if (val->IsBinary())
		{
			boost::shared_ptr<TypeInfo<Binary> > t = boost::dynamic_pointer_cast<TypeInfo<Binary> >(val);
			subtype = BinarySubtypeGeneric;
			data = t->GetBinaryData(len);
		}
		else
		{
			boost::shared_ptr<TypeInfo<Guid> > t = boost::dynamic_pointer_cast<TypeInfo<Guid> >(val);
			if (t.get() != NULL)
			{
				subtype=BinarySubtypeUuid;
				//subtype=BinarySubtypeOldUuid;
				len = 16;
				data = boost::make_shared<std::uint8_t[]>(len);
				uint8_t *bytes = t->GetObject()->ToByteArray();
				memcpy(data.get(),bytes,16);
			}
		}
	}
	
	boost::shared_ptr<TypeInfoBase> &BsonBinary::GetValue()
	{
		if (subtype==BinarySubtypeUuid || subtype==BinarySubtypeOldUuid)
		{
			boost::shared_ptr<Guid> g(new Guid(data.get()));
			pVal.reset(new TypeInfo<Guid>(g));
		}
		else
		{
			boost::shared_ptr<Binary> b(new Binary);
			b->insert(b->end(), data.get() ,data.get()+len);
			pVal.reset(new TypeInfo<Binary>(b));
		}
		return pVal;
	}

	boost::shared_ptr<std::uint8_t[]> BsonBinary::Serialize(std::size_t &size)
	{
		if (data.get() == NULL || len==0)
			return data;
		size = len+5;
		boost::shared_ptr<uint8_t[]> buf = boost::make_shared<uint8_t[]>(size);
		memcpy(buf.get(), &len, 4);
		buf[4] = subtype;
		memcpy(buf.get()+5, data.get(), len);
		return buf;
	}
		
	bool BsonBinary::Deserialize(const boost::shared_ptr<MemStream> &stream)
	{
		//size_t len;
		stream->GetBytes(&len,4);
		subtype=(BinarySubtype)stream->GetByte();
		if (subtype == BinarySubtypeOldBinary)
		{
			stream->GetBytes(&len,4);
		}
		if (len==0)
				return false;
		data = boost::make_shared<uint8_t[]>(len);
		stream->GetBytes(data.get(), len);
		return true;
	}
}
