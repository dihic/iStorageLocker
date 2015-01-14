#include "UnitManager.h"

using namespace std;

namespace IntelliStorage
{
	boost::shared_ptr<StorageUnit> UnitManager::FindEmptyUnit()
	{
		boost::shared_ptr<StorageUnit> unit;
		for (UnitIterator it = unitList.begin(); it != unitList.end(); ++it)
			if (it->second->IsEmpty())
			{
				unit = it->second;
				break;
			}
		return unit;
	}
	
	boost::shared_ptr<StorageUnit> UnitManager::FindUnit(std::string presId)
	{
		boost::shared_ptr<StorageUnit> unit;
		for (UnitIterator it = unitList.begin(); it != unitList.end(); ++it)
		{
			if (it->second->IsEmpty())
				continue;
			if (it->second->GetPresId() == presId)
			{
				unit = it->second;
				break;
			}
		}
		return unit;
	}
	
	boost::shared_ptr<StorageUnit> UnitManager::FindUnit(std::uint16_t id)
	{
		boost::shared_ptr<StorageUnit> unit;
		UnitIterator it = unitList.find(id);
		if (it != unitList.end())
			unit = it->second;
		return unit;
	}
	
	void UnitManager::Add(std::uint16_t id, boost::shared_ptr<StorageUnit> unit) 
	{
		unitList[id] = unit; 
	}
}
