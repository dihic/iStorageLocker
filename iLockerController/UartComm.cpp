#include "UartComm.h"

using namespace std;

extern "C"
{
	void UartComm::WorkThread(void const *arg)
	{
		UartComm *comm = (UartComm *)arg;
		while(1)
		{
			comm->DataReceiver();
			osThreadYield();
		}
	}
}

UartComm::UartComm(ARM_DRIVER_USART &u, ARM_USART_SignalEvent_t cb_event, std::uint32_t baudrate, std::size_t bufSize)
	:uart(u), bufferSize(bufSize)
{
	uart.Initialize(cb_event);
	uart.PowerControl(ARM_POWER_FULL);
	uart.Control(ARM_USART_MODE_ASYNCHRONOUS |
							 ARM_USART_DATA_BITS_8 |
							 ARM_USART_PARITY_NONE |
							 ARM_USART_STOP_BITS_1 |
							 ARM_USART_FLOW_CONTROL_NONE, baudrate);
	WorkThreadDef.pthread = WorkThread;
	WorkThreadDef.tpriority = osPriorityNormal;
	WorkThreadDef.instances = 1;
	WorkThreadDef.stacksize = 0;
	buffer = boost::make_shared<uint8_t[]>(bufferSize);
}

UartComm::~UartComm()
{
	Stop();
	uart.Uninitialize();
}

void UartComm::Start()
{
	if (isStarted)
		return;
	uart.Control(ARM_USART_CONTROL_TX, 1);
	uart.Control(ARM_USART_CONTROL_RX, 1);
	head = tail = 0;

	uart.Receive(buffer.get(), bufferSize);
	if (tid == NULL)
		tid = osThreadCreate(&WorkThreadDef, this);
	isStarted = true;
}
	
void UartComm::Stop()
{
	if (!isStarted)
		return;
	if (tid != NULL)
		osThreadTerminate(tid);
	uart.Control(ARM_USART_CONTROL_TX, 0);
	uart.Control(ARM_USART_CONTROL_RX, 0);
	uart.Control(ARM_USART_ABORT_TRANSFER, 0);
	isStarted = false;
}

void UartComm::DataReceiver()
{
	if (!uart.GetStatus().rx_busy)
		return;
	
	tail = uart.GetRxCount();
	if (tail==bufferSize)		//wait for next read to avoid misjudgment
		return;
	
	while (head != tail)
	{
		DataProcess(buffer[head]);
		if (++head>=bufferSize)
			head = 0;
	}
}

void UartComm::Sync()
{
	while (uart.GetStatus().tx_busy)
		__nop();
}
