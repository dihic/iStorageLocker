#ifndef __CAN_DEVICE_H
#define __CAN_DEVICE_H

#include <cstdint>
#include <cstddef>
#include "CanEx.h"
#include <cmsis_os.h>
#include "FastDelegate.h"

using namespace fastdelegate;

namespace IntelliStorage
{
	class CanDevice;
	
	//Arguments structure for processing threads
	struct WorkThreadArgs
	{
		CanDevice &Device;
		const std::uint16_t Attr;
		const std::int32_t SignalId;
		const bool IsWriteCommand;
		boost::shared_ptr<std::uint8_t[]> Data;
		std::size_t DataLen;
		WorkThreadArgs(CanDevice &dev, std::uint16_t attr, std::int32_t signal, bool write)
			:Device(dev), Attr(attr), SignalId(signal), IsWriteCommand(write)
		{
		}
		virtual ~WorkThreadArgs() {}
	};
	
//	struct SignalInfo
//	{
//		osThreadId threadId;
//		std::int32_t signalId;
//	};
	
	class CanDevice : public CANExtended::ICanDevice
	{
		protected:
			boost::shared_ptr<CANExtended::OdEntry> EntryBuffer;
			bool busy;
			void ReadAttribute(std::uint16_t attr);
      void WriteAttribute(std::uint16_t attr, const boost::shared_ptr<std::uint8_t[]> &,std::size_t size);
		private:
			static boost::scoped_ptr<osThreadDef_t> WorkThreadDef;
			static std::int32_t SyncSignalId;
			static std::map<std::uint32_t, osThreadId> SyncTable;
			static void WorkThread(void const *arg);
		public:
			bool IsBusy() const { return busy;}
			typedef FastDelegate3<CanDevice &,std::uint16_t, bool> ResultCB;
			typedef FastDelegate4<CanDevice &,std::uint16_t, const boost::shared_ptr<std::uint8_t[]> &, std::size_t> DataCB;
			DataCB ReadCommandResponse;
			ResultCB WriteCommandResponse;
		
			CanDevice(CANExtended::CanEx &canex, std::uint16_t deviceId);
			
			virtual ~CanDevice() {	}
			
			virtual void ResponseRecievedEvent(boost::shared_ptr<CANExtended::OdEntry> entry);
			virtual void ProcessRecievedEvent(boost::shared_ptr<CANExtended::OdEntry> entry);
		
//			typedef FastDelegate0<> EventHandler;
//			
//			EventHandler DataSyncEvent;
			
			
	};
}

#endif

