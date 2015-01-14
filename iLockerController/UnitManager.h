#ifndef _UNIT_MANAGER_H
#define _UNIT_MANAGER_H

#include <map>
#include <cstdint>
#include <boost/smart_ptr.hpp>
#include <boost/make_shared.hpp>
#include "StorageUnit.h"

namespace IntelliStorage
{
	class UnitManager
	{
		private:
			std::map<std::uint16_t, boost::shared_ptr<StorageUnit> > unitList;
		public:
			typedef std::map<std::uint16_t, boost::shared_ptr<StorageUnit> >::iterator UnitIterator;
			UnitManager() {}
			~UnitManager() {}
			void Add(std::uint16_t id, boost::shared_ptr<StorageUnit> unit);
	
			std::map<std::uint16_t, boost::shared_ptr<StorageUnit> > &GetList() { return unitList; }
			boost::shared_ptr<StorageUnit> FindEmptyUnit();
			boost::shared_ptr<StorageUnit> FindUnit(std::string presId);
			boost::shared_ptr<StorageUnit> FindUnit(std::uint16_t id);
	};
	
}

#endif
