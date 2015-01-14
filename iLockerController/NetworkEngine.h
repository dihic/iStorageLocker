#ifndef _NETWORK_ENGINE_H
#define _NETWORK_ENGINE_H

#include "TcpClient.h"
#include "CommStructures.h"
#include "StorageUnit.h"
#include <boost/type_traits.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/make_shared.hpp>

namespace IntelliStorage
{
	class NetworkEngine
	{
		private:
			TcpClient tcp;
			std::map<std::uint16_t, boost::shared_ptr<StorageUnit> > &unitList;
			void TcpClientCommandArrival(boost::shared_ptr<std::uint8_t[]> payload, std::size_t size);
		public:
			NetworkEngine(const std::uint8_t *endpoint, std::map<std::uint16_t, boost::shared_ptr<StorageUnit> > &list);
			~NetworkEngine() {}
			void SendHeartBeat();
			void RequestPrescriptionByBleId(std::string &bleId);
			void NotifyBleLeave(std::string &bleId);
			void Process();
			void Connection();
			void InventoryRfid();
			void ChangeServiceEndpoint(const std::uint8_t *endpoint)
			{
				tcp.ChangeServiceEndpoint(endpoint);
			}
			
			void DeviceReadResponse(CanDevice &device,std::uint16_t attr, const boost::shared_ptr<std::uint8_t[]> &data, std::size_t size);
			void DeviceWriteResponse(CanDevice &device,std::uint16_t attr, bool result);
	};
}

#endif
