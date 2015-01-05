#ifndef _BSON_DOCUMENT_H
#define _BSON_DOCUMENT_H

#include "INonterminal.h"
#include "Element.h"

namespace BSON
{
	class BsonDocument : public INonterminal
	{
	private:
		DocumentType docType;
		boost::shared_ptr<DynamicObject> object;
		boost::shared_ptr<ArrayBase> array;
		
		template<DocumentType T>
		void SerializeSub(std::vector<uint8_t> &list, std::size_t &size);

		bool InstantiateObject(DynamicObject::Table &t,DynamicObject &obj);
	public:
		BsonDocument()
			: docType(DocumentTypeUnknown) 
		{
			object = boost::make_shared<DynamicObject>();
		}
		
		template<class T>	
		BsonDocument(const boost::shared_ptr<T> &p)
		{
			if (boost::is_base_of<DynamicObject, T>::value)
			{
				docType = DocumentTypeObject;
				object = boost::reinterpret_pointer_cast<DynamicObject>(p);
			}
			else if (boost::is_base_of<ArrayBase, T>::value)
			{
				docType = DocumentTypeArray;
				array = boost::reinterpret_pointer_cast<ArrayBase>(p);
			}
		}
		
		virtual ~BsonDocument() {}
		void SetDocumentType(DocumentType type);
		virtual void SetValue(const boost::shared_ptr<TypeInfoBase> &val);
		virtual boost::shared_ptr<TypeInfoBase> &GetValue();
		virtual boost::shared_ptr<std::uint8_t[]> Serialize(std::size_t &size);
		virtual bool Deserialize(const boost::shared_ptr<MemStream> &stream);
	};	
}
#endif

