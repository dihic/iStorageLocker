#ifndef _COMM_STRUCTURES_H
#define _COMM_STRUCTURES_H

#include "TypeInfoBase.h"
#include <cstdint>

namespace IntelliStorage
{
	enum class CodeType
	{
		HeartBeatCode = 0x00,          //class HeartBeat
		QueryNodeId = 0x04,     //class NodeList
		RfidDataCode = 0x0E,        //class RfidData
		CommandResponse = 0x14,
		QueryRfid = 0x15,
		BarcodeCommand = 0x20,
		CodeWhoAmI = 0xFD,
	};

	DECLARE_CLASS(HeartBeat)
	{
		public:
			HeartBeat()
			{
				REGISTER_FIELD(times);
			}
			virtual ~HeartBeat() {}
			std::uint64_t times;
	};
	
	DECLARE_CLASS(SyetemInfo)
	{
		public:
			SyetemInfo()
			{
				REGISTER_FIELD(Product);
				REGISTER_FIELD(Version);
				REGISTER_FIELD(CpuId);
				REGISTER_FIELD(MacAddress);
			}
			virtual ~SyetemInfo() {}
			std::string Product;
			std::uint32_t Version;
 			std::uint32_t CpuId;
			Binary MacAddress;
	};

	DECLARE_CLASS(NodeQuery)
	{
		public:
			NodeQuery()
			{
				REGISTER_FIELD(NodeId);
			}
			virtual ~NodeQuery() {}
			int NodeId;
	};

	DECLARE_CLASS(NodeList)
	{
		public:
			NodeList()
			{
				REGISTER_FIELD(NodeIds);
			}
			virtual ~NodeList() {}
			Array<int> NodeIds;
	};

	DECLARE_CLASS(RfidDataBson)
	{
		public:
			RfidDataBson()
			{
				REGISTER_FIELD(NodeId);
				REGISTER_FIELD(State);
				REGISTER_FIELD(CardId);
				REGISTER_FIELD(PresId);
			}
			virtual ~RfidDataBson() {}
			int NodeId;
			int State;
			std::string CardId;
			std::string PresId;
	};
	
	DECLARE_CLASS(CommandResult)
	{
		public:
			CommandResult()
			{
				REGISTER_FIELD(NodeId);
				REGISTER_FIELD(Command);
				REGISTER_FIELD(Result);
			}
			virtual ~CommandResult() {}
			int NodeId;
			int Command;
			bool Result;
	};
	
	DECLARE_CLASS(StrCommand)
	{
		public:
			StrCommand()
			{
				REGISTER_FIELD(Command);
			}
			virtual ~StrCommand() {}
			std::string Command;
	};
	
	class CommStructures
	{
		public:
			static void Register();
	};
}
#endif

