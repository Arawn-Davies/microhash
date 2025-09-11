using System;
using System.Text;
using BenchmarkDotNet.Attributes;
using BenchmarkDotNet.Running;

namespace MicroHash
{
    public class HashBenchmarks
    {
        // Inputs of varying sizes

        /// <summary>
        /// Small: 4 bytes of data ("veni", from 'Veni vidi vici')
        /// </summary>
        private byte[] smallInput;
        /// <summary>
        /// Medium: 1 KB of randomized data
        /// </summary>
        private byte[] mediumInput;
        /// <summary>
        /// Large: 1 MB of randomized data
        /// </summary>
        private byte[] largeInput;

        /// <summary>
        /// Benchmark setup to initialize inputs.
        /// </summary>
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

        /// <summary>
        /// Benchmark method for hashing small input.
        /// </summary>
        /// <returns>Hash for small input data, 'veni'.</returns>
        [Benchmark]
        public ulong MicroHash_Small() => Microhash64.ComputeHash(smallInput);

        /// <summary>
        /// Benchmark method for hashing medium input.
        /// </summary>
        /// <returns>Hash for randomized medium input</returns>
        [Benchmark]
        public ulong MicroHash_Medium() => Microhash64.ComputeHash(mediumInput);

        /// <summary>
        /// Benchmark method for hashing large input.
        /// </summary>
        /// <returns>Hash for randomized large input</returns>
        [Benchmark]
        public ulong MicroHash_Large() => Microhash64.ComputeHash(largeInput);

        /// <summary>
        /// Benchmark method to run through predefined test vectors.
        /// </summary>
        [Benchmark]
        public void MicroHash_TestVectors()
        {
            foreach (var input in Program.testInputs)
            {
                byte[] data = Encoding.UTF8.GetBytes(input);
                ulong hash = Microhash64.ComputeHash(data);
                Console.WriteLine($"MicroHash64(\"{input}\")\t= 0x{hash:X16}");
            }
        }
    }
}