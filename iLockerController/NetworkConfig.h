#ifndef _NETWORK_CONFIG_H
#define _NETWORK_CONFIG_H

#include "ConfigComm.h"
#include "FastDelegate.h"

using namespace fastdelegate;

enum IpConfigItem
{
	IpConfigGetIpAddress =0,
	IpConfigGetMask,
	IpConfigGetGateway,
	IpConfigGetServiceEnpoint,
	//IpConfigGetDHCPEnable,
	IpConfigSetIpAddress = 0x10,
  IpConfigSetMask = 0x11,
  IpConfigSetGateway = 0x12,
  IpConfigSetServiceEnpoint = 0x13,
	//IpConfigSetDHCPEnable = 0x14,
  IpConfigWhoAmI  = 0xaa,
	IpConfigDebugOn = 0xb0,
	IpConfigDebugOff = 0xb1,
};

class NetworkConfig
{
	private:
		uint8_t serviceEndpoint[6];
		ConfigComm &comm;
		void GenerateMacAddress();
		void CommandArrival(std::uint8_t command,std::uint8_t *parameters,std::size_t len);
		void Init();
	public:
		typedef FastDelegate1<const std::uint8_t *> ServiceEndpointChangedHandler;
		ServiceEndpointChangedHandler ServiceEndpointChangedEvent;
		const uint8_t *GetIpConfig(IpConfigItem item);
		void SetIpConfig(IpConfigItem item, const uint8_t *data);
		NetworkConfig(ConfigComm *u);
		~NetworkConfig() {}
};

#endif
