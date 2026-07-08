#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

// microhash-ng — revised microhash, hardened against the SMHasher battery.
//
// Differences from original microhash (src/cpp/microhash.hpp):
//   - The mixing loop consumes eight 4-byte words per block instead of four,
//     so bytes 16-31 (previously a dead zone) now influence the digest, and
//     the big-endian length field in the final padding block is mixed in.
//   - Each word is absorbed with TWO ARX rounds (the second reinjects the
//     word rotated by 16), so a difference passes through two nonlinear
//     rounds before the next word could cancel it. Fixes the sparse-key
//     collisions SMHasher found in single-round absorption.
//   - Four ARX finalization rounds with pi-derived constants diffuse the
//     final words into the whole state. Fixes tail-byte avalanche.
//
// Output is NOT compatible with original microhash. State size, padding
// layout, and the 64-bit digest format are unchanged.
namespace MicroHashNG {

class hashPipe {
public:
    static uint64_t ComputeHash(const uint8_t* data, size_t length) {
        // State initialization
        uint32_t state[2] = { 0x243F6A88u, 0x85A308D3u };

        // Finalization round constants (continuation of pi's fraction)
        static const uint32_t kFinalConst[4] =
            { 0x13198A2Eu, 0x03707344u, 0xA4093822u, 0x299F31D0u };

        const int blockSize = 32;
        const size_t paddedLen = ((length + 5 + blockSize - 1) / blockSize) * blockSize;

        uint8_t block[32] = {};

        for (size_t offset = 0; offset < paddedLen; offset += blockSize) {
            for (int i = 0; i < blockSize; i++) {
                size_t index = offset + (size_t)i;

                if (index < length) {
                    block[i] = data[index];
                } else if (index == length) {
                    block[i] = 0x80u;
                } else if (index >= paddedLen - 4) {
                    int shift = 8 * (blockSize - 1 - i);
                    block[i] = (uint8_t)((length >> shift) & 0xFFu);
                } else {
                    block[i] = 0x00u;
                }
            }

            // ng: all eight words per block, two ARX rounds per word
            for (int i = 0; i < 8; i++) {
                size_t base = (size_t)i * 4;
                uint32_t word =
                    (uint32_t)block[base]            |
                    ((uint32_t)block[base + 1] << 8)  |
                    ((uint32_t)block[base + 2] << 16) |
                    ((uint32_t)block[base + 3] << 24);

                state[0] = RotateLeft(state[0] ^ word, 5) + state[1];
                state[1] = RotateLeft(state[1] + word, 11) ^ state[0];
                state[0] = RotateLeft(state[0] ^ RotateLeft(word, 16), 5) + state[1];
                state[1] = RotateLeft(state[1] + word, 11) ^ state[0];
            }
        }

        for (int r = 0; r < 4; r++) {
            state[0] = RotateLeft(state[0] ^ kFinalConst[r], 5) + state[1];
            state[1] = RotateLeft(state[1] + kFinalConst[r], 11) ^ state[0];
        }

        uint32_t final_val = state[0] ^ RotateLeft(state[1], 3);
        return ((uint64_t)final_val << 32) | (uint64_t)state[1];
    }

    // Convenience overload for std::vector
    static uint64_t ComputeHash(const std::vector<uint8_t>& data) {
        return ComputeHash(data.data(), data.size());
    }

private:
    static uint32_t RotateLeft(uint32_t value, int bits) {
        return (value << bits) | (value >> (32 - bits));
    }
};

} // namespace MicroHashNG
