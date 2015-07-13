#ifndef __CAN_DEVICE_H
#define __CAN_DEVICE_H

#include <cstdint>
#include <cstddef>
#include "CanEx.h"
#include <cmsis_os.h>
#include "FastDelegate.h"
#include <boost/enable_shared_from_this.hpp>

#ifdef DEBUG_PRINT
#include <iostream>
#endif

using namespace fastdelegate;

class CanDevice;

//Arguments structure for processing threads
class WorkThreadArgs
{
	public:
		boost::weak_ptr<CanDevice> Device;
	  //CanDevice *Device;
		const std::uint16_t Attr;
		const std::int32_t SignalId;
		const bool IsWriteCommand;
		boost::shared_ptr<std::uint8_t[]> Data;
		std::size_t DataLen;
		WorkThreadArgs(boost::weak_ptr<CanDevice> dev, std::uint16_t attr, std::int32_t signal, bool write)
		//WorkThreadArgs(CanDevice *dev, std::uint16_t attr, std::int32_t signal, bool write)
			:Device(dev), Attr(attr), SignalId(signal), IsWriteCommand(write)
		{
		}
		virtual ~WorkThreadArgs() {}
};

class CanDevice : public CANExtended::ICanDevice, public boost::enable_shared_from_this<CanDevice>
{
	protected:
		boost::weak_ptr<CANExtended::CanEx> canex;
		boost::shared_ptr<CANExtended::OdEntry> EntryBuffer;
		bool busy;
		void ReadAttribute(std::uint16_t attr);
		void WriteAttribute(std::uint16_t attr, const boost::shared_ptr<std::uint8_t[]> &,std::size_t size);
		virtual boost::shared_ptr<CanDevice> This()
		{
			return shared_from_this();
		}
	private:
		static bool WorkThreadDefInited;
		static osSemaphoreId semaphore; 
		static osThreadDef_t WorkThreadDef;
//		static std::int32_t SyncSignalId;
		static std::map<std::uint32_t, osThreadId> SyncTable;
		static void CanWorkThread(void const *arg);
	public:
		bool IsBusy() const { return busy;}
		typedef FastDelegate3<CanDevice &,std::uint16_t, bool> ResultCB;
		typedef FastDelegate4<CanDevice &,std::uint16_t, const boost::shared_ptr<std::uint8_t[]> &, std::size_t> DataCB;
		DataCB ReadCommandResponse;
		ResultCB WriteCommandResponse;
	
		CanDevice(boost::weak_ptr<CANExtended::CanEx> ex, std::uint16_t deviceId);
		
		virtual ~CanDevice() {	}
		
		virtual void ResponseRecievedEvent(boost::shared_ptr<CANExtended::OdEntry> &entry) override;
//		virtual void ProcessRecievedEvent(boost::shared_ptr<CANExtended::OdEntry> &entry) override = 0;
	
//			typedef FastDelegate0<> EventHandler;
//			
//			EventHandler DataSyncEvent;
		
		
};

#endif

