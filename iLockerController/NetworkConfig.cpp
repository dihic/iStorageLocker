#include "NetworkConfig.h"
#include <cstring>
#include <rl_net_lib.h>
#include "System.h"


using namespace std;

extern ETH_CFG eth0_config;
extern LOCALM nlocalm[];
extern LOCALM localm[];
extern char lhost_name[];

#define MAC_ADDRESS 		 0x10
#define ENDPOINT_ADDRESS 0x16
#define IPCONFIG_ADDRESS 0x20

static const LOCALM *IpconfigRom = (const LOCALM *)(USER_ADDR+IPCONFIG_ADDRESS);
static const uint8_t *EndpointRom = (const uint8_t *)(USER_ADDR+ENDPOINT_ADDRESS);
static const uint8_t *MacRom = (const uint8_t *)(USER_ADDR+MAC_ADDRESS);

#define IDENTIFIER_0 0xAA
#define IDENTIFIER_1 0xAB
#define IDENTIFIER_2 0xAC
#define IDENTIFIER_3 0xAD
	
//extern ARM_DRIVER_UART *DebugUart;
//extern ARM_DRIVER_USART Driver_UART1;
	
NetworkConfig::NetworkConfig(ConfigComm *u)
	: comm(*u)
{
	comm.OnCommandArrivalEvent.bind(this, &NetworkConfig::CommandArrival);
	Init();
}

void NetworkConfig::Init()
{
	bool firstUse = false;
	if (UserFlash[0]!=IDENTIFIER_0)
		firstUse=true;
	else if (UserFlash[1]!=IDENTIFIER_1)
		firstUse=true;
	else if (UserFlash[2]!=IDENTIFIER_2)
		firstUse=true;
	else if (UserFlash[3]!=IDENTIFIER_3)
		firstUse=true;
	if (firstUse)
	{
		HAL_FLASH_Unlock();
		EraseFlash();
		HAL_FLASH_Program(TYPEPROGRAM_BYTE, USER_ADDR  , IDENTIFIER_0);
		HAL_FLASH_Program(TYPEPROGRAM_BYTE, USER_ADDR+1, IDENTIFIER_1);
		HAL_FLASH_Program(TYPEPROGRAM_BYTE, USER_ADDR+2, IDENTIFIER_2);
		HAL_FLASH_Program(TYPEPROGRAM_BYTE, USER_ADDR+3, IDENTIFIER_3);
		GenerateMacAddress();
		uint8_t *ip = reinterpret_cast<uint8_t *>(&nlocalm[0]);
		for(uint32_t i=0; i<sizeof(LOCALM); ++i)
			HAL_FLASH_Program(TYPEPROGRAM_BYTE, USER_ADDR+IPCONFIG_ADDRESS+i, ip[i]);
		serviceEndpoint[0]=192;
		serviceEndpoint[1]=168;
		serviceEndpoint[2]=0;
		serviceEndpoint[3]=1;
		serviceEndpoint[4]=0x40;
		serviceEndpoint[5]=0x1F;
		for(int i=0;i<6;++i)
			HAL_FLASH_Program(TYPEPROGRAM_BYTE, USER_ADDR+ENDPOINT_ADDRESS+i, serviceEndpoint[i]);
//		for(uint32_t i=0;i<256;++i)
//			HAL_FLASH_Program(TYPEPROGRAM_BYTE, CARD_ADDR+(i<<8), 0);
		HAL_FLASH_Lock();
	}
	else
	{
		memcpy(eth0_config.MacAddr, MacRom, 6);
		memcpy(serviceEndpoint, EndpointRom, 6);
		memcpy(&nlocalm[0], IpconfigRom, sizeof(LOCALM));
	}
}

const uint8_t *NetworkConfig::GetIpConfig(IpConfigItem item)
{
	const uint8_t *result = NULL;
	switch (item)
	{
//		case IpConfigSetDHCPEnable:
//			break;
		case IpConfigGetIpAddress:
			result = localm[0].IpAddr;
			break;
		case IpConfigGetMask:
			result = localm[0].NetMask;
			break;
		case IpConfigGetGateway:
			result = localm[0].DefGW;
			break;
		case IpConfigGetServiceEnpoint:
			result = serviceEndpoint;
			break;
		default:
			break;
	}
	return result;
}

