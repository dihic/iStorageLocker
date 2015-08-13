using System;
using System.IO;
using System.IO.Ports;
using System.Text;
using System.Threading;

namespace FlashFS
{
    public enum CommandType : byte
    {
        Format = 0xc0,
        CreateNew = 0xc1,
        Access = 0xc2,
        Erase = 0xc3,
        Write = 0xc4,
        Read = 0xc5,
        Info = 0xcf,
    };

    class FsComm
    {
        private enum StateType : byte
        {
            Delimiter1,
            Delimiter2,
            FileCommand,
            FileDataLength,
            FileData,
            Checksum,
        };

        private readonly SerialPort serial;
        //public event DataArrival DataArrivalEvent;
        private CommandType resultCommand;
        private byte[] resultData;

        public FsComm(string port)
        {
            serial = new SerialPort(port, 115200, Parity.None, 8, StopBits.One)
            {
                Handshake = Handshake.None, //Handshake not needed   
            };
            serial.DataReceived += SerialDataReceived;
            serial.Open();
            serial.DiscardOutBuffer();
            serial.DiscardInBuffer();
        }

        private StateType dataState = StateType.Delimiter1;
        private byte sumcheck;
        private byte command;
        private ushort readIndex;
        private ushort readLength;
        private static readonly byte[] Header = {0xfe, 0xfa, 0xfb};
        private readonly AutoResetEvent syncEvent = new AutoResetEvent(false);

        private byte[] recieveData;

        private void SerialDataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            var count = serial.BytesToRead;
            while (count > 0)
            {
                var b = (byte)serial.ReadByte();
                switch (dataState)
                {
                    case StateType.Delimiter1:
                        if (b == Header[0])
                            dataState = StateType.Delimiter2;
                        break;
                    case StateType.Delimiter2:
                        if (b == Header[2])
                        {
                            dataState = StateType.FileCommand;
                            sumcheck = (byte)(Header[0] + Header[2]);
                        }
                        else
                            dataState = StateType.Delimiter1;
                        break;
                    case StateType.FileCommand:
                        sumcheck += b;
                        command = b;
                        readIndex = 0;
                        readLength = 0;
                        dataState = StateType.FileDataLength;
                        break;
                    case StateType.FileDataLength:
                        sumcheck += b;
                        readLength |= (ushort)(b << (readIndex << 3));
                        if (++readIndex > 1)
                        {
                            if (readLength == 0)
                            {
                                dataState = StateType.Checksum;
                            }
                            else
                            {
                                dataState = StateType.FileData;
                                recieveData = new byte[readLength];
                                readIndex = 0;
                            }
                        }
                        break;
                    case StateType.FileData:
                        sumcheck += b;
                        recieveData[readIndex++] = b;
                        if (readIndex >= readLength)
                            dataState = StateType.Checksum;
                        break;
                    case StateType.Checksum:
                        if (sumcheck == b)
                        {
                            resultCommand = (CommandType)command;
                            resultData = recieveData;
                            syncEvent.Set();
                            //if (DataArrivalEvent != null)
                            //    DataArrivalEvent((CommandType) command, recieveData);
                        }
                        dataState = StateType.Delimiter1;
                        break;
                }
                count--;
            }
        }

        public byte[] SendCommand(CommandType code, byte[] data = null)
        {
            Send(code, data);
            syncEvent.WaitOne(Timeout.Infinite);
            return resultCommand == code ? resultData : null;
        }

        private void Send(CommandType com, byte[] data)
        {
            var len = 0;
            if (data != null)
                len = data.Length;
            var pre = new[] {Header[0], Header[1], (byte) com, (byte) (len & 0xff), (byte) ((len >> 8) & 0xff)};
            byte checksum = 0;
            for (var i = 0; i < 5; i++)
                checksum += pre[i];
            for (var i = 0; i < len; i++)
                if (data != null) 
                    checksum += data[i];
            serial.Write(pre, 0, 5);
            if (len > 0 && data != null)
                serial.Write(data, 0, len);
            pre[0] = checksum;
            serial.Write(pre, 0, 1);
        }
    }
}

