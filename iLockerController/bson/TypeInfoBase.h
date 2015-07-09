#ifndef _TYPEINFOBASE_H
#define _TYPEINFOBASE_H

#include <string>
#include <map>
#include <vector>
#include <typeinfo>
#include <cstdint>
#include <cstddef>

#include <boost/type_traits.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/core/null_deleter.hpp>

typedef std::vector<std::uint8_t> Binary;

class DynamicObject;

class TypeInfoBase
{
	protected:
		const std::type_info &	info;
		const std::size_t 			size;
		boost::shared_ptr<void> Ptr;
		TypeInfoBase(const std::type_info &i,std::size_t s)
			: info(i),size(s) {}
	public:		
		template<class T>
		boost::shared_ptr<T> GetObject() const 
		{
			return boost::reinterpret_pointer_cast<T>(Ptr);
		}
		
		boost::shared_ptr<void> GetObject() const
		{
			return Ptr;
		}
		std::size_t GetSize() const { return size; }
		const std::type_info &GetType() const { return info; }
			
		virtual ~TypeInfoBase() {}
		virtual bool IsBinary() const =0;
		virtual bool IsArray() 	const =0;
		virtual	bool IsObject() const =0;
};

template<class T>
class TypeInfo : public TypeInfoBase
{
	friend class TypeInfoBase;
	public:
		static boost::shared_ptr<TypeInfo<T> > Register(T &p)
		{
			boost::shared_ptr<TypeInfo<T> > info(new TypeInfo<T>);
			boost::shared_ptr<T> px(&p, boost::null_deleter());
			info->Ptr = px;
			return info;
		}	
		
		TypeInfo(boost::shared_ptr<T> &p) 
			: TypeInfoBase(typeid(T), sizeof(T)),
				isObj(boost::is_base_of<DynamicObject, T>::value)
		{	Ptr = p;}
		
		TypeInfo(const T &p) 
			: TypeInfoBase(typeid(T),sizeof(T)),
				isObj(boost::is_base_of<DynamicObject, T>::value)
		{	Ptr = boost::make_shared<T>(p);}
		
		virtual ~TypeInfo() {}
		boost::shared_ptr<T> GetObject() const 
		{
			return boost::reinterpret_pointer_cast<T>(Ptr);
		}
		
		virtual const std::type_info &GetType() const { return info; }
		virtual bool IsBinary() const { return false; }
		virtual bool IsArray() const { return false; }
		virtual bool IsObject() const { return isObj; }
	private:
		const bool isObj;
		TypeInfo()
			: TypeInfoBase(typeid(T), sizeof(T)), 
				isObj(boost::is_base_of<DynamicObject, T>::value)
		{}
};

template<>
class TypeInfo<Binary> : public TypeInfoBase
{
	friend class TypeInfoBase;
	private:
		boost::shared_ptr<Binary> binaryPtr;
	public:
		static boost::shared_ptr<TypeInfo<Binary> > Register(Binary &b)
		{
			boost::shared_ptr<Binary> bx(&b, boost::null_deleter());
			boost::shared_ptr<TypeInfo<Binary> > info(new TypeInfo<Binary>(bx));
			return info;
		}
		
		TypeInfo(boost::shared_ptr<Binary> &p)
			: TypeInfoBase(typeid(Binary),sizeof(Binary))
		{
			Ptr = p;
			binaryPtr = p;
		}

		const boost::shared_ptr<Binary> &GetObject() { return binaryPtr; }
		
		boost::shared_ptr<std::uint8_t[]> GetBinaryData(std::size_t &size) 
		{
			//boost::shared_ptr<Binary> binary = binaryPtr.lock();
			size = binaryPtr->size();
			boost::shared_ptr<std::uint8_t[]> array = boost::make_shared<std::uint8_t[]>(size);
			std::copy(binaryPtr->begin(), binaryPtr->end(), array.get());
			return array;
		}
		
		virtual ~TypeInfo() {}
		virtual bool IsBinary() const { return true; }
		virtual bool IsArray() const { return false; }
		virtual bool IsObject() const { return false; } 
};

template<class T>
class Array;
class ArrayGeneric;

class ArrayBase : public TypeInfoBase
{
	template<class T>
	friend class Array;
	friend class ArrayGeneric;
	
	protected:
		std::vector<boost::shared_ptr<TypeInfoBase> > list;
		bool isObj;
	public:
		const char *ClassName;
	
		ArrayBase()
			: TypeInfoBase(typeid(char), 1), 
				isObj(false),
				ClassName(NULL)
		{}
	
		template<class U>
		ArrayBase(U &dump) 
			: TypeInfoBase(typeid(U), sizeof(U)),
				isObj(boost::is_base_of<DynamicObject, U>::value),
				ClassName(NULL)
		{}
		
		template<class U>
		ArrayBase(U &dump, const char *cn) 
			: TypeInfoBase(typeid(U), sizeof(U)),
				isObj(boost::is_base_of<DynamicObject, U>::value),
				ClassName(cn)
		{}
		
		std::size_t Count() const { return list.size(); }
		
		boost::shared_ptr<TypeInfoBase> & operator [](unsigned int n)
		{
			return (list.at(n));
		}
	
		void Add(boost::shared_ptr<TypeInfoBase> &base) 
		{
			list.push_back(base);
		}
		
