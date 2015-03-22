#include "CanDevice.h"

#define SYNC_TIME 500

using namespace std;

namespace IntelliStorage
{
	
	int32_t CanDevice::SyncSignalId = 1;
	map<uint32_t, osThreadId> CanDevice::SyncTable;
	
	boost::scoped_ptr<osThreadDef_t> CanDevice::WorkThreadDef;
	
	CanDevice::CanDevice(CANExtended::CanEx &canex, std::uint16_t deviceId)
		: CANExtended::ICanDevice(canex,deviceId), busy(false)
	{		
		if (WorkThreadDef.get()==NULL)
		{
			WorkThreadDef.reset(new osThreadDef_t);
			WorkThreadDef->pthread = WorkThread;
			WorkThreadDef->tpriority = osPriorityNormal;
			WorkThreadDef->instances = 4;
			WorkThreadDef->stacksize = 0;
		}
	}
	
	void CanDevice::WorkThread(void const *arg)
	{
		osThreadId threadid = osThreadGetId();
		
		osSignalWait(0x100, osWaitForever);	//Wait for init
		osSignalClear(threadid, 0x100);
		
		WorkThreadArgs *wta = (WorkThreadArgs *)(arg);
		if (wta == NULL)
			return;
		CanDevice &device = wta->Device;
		uint16_t attr = wta->Attr;
		uint8_t isWriteCommand = wta->IsWriteCommand;
		
		boost::scoped_ptr<CANExtended::OdEntry> entry(new CANExtended::OdEntry(attr, isWriteCommand));
		if (isWriteCommand)
			entry->SetVal(wta->Data, wta->DataLen);
		else
			entry->SetVal(&isWriteCommand, 1);
		
		delete wta;	//Release wta
		
		osEvent result;
		uint8_t tryCount = 3;
		do
		{
			device.canex.Request(device.DeviceId, *entry);	
			result = osSignalWait(0xff, SYNC_TIME);
			--tryCount;
		} while (result.status == osEventTimeout && tryCount>0); // Wait for response
		
		if (tryCount==0)
			cout<<"CAN Timeout!"<<endl;
		
		if (result.status == osEventSignal)
			osSignalClear(threadid, 0xff);
		
		entry.reset();
		
		if (isWriteCommand)
		{
			if (device.WriteCommandResponse)
				device.WriteCommandResponse(device, attr, result.status == osEventSignal);
		}
		else
		{
			if (device.EntryBuffer.get()!=NULL)
			{
				if (device.ReadCommandResponse)
					device.ReadCommandResponse(device, attr, device.EntryBuffer->GetVal(), device.EntryBuffer->GetLen());
				device.EntryBuffer.reset();
			}
			else
				if (device.ReadCommandResponse)
				{
					boost::shared_ptr<uint8_t[]> dump;
					device.ReadCommandResponse(device, attr, dump, 0);
				}
		}
		SyncTable.erase((device.DeviceId<<16)|attr);
		device.busy = false;
	}
	
	void CanDevice::ReadAttribute(uint16_t attr)
	{
		boost::shared_ptr<uint8_t[]> dump;
		if (SyncTable.find((DeviceId<<16)|attr) != SyncTable.end())
		{
			if (ReadCommandResponse)
				ReadCommandResponse(*this, attr, dump, 0);
			return;
		}
		
		busy = true;
		WorkThreadArgs *args = new WorkThreadArgs(*this, attr, 0xff, false);
		osThreadId tid = osThreadCreate(WorkThreadDef.get(), args);
		while (tid==NULL)
		{
			osDelay(100);
			tid = osThreadCreate(WorkThreadDef.get(), args);
		}
		
//		if (tid == NULL)
//		{
//			if (ReadCommandResponse)
//				ReadCommandResponse(*this, attr, dump, 0);
//			busy = false;
//		}
//		else
//		{
			SyncTable[(DeviceId<<16)|attr] = tid;
			osSignalSet(tid, 0x100);			//Continue work thread after recorded in SyncTable
//		}
	}
	
  void CanDevice::WriteAttribute(uint16_t attr,const boost::shared_ptr<std::uint8_t[]> &data, size_t size)
	{
		if (SyncTable.find((DeviceId<<16)|attr) != SyncTable.end())
		{
			if (WriteCommandResponse != NULL)
				WriteCommandResponse(*this, attr, false);
			return;
		}
		
		busy = true;
		WorkThreadArgs *args = new WorkThreadArgs(*this, attr, 0xff, true);
		args->Data = data;
		args->DataLen = size;

		osThreadId tid = osThreadCreate(WorkThreadDef.get(), args);
		
		while (tid==NULL)
		{
			osDelay(100);
			tid = osThreadCreate(WorkThreadDef.get(), args);
		}
		
//		if (tid == NULL)
//		{
//			if (WriteCommandResponse)
//				WriteCommandResponse(*this, attr, false);
//			busy = false;
//		}
//		else
//		{
			SyncTable[(DeviceId<<16)|attr] = tid;
			osSignalSet(tid, 0x100);
//		}
	}
	
	void CanDevice::ResponseRecievedEvent(boost::shared_ptr<CANExtended::OdEntry> entry)
	{
		map<uint32_t, osThreadId>::iterator it = SyncTable.find((DeviceId<<16)|entry->Index);
		if (it !=  SyncTable.end())
		{
			EntryBuffer = entry;
			osSignalSet(it->second, 0xff);
		}
	}
	
	void CanDevice::ProcessRecievedEvent(boost::shared_ptr<CANExtended::OdEntry> entry)
	{
		
	}
}

