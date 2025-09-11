using System;
using System.Text;
using BenchmarkDotNet.Attributes;
using BenchmarkDotNet.Running;

namespace hashbrown
{
    public class HashBenchmarks
    {
        private byte[] smallInput;
        private byte[] mediumInput;
        private byte[] largeInput;

        [GlobalSetup]
        public void Setup()
        {
            smallInput = Encoding.UTF8.GetBytes("veni");
            mediumInput = new byte[1024]; // 1 KB
            largeInput = new byte[1024 * 1024]; // 1 MB

            // Fill with some predictable data
            for (int i = 0; i < mediumInput.Length; i++) mediumInput[i] = (byte)(i % 256);
            for (int i = 0; i < largeInput.Length; i++) largeInput[i] = (byte)(i % 256);
        }

        [Benchmark]
        public ulong LiteHash_Small() => Program.MicroHash64(smallInput);

        [Benchmark]
        public ulong LiteHash_Medium() => Program.MicroHash64(mediumInput);

        [Benchmark]
        public ulong LiteHash_Large() => Program.MicroHash64(largeInput);

        [Benchmark]
        public void LiteHash_TestVectors()
        {
            foreach (var input in Program.testInputs)
            {
                byte[] data = Encoding.UTF8.GetBytes(input);
                ulong hash = Program.MicroHash64(data);
                Console.WriteLine($"MicroHash64(\"{input}\")\t= 0x{hash:X16}");
            }
        }
    }
}