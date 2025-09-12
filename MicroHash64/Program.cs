using System;
using System.Text;
using BenchmarkDotNet.Running;

namespace MicroHash
{
    class Program
    {
        /// <summary>
        /// Test input strings for hashing.
        /// </summary>
        public static string[] testInputs = new string[]
        {
            "Hello, World!",
            "The quick brown fox jumps over the lazy dog",
            "",
            "a",
            "abc",
            "        ",
            "abcdefghijklmnopqrstuvwxyz",
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
            "0000000000000000000000000000000000000000000000000000000000000000",
            "1111111111111111111111111111111111111111111111111111111111111111",
            "123456789012345678901234567890",
            "0101010101010101010101010101010101010101010101010101010101010101",
            "0101011101010111010101010101011101010111000101010001110101010100"
    };

        /// <summary>
        /// Main entry point of the program.
        /// </summary>
        /// <param name="args"></param>
        static void Main(string[] args)
        {
            bool benchmark = false;
            bool tests = false;
            bool coltest = true;
            if (benchmark)
            {
                BenchmarkRunner.Run<HashBenchmarks>();
            }
            else if (coltest)
            {
                HashBenchmarks.ColTest();
            }
            else if (tests)
            {
                foreach (var input in testInputs)
                {
                    byte[] data = Encoding.UTF8.GetBytes(input);
                    ulong hash = Microhash64.ComputeHash(data);
                    Console.WriteLine($"MicroHash64(\"{input}\")\t= 0x{hash:X16}");
                }
            }
            else
            {
                string input = "";
                if (args.Length != 1)
                {
                    Console.WriteLine("Usage: MicroHash \"your string here\"");

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
                    Console.WriteLine("Enter a string to hash using MicroHash64:");
                    input = Console.ReadLine() ?? "";
                    if (String.IsNullOrEmpty(input))
                    {
                        Console.WriteLine("No input provided. Exiting.");
                        input = "Hello, World!"; // Default value if no input is given
                    }
                }


                byte[] data = Encoding.UTF8.GetBytes(input);
                byte[] runonce = data;

                // Run 3 times
                for (int i = 0; i < 3; i++)
                {
                    runonce = BitConverter.GetBytes(Microhash64.ComputeHash(runonce));
                }
                ulong hash = Microhash64.ComputeHash(data);

                Console.WriteLine($"MicroHash64(\"{input}\") = 0x{hash:X16}");
            }
        }

        /// <summary>
        /// Public method to hash byte array data and print the result.
        /// </summary>
        /// <param name="data"></param>
        public static void Hash(byte[] data)
        {
            ulong hash = Microhash64.ComputeHash(data);
            Console.WriteLine(hash);
        }
    }
}
