// microhash-ng C++ test suite.
// Golden vectors plus regression tests for the two defects fixed relative
// to original microhash: the byte-16-31 dead zone and the unread length field.
#include "microhash_ng.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

using MicroHashNG::hashPipe;

static int g_passed = 0;
static int g_failed = 0;

#define CHECK(cond, label)                                    \
    do {                                                      \
        if (cond) { g_passed++; }                             \
        else { g_failed++; printf("[FAIL] %s\n", label); }    \
    } while (0)

static uint64_t H(const std::string& s) {
    return hashPipe::ComputeHash(
        reinterpret_cast<const uint8_t*>(s.data()), s.size());
}

static void testVectors() {
    struct { const char* input; uint64_t expected; } vectors[] = {
        {"Hello, World!",                                                    0xA40E5C7D0BFBA07Dull},
        {"The quick brown fox jumps over the lazy dog",                      0x5BD8C52E8C1E2175ull},
        {"",                                                                 0x6CA97D4E1A59E8ECull},
        {"a",                                                                0xD1EF310FB09DC1DCull},
        {"abc",                                                              0x1351FEBF7FEDB189ull},
        {"        ",                                                         0x38AFC965BDFDC9EBull},
        {"abcdefghijklmnopqrstuvwxyz",                                       0xF2A991C82844982Full},
        {"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",   0x7AB7F4398A2A0130ull},
        {"0000000000000000000000000000000000000000000000000000000000000000", 0xBC55379EAAB952BFull},
        {"1111111111111111111111111111111111111111111111111111111111111111", 0x42AF241530C58F18ull},
        {"123456789012345678901234567890",                                   0x7A78EB4902E77E91ull},
        {"0101010101010101010101010101010101010101010101010101010101010101", 0x38232B18B8755FA6ull},
        {"0101011101010111010101010101011101010111000101010001110101010100", 0x29E828BAC44A055Bull},
    };
    for (auto& v : vectors)
        CHECK(H(v.input) == v.expected, v.input);
}

// Regression: original microhash ignored bytes 16-31 of every block.
// Every single-byte flip at every position must change the digest.
static void testNoDeadZones() {
    for (size_t len : {32u, 64u, 256u}) {
        std::vector<uint8_t> base(len);
        for (size_t j = 0; j < len; j++) base[j] = (uint8_t)((j * 7) % 256);
        uint64_t ref = hashPipe::ComputeHash(base);
        for (size_t i = 0; i < len; i++) {
            std::vector<uint8_t> mod = base;
            mod[i] ^= 0xFF;
            CHECK(hashPipe::ComputeHash(mod) != ref, "dead-zone byte flip undetected");
        }
    }
}

// Regression: the specific 64-byte collision pair from the original
// (contents agreeing only on bytes 0-15 of each block) must now differ.
static void testFormerCollisionPair() {
    std::string a = std::string(16, 'A') + std::string(16, 'X') +
                    std::string(16, 'A') + std::string(16, 'Y');
    std::string b = std::string(16, 'A') + std::string(16, 'Z') +
                    std::string(16, 'A') + std::string(16, 'W');
    CHECK(H(a) != H(b), "former dead-zone collision pair still collides");
}

// Regression: the length field (final-block bytes 28-31) must be mixed.
static void testLengthSensitivity() {
    CHECK(H("abc") != H(std::string("abc") + '\0'), "trailing NUL append undetected");
    std::string s27(27, 'x');  // pads within one block
    std::string s26(26, 'x');
    CHECK(H(s27) != H(s26), "length change undetected");
}

static void testDeterminism() {
    for (const char* s : {"", "a", "microhash-ng"})
        CHECK(H(s) == H(s), "non-deterministic digest");
}

static void testVectorOverload() {
    std::vector<uint8_t> v = {'a', 'b', 'c'};
    CHECK(hashPipe::ComputeHash(v) == H("abc"), "vector overload mismatch");
}

int main() {
    printf("=== MicroHash-NG C++ Test Suite ===\n\n");
    testVectors();
    testNoDeadZones();
    testFormerCollisionPair();
    testLengthSensitivity();
    testDeterminism();
    testVectorOverload();
    printf("Results: %d passed, %d failed\n", g_passed, g_failed);
    return g_failed == 0 ? 0 : 1;
}