		virtual ~ArrayBase() 
		{
		}

		virtual bool IsBinary() const { return false; }
		virtual bool IsArray() const { return true; }
		virtual bool IsObject() const { return isObj; } 
};

//Above defined class TypeInfoBase and derived types

class ArrayGeneric
{
protected:
	boost::shared_ptr<ArrayBase> pArray;
	ArrayGeneric() {}
public:
	virtual ~ArrayGeneric() {}
	
	boost::shared_ptr<ArrayBase> & GetArray() { return pArray; }
	std::size_t Count() const { return pArray->Count(); }
	
};

class DynamicObject
{
	public:
		typedef std::map<std::string, boost::shared_ptr<TypeInfoBase> >::iterator Iterator;
		typedef std::map<std::string, boost::shared_ptr<TypeInfoBase> > 					Table;
		const char *ClassName;
	protected:
		Table m_Map;
		
		template<class U>
		void RegisterTypeInfo(const char *name, U &member)
		{
			std::string memberName = name;
			boost::shared_ptr<TypeInfoBase> base = TypeInfo<U>::Register(member);
			m_Map[memberName] = base;
		}
		
		template<class V>
		void RegisterTypeInfo(const char *name, Array<V> &member)
		{
			std::string memberName = name;
			boost::shared_ptr<TypeInfoBase> base = member.GetArray();
			m_Map[memberName] = base;
		}
		
	public:		
		DynamicObject(const char cn[]): ClassName(cn) { }
		DynamicObject() : ClassName(NULL) { }
		virtual ~DynamicObject() 
		{
		}
			
		boost::shared_ptr<TypeInfoBase> GetMember(std::string memberName)
		{
			Iterator it = m_Map.find(memberName);
			if (it != m_Map.end())
				return it->second;
			boost::shared_ptr<TypeInfoBase> empty;
			return empty;
		}
		
		virtual bool IsRaw() { return 1; }
		
		Table &GetTable() { return m_Map; }
};

template<class T>
class Array : public ArrayGeneric
{
public:
	Array()
	{
		T dump;
		if (boost::is_base_of<DynamicObject, T>::value)
		{
			DynamicObject *obj = (DynamicObject *)(&dump);
			pArray = boost::make_shared<ArrayBase>(dump, obj->ClassName);
		}
		else
			pArray = boost::make_shared<ArrayBase>();
	}
	
	Array(boost::shared_ptr<ArrayBase> &p)
	{
		if (typeid(T)==p->GetType())
			pArray = p;
		else
			pArray.reset();
	}
	
	virtual ~Array() {}
	
	void Clear()
	{
		pArray->list.clear();
	}
	
	//Add a constant element
	void Add(const T &element) 
	{
		boost::shared_ptr<TypeInfoBase> base(new TypeInfo<T>(element));
		pArray->Add(base); 
	}
	
	//Add an element pointer
	void Add(boost::shared_ptr<T> &element) 
	{
		boost::shared_ptr<TypeInfoBase> base(new TypeInfo<T>(element));
		pArray->Add(base);
	}
	
	T &operator [](unsigned int n)
	{
		ArrayBase &ab = *pArray;
		//boost::weak_ptr<TypeInfoBase> pBase = ab[n];
		T &result = *ab[n]->GetObject<T>();
		return result;
	}
};


typedef boost::shared_ptr<DynamicObject> (*InitiatorFuntion)(void);

class ClassFactory   
{   
public:	
	static boost::shared_ptr<DynamicObject> CreateObject(std::string type)
	{
		std::map<std::string, InitiatorFuntion>::iterator it = m_clsMap.find(type);
		if ( it != m_clsMap.end() )   
		{   
			return (it->second)();
		}  
		boost::shared_ptr<DynamicObject> empty;
		return empty;
	}
private:
	static void RegisterClass(std::string type,InitiatorFuntion method)
	{
		m_clsMap[type]=method;
	}
	static std::map<std::string, InitiatorFuntion> m_clsMap;
	
	template<class T, const char name[]> friend class DynamicBase;
};


template<class T, const char class_name[]>
class DynamicBase : public DynamicObject
{
	private:
		static boost::shared_ptr<DynamicObject> CreateInstance()//__attribute__((malloc))
		{
			return boost::make_shared<T>(); //  new T;
		}
	public:
		static void Register()
		{
			ClassFactory::RegisterClass(std::string(class_name), CreateInstance);
			ClassFactory::RegisterClass(std::string(typeid(T).name()), CreateInstance);
		}
		DynamicBase() : DynamicObject(class_name) {}
		virtual ~DynamicBase() {}
		virtual bool IsRaw() { return 0; }
};

#define DECLARE_CLASS(class_name) \
extern const char _name_##class_name[]; \
class class_name : public DynamicBase<class_name, _name_##class_name>

#define CLAIM_CLASS(class_name) \
const char _name_##class_name[]=#class_name
	
#define DEFINE_CLASS(class_name) \
char _name_##class_name[]=#class_name; \
class class_name : public DynamicBase<class_name, _name_##class_name>

#define REGISTER_CLASS(class_name)	\
DynamicBase<class_name, _name_##class_name>::Register()

#define DEFINE_CLASS_EX(class_name,base_class) \
DEFINE_CLASS(class_name), public base_class

#define REGISTER_FIELD(field) \
RegisterTypeInfo(#field,field)

#endif
