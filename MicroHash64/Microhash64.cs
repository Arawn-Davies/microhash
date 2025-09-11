using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MicroHash
{
    internal class Microhash64
    {
        public static ulong ComputeHash(byte[] data)
        {
            // Simple non-cryptographic hash function
            // State initialization
            uint[] state = new uint[2] { 0x243F6A88, 0x85A308D3 };
            // Process input in 32-byte blocks with padding
            const int blockSize = 32;
            // Calculate the padded length
            int paddedLength = ((data.Length + 5 + blockSize - 1) / blockSize) * blockSize;
            // Create a block buffer
            byte[] block = new byte[blockSize];
            // Process each block
            for (int offset = 0; offset < paddedLength; offset += blockSize)
            {
                // Fill the block with data or padding
                for (int i = 0; i < blockSize; i++)
                {
                    int index = offset + i;
                    // If index is within data length, copy data
                    if (index < data.Length)
                    {
                        block[i] = data[index];
                    }
                    // If index is at data length, append 0x80
                    else if (index == data.Length)
                    {
                        block[i] = 0x80;
                    }
                    // If index is within the last 4 bytes, append length
                    else if (index >= paddedLength - 4)
                    {
                        int shift = 8 * (blockSize - 1 - i);
                        block[i] = (byte)((data.Length >> shift) & 0xFF);
                    }
                    // Otherwise, pad with zeros
                    else
                    {
                        block[i] = 0x00;
                    }
                }

                // For each 4-byte word in the block
                for (int i = 0; i < 4; i++)
                {
                    // Convert 4 bytes to a uint
                    uint word = BitConverter.ToUInt32(block, i * 4);
                    // Rotate left and mix into state
                    state[0] = RotateLeft(state[0] ^ word, 5) + state[1];
                    state[1] = RotateLeft(state[1] + word, 11) ^ state[0];
                }
            }
            // XOR state with rotated values for finalization
            uint final = state[0] ^ RotateLeft(state[1], 3);
            // Combine to form 64-bit hash
            return ((ulong)final << 32) | state[1];
        }

        // Rotate left utility
        static uint RotateLeft(uint value, int bits)
        {
            return (value << bits) | (value >> (32 - bits));
        }
    }
}
