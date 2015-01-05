#ifndef _BSON_H
#define _BSON_H

#include "BsonDocument.h"
#include "MemStream.h"
#include <cstdint>
#include <cmsis_os.h> 

namespace BSON
{
	extern const osMutexDef_t os_mutex_def_bson;
	
	class Bson
	{
		//private:
			//static osMutexId bson_mutex_id;

		public:
			template<class T>
			static boost::shared_ptr<std::uint8_t[]> Serialize(const boost::shared_ptr<T> &obj, std::size_t &size);
			
			template<class T>
			static void Deserialize(const boost::shared_ptr<MemStream> &stream, boost::shared_ptr<T> &obj);
	};
	
	template<class T>
	void Bson::Deserialize(const boost::shared_ptr<MemStream> &stream, boost::shared_ptr<T> &obj)
	{
//		if (bson_mutex_id == 0)
//			bson_mutex_id = osMutexCreate(osMutex(bson));
		if (!boost::is_base_of<DynamicObject, T>::value)
		{
			obj.reset();
			return;
		}
		
		//osMutexWait(bson_mutex_id, osWaitForever);
		
		obj.reset(new T);
		boost::shared_ptr<BsonDocument> doc(new BsonDocument(obj));
		doc->Deserialize(stream);
		
		//osMutexRelease(bson_mutex_id);
	}
	
	template<class T>
	boost::shared_ptr<std::uint8_t[]> Bson::Serialize(const boost::shared_ptr<T> &obj, std::size_t &size)
	{
		//if (bson_mutex_id == 0)
		//	bson_mutex_id = osMutexCreate(osMutex(bson));
		
		boost::shared_ptr<std::uint8_t[]>  data;
		
		if (!boost::is_base_of<DynamicObject, T>::value)
			return data;
		
		//osMutexWait(bson_mutex_id, osWaitForever);
		
		boost::shared_ptr<BsonDocument> doc(new BsonDocument(obj));
		data = doc->Serialize(size);
		
		//osMutexRelease(bson_mutex_id);
		return data;
	}
}

#endif

