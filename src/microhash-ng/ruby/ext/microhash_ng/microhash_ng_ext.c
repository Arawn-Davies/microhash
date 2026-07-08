/*
 * MicroHashNG native extension — same algorithm as
 * src/microhash-ng/cpp/microhash_ng.hpp (all 8 words per block, two ARX
 * rounds per word, four finalization rounds).
 * Exposes MicroHashNG.native_compute_hash(String) -> Integer.
 */
#include "ruby.h"
#include <stddef.h>
#include <stdint.h>

static uint32_t rotate_left(uint32_t value, int bits)
{
    return (value << bits) | (value >> (32 - bits));
}

static const uint32_t final_constants[4] =
    { 0x13198A2Eu, 0x03707344u, 0xA4093822u, 0x299F31D0u };

static uint64_t compute_hash(const uint8_t *data, size_t length)
{
    uint32_t state[2] = { 0x243F6A88u, 0x85A308D3u };

    const int blockSize = 32;
    const size_t paddedLen = ((length + 5 + blockSize - 1) / blockSize) * blockSize;

    uint8_t block[32];

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

        /* ng: all eight words per block, two ARX rounds per word */
        for (int i = 0; i < 8; i++) {
            size_t base = (size_t)i * 4;
            uint32_t word =
                (uint32_t)block[base]             |
                ((uint32_t)block[base + 1] << 8)  |
                ((uint32_t)block[base + 2] << 16) |
                ((uint32_t)block[base + 3] << 24);

            state[0] = rotate_left(state[0] ^ word, 5) + state[1];
            state[1] = rotate_left(state[1] + word, 11) ^ state[0];
            state[0] = rotate_left(state[0] ^ rotate_left(word, 16), 5) + state[1];
            state[1] = rotate_left(state[1] + word, 11) ^ state[0];
        }
    }

    for (int r = 0; r < 4; r++) {
        state[0] = rotate_left(state[0] ^ final_constants[r], 5) + state[1];
        state[1] = rotate_left(state[1] + final_constants[r], 11) ^ state[0];
    }

    uint32_t final_val = state[0] ^ rotate_left(state[1], 3);
    return ((uint64_t)final_val << 32) | (uint64_t)state[1];
}

static VALUE mh_native_compute_hash(VALUE self, VALUE str)
{
    StringValue(str);
    uint64_t digest = compute_hash((const uint8_t *)RSTRING_PTR(str),
                                   (size_t)RSTRING_LEN(str));
    return ULL2NUM(digest);
}

void Init_microhash_ng_ext(void)
{
    VALUE mMicroHashNG = rb_define_module("MicroHashNG");
    rb_define_module_function(mMicroHashNG, "native_compute_hash",
                              mh_native_compute_hash, 1);
}
