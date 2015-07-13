#include "CanDevice.h"

#define SYNC_TIME 500

using namespace std;

osSemaphoreDef(semaCanDevice);
	
//int32_t CanDevice::SyncSignalId = 1;
map<uint32_t, osThreadId> CanDevice::SyncTable;

bool CanDevice::WorkThreadDefInited = false;

osSemaphoreId CanDevice::semaphore;
osThreadDef_t CanDevice::WorkThreadDef;

CanDevice::CanDevice(boost::weak_ptr<CANExtended::CanEx> ex, std::uint16_t deviceId)
	: CANExtended::ICanDevice(deviceId), canex(ex), busy(false)
{
	if (!WorkThreadDefInited)
	{
		semaphore = osSemaphoreCreate(osSemaphore(semaCanDevice), 4);
//		WorkThreadDef.reset(new osThreadDef_t);
		WorkThreadDef.pthread = CanWorkThread;
		WorkThreadDef.tpriority = osPriorityNormal;
		WorkThreadDef.instances = 4;
		WorkThreadDef.stacksize = 4096;
		WorkThreadDefInited = true;
	}
}

void CanDevice::CanWorkThread(void const *arg)
{
	osThreadId threadid = osThreadGetId();
	
	osSignalWait(0x100, osWaitForever);	//Wait for init
	osSignalClear(threadid, 0x100);
	
	WorkThreadArgs *wta = (WorkThreadArgs *)(arg);
	if (wta == NULL)
		return;
	boost::shared_ptr<CanDevice> device = wta->Device.lock();
	//CanDevice *device = wta->Device;
	if (device == nullptr)
	{
		delete wta;	//Release wta
		return;
	}
	uint16_t attr = wta->Attr;
	uint8_t isWriteCommand = wta->IsWriteCommand;
	
	boost::shared_ptr<CANExtended::OdEntry> entry(new CANExtended::OdEntry(attr, isWriteCommand));
	if (isWriteCommand)
		entry->SetVal(wta->Data, wta->DataLen);
	else
		entry->SetVal(&isWriteCommand, 1);
	
	delete wta;	//Release wta
	
	osEvent result;
	uint8_t tryCount = 3;
	auto ex = device->canex.lock();
	if (ex == nullptr)
		return;
	do
	{
		ex->Request(device->DeviceId, *entry);	
		result = osSignalWait(0xff, SYNC_TIME);
		--tryCount;
	} while (result.status == osEventTimeout && tryCount>0); // Wait for response

#ifdef DEBUG_PRINT
	if (tryCount==0)
		cout<<"CAN Timeout!"<<endl;
#endif
	
	if (result.status == osEventSignal)
		osSignalClear(threadid, 0xff);
	
	entry.reset();
	
	if (isWriteCommand)
	{
		if (device->WriteCommandResponse)
			device->WriteCommandResponse(*device, attr, result.status == osEventSignal);
	}
	else
	{
		if (device->EntryBuffer.get()!=NULL)
		{
			if (device->ReadCommandResponse)
				device->ReadCommandResponse(*device, attr, device->EntryBuffer->GetVal(), device->EntryBuffer->GetLen());
			device->EntryBuffer.reset();
		}
		else
			if (device->ReadCommandResponse)
			{
				boost::shared_ptr<uint8_t[]> dump;
				device->ReadCommandResponse(*device, attr, dump, 0);
			}
	}
	SyncTable.erase((device->DeviceId<<16)|attr);
	device->busy = false;
	osSemaphoreRelease(semaphore);
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
	osSemaphoreWait(semaphore, osWaitForever);
	WorkThreadArgs *args = new WorkThreadArgs(This(), attr, 0xff, false);
	osThreadId tid = osThreadCreate(&WorkThreadDef, args);

//	while (tid==NULL)
//	{
//		osDelay(100);
//		tid = osThreadCreate(&WorkThreadDef, args);
//	}
	
	SyncTable[(DeviceId<<16)|attr] = tid;
	osSignalSet(tid, 0x100);			//Continue work thread after recorded in SyncTable
}

void CanDevice::WriteAttribute(uint16_t attr,const boost::shared_ptr<std::uint8_t[]> &data, size_t size)
{
	int8_t count=3;
	while (SyncTable.find((DeviceId<<16)|attr) != SyncTable.end())
	{
		if (--count<=0)
			break;
		osDelay(SYNC_TIME);
	}
	if (count<=0 && WriteCommandResponse != NULL)
	{
#ifdef DEBUG_PRINT
		cout<<"CAN Write Timeout!"<<endl;
#endif
		WriteCommandResponse(*this, attr, false);
		return;
	}
	
	busy = true;
	osSemaphoreWait(semaphore, osWaitForever);
	WorkThreadArgs *args = new WorkThreadArgs(This(), attr, 0xff, true);
	args->Data = data;
	args->DataLen = size;
	osThreadId tid = osThreadCreate(&WorkThreadDef, args);
	
//	while (tid==NULL)
//	{
//		osDelay(100);
//		tid = osThreadCreate(&WorkThreadDef, args);
//	}
	
	SyncTable[(DeviceId<<16)|attr] = tid;
	osSignalSet(tid, 0x100);
}

void CanDevice::ResponseRecievedEvent(boost::shared_ptr<CANExtended::OdEntry> &entry)
{
	auto it = SyncTable.find((DeviceId<<16)|entry->Index);
	if (it !=  SyncTable.end())
	{
		EntryBuffer = entry;
		osSignalSet(it->second, 0xff);
	}
}

