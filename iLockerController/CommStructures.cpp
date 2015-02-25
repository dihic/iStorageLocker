#include "CommStructures.h"

namespace IntelliStorage
{
	CLAIM_CLASS(HeartBeat);
	CLAIM_CLASS(NodeQuery);
	CLAIM_CLASS(NodeList);
	CLAIM_CLASS(RfidDataBson);
	CLAIM_CLASS(CommandResult);

	void CommStructures::Register()
	{
		REGISTER_CLASS(HeartBeat);
		REGISTER_CLASS(NodeQuery);
		REGISTER_CLASS(NodeList);
		REGISTER_CLASS(RfidDataBson);
		REGISTER_CLASS(CommandResult);
	}
}
	
