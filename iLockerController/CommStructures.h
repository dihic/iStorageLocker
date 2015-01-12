#ifndef _COMM_STRUCTURES_H
#define _COMM_STRUCTURES_H

#include "TypeInfoBase.h"
#include <cstdint>

namespace IntelliStorage
{
	enum CodeType
	{
		HeartBeatCode = 0x00,          //class HeartBeat
		//Calibration 	= 0x01,     //class SensorFlags
		//SetMedicineInfo = 0x02,   //class MedicineInfo
		//ResetZero = 0x03,       //class NodeQuery
		QueryNodeId = 0x04,     //class NodeList
		//QueryQuantity = 0x05,   //class QuantityResult
		//QueryAdCount = 0x06,    //class AdValueList
		//QueryMedicineInfo = 0x07, //class MedicineInfo
		//QueryRamp = 0x08,       //class RampArray
		//SetSensorDisable = 0x09,//class SensorFlags
		//SetDisplayMap = 0x0A, 
		//SetPrescript = 0x0B,    //class Prescription
		//ClearPrescript = 0x0C,  //class Prescription
		//SetCalWeight = 0x0D,    //class CalibrationWeight
		RfidDataCode = 0x0E,        //class RfidData
		CommandResponse = 0x14,
		
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

	DECLARE_CLASS(RfidData)
	{
		public:
			RfidData()
			{
				REGISTER_FIELD(NodeId);
				REGISTER_FIELD(State);
				REGISTER_FIELD(CardId);
				REGISTER_FIELD(PresId);
			}
			virtual ~RfidData() {}
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
	
	class CommStructures
	{
		public:
			static void Register();
	};
}
#endif

