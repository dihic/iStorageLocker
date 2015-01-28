#include "SimpleFS.h"

#define TAG1 0x44
#define TAG2 0x53
#define TAG3 0x46
#define TAG4 0x53

SimpleFS::SimpleFS(ARM_DRIVER_SPI *spi, GPIO_TypeDef *cs_port, uint16_t cs_pin, ConfigComm *u)
	: index(0), currentHeader(0), currentSize(0), comm(*u)
{
	comm.OnFileSystemCommandArrivalEvent.bind(this, &SimpleFS::CommandArrival);
	flash.reset(new Spansion::Flash(spi, cs_port, cs_pin));
	boost::shared_ptr<uint8_t []> buffer = boost::make_shared<uint8_t []>(36);
	
	available = flash->IsAvailable();
	
	if (!available)
		return;
	
	flash->FastReadMemory(0, buffer.get(), 36);
	formated = true;
	if (buffer[0] != TAG1)
		formated = false;
	else if (buffer[1] != TAG2)
		formated = false;
	else if (buffer[2] != TAG3)
		formated = false;
	else if (buffer[3] != TAG4)
		formated = false;
	count = 0;
	if (formated)
	{
		for(int i=4;i<36;++i)
			for(int j=0;j<8;++j)
				if ((buffer[i] & (1<<j)) == 0)
					++count;
	}
}

void SimpleFS::CommandArrival(std::uint8_t command,std::uint8_t *parameters,std::size_t len)
{
	uint32_t fileSize;
	uint32_t offset;
	uint8_t index = 1;
	CommandType ct = (CommandType)command;
	boost::shared_ptr<uint8_t []> buffer;
	switch (ct)
	{
		case CommandFormat:
			Format();
			comm.SendFSData(command, &index, 1);
			break;
		case CommandCreateNew:
			if (len == 4)
			{
				fileSize = parameters[0] | (parameters[1]<<8) | (parameters[2]<<16) | (parameters[3]<<24);
				index = CreateNew(fileSize);
			}
			else
				index = 0;
			comm.SendFSData(command, &index, 1);
			break;
		case CommandAccess:
			if (len == 1)
			{
				fileSize = Access(parameters[0]);
				buffer = boost::make_shared<uint8_t []>(4);
				buffer[0] = fileSize & 0xff;
				buffer[1] = (fileSize>>8) & 0xff;
				buffer[2] = (fileSize>>16) & 0xff;
				buffer[3] = (fileSize>>24) & 0xff;
				comm.SendFSData(command, buffer.get(), 4);
			}
			break;
		case CommandErase:
			index = Erase();
			comm.SendFSData(command, &index, 1);
			break;
		case CommandWrite:
			if (len>4)
			{
				offset = parameters[0] | (parameters[1]<<8) | (parameters[2]<<16) | (parameters[3]<<24);
				index = WriteFile(offset, parameters+4, len-4);
			}
			else
				index = 0;
			comm.SendFSData(command, &index, 1);
			break;
		case CommandRead:
			if (len == 6)
			{
				offset = parameters[0] | (parameters[1]<<8) | (parameters[2]<<16) | (parameters[3]<<24);
				fileSize = parameters[4] | (parameters[5]<<8);
				buffer = boost::make_shared<uint8_t []>(fileSize);
				ReadFile(offset, buffer.get(), fileSize);
				comm.SendFSData(command, buffer.get(), fileSize);
			}
			break;
		case CommandInfo:
			buffer = boost::make_shared<uint8_t []>(6);
			buffer[0] = available;
			buffer[1] = count;
			flash->FastReadMemory(36+count*8, buffer.get()+2, 4);
			comm.SendFSData(command, buffer.get(), 6);
			break;
		default:
			break;
	}
}

void SimpleFS::Format()
{
	if (!available)
		return;
	boost::shared_ptr<uint8_t []> buffer = boost::make_shared<uint8_t []>(40);
	buffer[0] = TAG1;
	buffer[1] = TAG2;
	buffer[2] = TAG3;
	buffer[3] = TAG4;
	for(int i=4;i<36;++i)
		buffer[i] = 0xff;
	buffer[36] = 0x00;
	buffer[37] = 0x10;
	buffer[38] = 0x00;
	buffer[39] = 0x00;
 	flash->WriteMemory(0, buffer.get(), 40, true);
	count = 0;
	index = 0;
	formated = true;
}

uint8_t SimpleFS::CreateNew(uint32_t size)
{
	if (!available)
		return 0;
	if (count == 0xff)
		return 0;
	boost::shared_ptr<uint8_t []> buffer = boost::make_shared<uint8_t []>(8);
	
	flash->FastReadMemory(36+count*8, buffer.get(), 4);
	currentHeader = (buffer[3]<<24)|(buffer[2]<<16)|(buffer[1]<<8)|buffer[0];
	currentSize = size;
	buffer[0] = size & 0xff;
	buffer[1] = (size>> 8) & 0xff;
	buffer[2] = (size>>16) & 0xff;
	buffer[3] = (size>>24) & 0xff;
	uint32_t addr = currentHeader+(size & 0xfffff000u);
	if (size & 0xfff)
		addr += 0x1000;
	buffer[4] = addr & 0xff;
	buffer[5] = (addr>> 8) & 0xff;
	buffer[6] = (addr>>16) & 0xff;
	buffer[7] = (addr>>24) & 0xff;
	flash->WriteMemory(36+count*8+4, buffer.get(), 8, false);
	buffer[0] = ~(1 << (count & 0x7));
	flash->WriteMemory(4+(count>>3), buffer.get() , 1, false);
	index = ++count;
	Erase();
	return count;
}

uint32_t SimpleFS::Access(uint8_t file)
{
	if (!available)
		return 0;
	if (file > count || file == 0)
		return 0;
	index = file;
	boost::shared_ptr<uint8_t []> buffer = boost::make_shared<uint8_t []>(8);
	flash->FastReadMemory(36+(index-1)*8, buffer.get(), 8);
	currentHeader = (buffer[3]<<24)|(buffer[2]<<16)|(buffer[1]<<8)|buffer[0];
	currentSize = (buffer[7]<<24)|(buffer[6]<<16)|(buffer[5]<<8)|buffer[4];
	return currentSize;
}

bool SimpleFS::Erase()
{
	if (!available)
		return false;
	if (index == 0)
		return false;
	int32_t size = currentSize;
	uint32_t offset = currentHeader;
	while (size > 0)
	{
		if ((size > 0x10000) && (offset & 0xffff == 0))
		{
			flash->SectorErase((offset>>16)&0x7f);
			offset += 0x10000;
			size -= 0x10000;
		}
		else
		{
			flash->ParameterSectorErase((offset>>12)&0x7ff);
			offset += 0x1000;
			size -= 0x1000;
		}
	}
	return false;
}

bool SimpleFS::WriteFile(uint32_t offset, uint8_t *data, uint16_t size)
{
	if (!available)
		return false;
	if (index == 0)
		return false;
	if (currentHeader+offset+size > MAX_ADDRESS)
		return false;
	flash->WriteMemory(currentHeader+offset, data, size, false);
	return true;
}

bool SimpleFS::ReadFile(uint32_t offset, uint8_t *data, uint16_t size)
{
	if (!available)
		return false;
	if (index == 0)
		return false;
	if (currentHeader+offset+size > MAX_ADDRESS)
		return false;
	flash->FastReadMemory(currentHeader+offset, data, size);
	return true;
}

