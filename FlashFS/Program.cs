using System;
using System.Configuration;
using System.IO;
using System.IO.Ports;
using System.Linq;

namespace FlashFS
{
    class Program
    {
// ReSharper disable once InconsistentNaming
        public readonly FsComm FS;

        private bool SendFile(string path)
        {
            if (!File.Exists(path))
            {
                Console.WriteLine("File not existed");
                return false;
            }
            var file = File.Open(path, FileMode.Open, FileAccess.Read);
            var lenData = new[]
            {
                (byte) (file.Length & 0xff),
                (byte) ((file.Length >> 8) & 0xff),
                (byte) ((file.Length >> 16) & 0xff),
                (byte) ((file.Length >> 24) & 0xff)
            };
            var result = FS.SendCommand(CommandType.CreateNew, lenData);
            if (result == null)
                return false;
            do
            {
                var len = (int)(file.Length - file.Position);
                if (len >= 0x100)
                    len = 0x100;
                var buffer = new byte[len + 4];
                buffer[0] = (byte) (file.Position & 0xff);
                buffer[1] = (byte) ((file.Position >> 8) & 0xff);
                buffer[2] = (byte) ((file.Position >> 16) & 0xff);
                buffer[3] = (byte) ((file.Position >> 24) & 0xff);
                file.Read(buffer, 4, len);
                result = FS.SendCommand(CommandType.Write, buffer);
                if (result == null || result[0] == 0)
                {
                    Console.WriteLine("Write failed");
                    file.Close();
                    return false;
                }
                Console.WriteLine("Write Data: " + file.Position + " Length: " + len);
                //Thread.Sleep(500);
            } while (file.Position < file.Length);
            file.Close();
            return true;
        }

        public Program()
        {
            var port = ConfigurationManager.AppSettings["PORT"];
            var ports = SerialPort.GetPortNames();
            if (ports.Length == 0)
            {
                Console.WriteLine("No available comm port!");
                Console.ReadLine();
                return;
            }
            if (port == "AUTO")
                port = ports[0];
            if (ports.All(p => p != port))
            {
                Console.WriteLine("No available comm port!");
                Console.ReadLine();
                return;
            }
            FS = new FsComm(port);
        }

        static void Main(string[] args)
        {
            if (args.Length == 0)
            {
                Console.WriteLine("No parameters");
                Console.WriteLine("Available commands: ");
                Console.WriteLine("Format - format file system");
                Console.WriteLine("Info - get file system infomation");
                Console.WriteLine("Write filepath - upload a data file");
                return;
            }
            byte[] data;
            var program = new Program();
            CommandType command;
            if (!Enum.TryParse(args[0], out command)) 
                return;
            switch (command)
            {
                case CommandType.Format:
                    data = program.FS.SendCommand(command);
                    if (data == null || data[0] == 0)
                        Console.WriteLine("Format failed");
                    else
                        Console.WriteLine("Format done");
                    break;
                case CommandType.CreateNew:
                case CommandType.Access:
                case CommandType.Erase:
                    Console.WriteLine("Unimplemented command");
                    break;
                case CommandType.Write:
                    if (args.Length >= 2)
                    {
                        if (program.SendFile(args[1]))
                            Console.WriteLine("Upload successful!");
                    }
                    else
                        Console.WriteLine("Missing Filename!");
                    break;
                case CommandType.Read:
                    Console.WriteLine("Unimplemented command");
                    break;
                case CommandType.Info:
                    data = program.FS.SendCommand(command);
                    if (data == null || data.Length < 6)
                        Console.WriteLine("Read info failed");
                    else
                    {
                        Console.WriteLine("Flash " + (data[0] != 0 ? "available" : "unavailable"));
                        Console.WriteLine("File count: " + data[1]);
                        var size = 0x800000 - (data[2] | (data[3] << 8) | (data[4] << 16) | (data[5] << 24));
                        Console.WriteLine("Free space: " + size + " bytes");
                    }
                    break;
                default:
                    Console.WriteLine("Unknown command");
                    //throw new ArgumentOutOfRangeException();
                    break;
            }
        }
    }
}
