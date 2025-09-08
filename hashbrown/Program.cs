using System;
using System.Text;

namespace hashbrown
{
    class Program
    {
        static void Main(string[] args)
        {
            string input = "";
            if (args.Length != 1)
            {
                Console.WriteLine("Usage: LiteHash \"your string here\"");
                
            }
            if (args.Length == 1)
            {
                input = args[0];
            }
            else if (args.Length > 1)
            {
                input = string.Join(" ", args);
            }
            if (args.Length == 0)
            {
                Console.WriteLine("Enter a string to hash using LiteHash64:");
                input = Console.ReadLine() ?? "";
                if (String.IsNullOrEmpty(input))
                {
                    Console.WriteLine("No input provided. Exiting.");
                    input = "Hello, World!"; // Default value if no input is given
                }
            }
            
            
            byte[] data = Encoding.UTF8.GetBytes(input);
            ulong hash = LiteHash64(data);

            Console.WriteLine($"LiteHash64(\"{input}\") = 0x{hash:X16}");
        }

        static ulong LiteHash64(byte[] data)
        {
            uint[] state = new uint[2] { 0x243F6A88, 0x85A308D3 };
            const int blockSize = 32;
            int paddedLength = ((data.Length + 5 + blockSize - 1) / blockSize) * blockSize;
            byte[] block = new byte[blockSize];

            for (int offset = 0; offset < paddedLength; offset += blockSize)
            {
                for (int i = 0; i < blockSize; i++)
                {
                    int index = offset + i;
                    if (index < data.Length)
                    {
                        block[i] = data[index];
                    }
                    else if (index == data.Length)
                    {
                        block[i] = 0x80;
                    }
                    else if (index >= paddedLength - 4)
                    {
                        int shift = 8 * (blockSize - 1 - i);
                        block[i] = (byte)((data.Length >> shift) & 0xFF);
                    }
                    else
                    {
                        block[i] = 0x00;
                    }
                }

                for (int i = 0; i < 4; i++)
                {
                    uint word = BitConverter.ToUInt32(block, i * 4);

                    state[0] = RotateLeft(state[0] ^ word, 5) + state[1];
                    state[1] = RotateLeft(state[1] + word, 11) ^ state[0];
                }
            }

            uint final = state[0] ^ RotateLeft(state[1], 3);
            return ((ulong)final << 32) | state[1];
        }

        static uint RotateLeft(uint value, int bits)
        {
            return (value << bits) | (value >> (32 - bits));
        }
    }
}
