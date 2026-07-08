using System;

namespace microhash_ng
{
    /// <summary>
    /// microhash-ng — revised microhash that mixes ALL 32 bytes of every
    /// block (eight 4-byte words instead of four), so the former byte-16-31
    /// dead zone and the encoded length field now influence the digest.
    /// Output is NOT compatible with original microhash.
    /// </summary>
    public static class hashPipeNG
    {
        public static ulong ComputeHash(byte[] data)
        {
            uint[] state = { 0x243F6A88u, 0x85A308D3u };

            const int blockSize = 32;
            int length = data.Length;
            int paddedLen = ((length + 5 + blockSize - 1) / blockSize) * blockSize;

            byte[] block = new byte[blockSize];

            for (int offset = 0; offset < paddedLen; offset += blockSize)
            {
                for (int i = 0; i < blockSize; i++)
                {
                    int index = offset + i;

                    if (index < length)
                        block[i] = data[index];
                    else if (index == length)
                        block[i] = 0x80;
                    else if (index >= paddedLen - 4)
                        block[i] = (byte)((length >> (8 * (blockSize - 1 - i))) & 0xFF);
                    else
                        block[i] = 0x00;
                }

                // ng: all eight words per block (bytes 0-31), not just the first four.
                // Words are assembled with explicit shifts (not BitConverter), so
                // output is identical on big-endian hosts too.
                for (int i = 0; i < 8; i++)
                {
                    int b = i * 4;
                    uint word = block[b]
                              | ((uint)block[b + 1] << 8)
                              | ((uint)block[b + 2] << 16)
                              | ((uint)block[b + 3] << 24);

                    state[0] = RotateLeft(state[0] ^ word, 5) + state[1];
                    state[1] = RotateLeft(state[1] + word, 11) ^ state[0];
                }
            }

            uint finalVal = state[0] ^ RotateLeft(state[1], 3);
            return ((ulong)finalVal << 32) | state[1];
        }

        private static uint RotateLeft(uint value, int bits)
        {
            return (value << bits) | (value >> (32 - bits));
        }
    }
}