void NetworkConfig::SetIpConfig(IpConfigItem item, const uint8_t *data)
{
	switch (item)
	{
//		case IpConfigSetDHCPEnable:
//			break;
		case IpConfigSetIpAddress:
			if (memcmp(localm[0].IpAddr,data,4))
				memcpy(localm[0].IpAddr,data,4);
			else
				return;
			break;
		case IpConfigSetMask:
			if (memcmp(localm[0].NetMask,data,4))
				memcpy(localm[0].NetMask,data,4);
			else
				return;
			break;
		case IpConfigSetGateway:
			if (memcmp(localm[0].DefGW,data,4))
				memcpy(localm[0].DefGW,data,4);
			else
				return;
			break;
		case IpConfigSetServiceEnpoint:
			if (memcmp(serviceEndpoint,data,6))
				memcpy(serviceEndpoint,data,6);
			else
				return;
			break;
		default:
			return;
	}
	
	HAL_FLASH_Unlock();
	PrepareWriteFlash(ENDPOINT_ADDRESS, 8+sizeof(LOCALM));
	uint8_t *ip = reinterpret_cast<uint8_t *>(&localm[0]);
	for(uint32_t i=0;i<6;++i)
		HAL_FLASH_Program(TYPEPROGRAM_BYTE, USER_ADDR+ENDPOINT_ADDRESS+i, serviceEndpoint[i]);
	for(uint32_t i=0; i<sizeof(LOCALM); ++i)
		HAL_FLASH_Program(TYPEPROGRAM_BYTE, USER_ADDR+IPCONFIG_ADDRESS+i, ip[i]);
	HAL_FLASH_Lock();
	if (ServiceEndpointChangedEvent)
		ServiceEndpointChangedEvent(serviceEndpoint);
}

void NetworkConfig::GenerateMacAddress()
{
	std::uint32_t mac = GET_RANDOM_NUMBER;
	memcpy(eth0_config.MacAddr+3, &mac, 3);
	//HAL_FLASH_Unlock();
	for(uint32_t i=0;i<6;++i)
		HAL_FLASH_Program(TYPEPROGRAM_BYTE, USER_ADDR+MAC_ADDRESS+i, eth0_config.MacAddr[i]);
	//HAL_FLASH_Lock();
}

void NetworkConfig::CommandArrival(std::uint8_t command,std::uint8_t *parameters,std::size_t len)
{
	IpConfigItem item = IpConfigItem(command);
	uint8_t ack = 0;
	switch (item)
	{
		case IpConfigGetIpAddress:
		case IpConfigGetMask:
		case IpConfigGetGateway:
			comm.SendData(item, GetIpConfig(item), 4);
			break;
		case IpConfigGetServiceEnpoint:
			comm.SendData(IpConfigGetServiceEnpoint, GetIpConfig(IpConfigGetServiceEnpoint), 6);
			break;
		case IpConfigSetIpAddress:
		case IpConfigSetMask:
		case IpConfigSetGateway:
			if (len == 4)
				SetIpConfig(item, parameters);
			else
				ack = 1;
			comm.SendData(item, &ack, 1);
			break;
		case IpConfigSetServiceEnpoint:
			if (len == 6)
				SetIpConfig(IpConfigSetServiceEnpoint, parameters);
			else
				ack = 1;
			comm.SendData(item, &ack, 1);
			break;
		case IpConfigWhoAmI:
			ack = 0xaa;
			comm.SendData(IpConfigWhoAmI, NULL, 0);
			break;
//		case IpConfigDebugOn:
//			DebugUart = &Driver_UART1;
//			cout<<"Debug On"<<endl;
//			break;
//		case IpConfigDebugOff:
//			cout<<"Debug Off"<<endl;
//			DebugUart = NULL;
//			break;
	}
}
