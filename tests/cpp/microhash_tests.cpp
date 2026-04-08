// microhash_tests.cpp
// Test suite for MicroHash C++ implementation, mirroring the benchmark inputs.
//
// Build: g++ -std=c++17 -I../../src/cpp -o microhash_tests microhash_tests.cpp
// Run:   ./microhash_tests

#include "microhash.hpp"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Minimal test harness
// ---------------------------------------------------------------------------

static int s_passed = 0;
static int s_failed = 0;

#define ASSERT_EQ(actual, expected, label)                                     \
    do {                                                                       \
        if ((actual) == (expected)) {                                          \
            printf("[PASS] %s\n", (label));                                    \
            ++s_passed;                                                        \
        } else {                                                               \
            printf("[FAIL] %s\n"                                               \
                   "       expected: 0x%016llX\n"                              \
                   "       actual:   0x%016llX\n",                             \
                   (label),                                                    \
                   (unsigned long long)(expected),                             \
                   (unsigned long long)(actual));                               \
            ++s_failed;                                                        \
        }                                                                      \
    } while (0)

#define ASSERT_NE(a, b, label)                                                 \
    do {                                                                       \
        if ((a) != (b)) {                                                      \
            printf("[PASS] %s\n", (label));                                    \
            ++s_passed;                                                        \
        } else {                                                               \
            printf("[FAIL] %s — values should differ but both = 0x%016llX\n", \
                   (label), (unsigned long long)(a));                          \
            ++s_failed;                                                        \
        }                                                                      \
    } while (0)

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::vector<uint8_t> ToBytes(const char* s) {
    size_t len = strlen(s);
    return std::vector<uint8_t>(reinterpret_cast<const uint8_t*>(s),
                                reinterpret_cast<const uint8_t*>(s) + len);
}

static uint64_t H(const char* s) {
    return MicroHash::hashPipe::ComputeHash(ToBytes(s));
}

static uint64_t H(const std::vector<uint8_t>& v) {
    return MicroHash::hashPipe::ComputeHash(v);
}

// ---------------------------------------------------------------------------
// Test groups
// ---------------------------------------------------------------------------

// Known-good hash values computed from the reference implementation.
static void TestVectors() {
    printf("\n--- Test Vectors ---\n");
    ASSERT_EQ(H("Hello, World!"),                                               0x352256EFEDC72BD1ULL, "Hello, World!");
    ASSERT_EQ(H("The quick brown fox jumps over the lazy dog"),                 0x37876396F9CCB637ULL, "quick brown fox");
    ASSERT_EQ(H(""),                                                            0xFD1FADBB7E12CB96ULL, "empty string");
    ASSERT_EQ(H("a"),                                                           0x9B1F9089AF49253EULL, "single char 'a'");
    ASSERT_EQ(H("abc"),                                                         0x8874CA7BE18B8218ULL, "abc");
    ASSERT_EQ(H("        "),                                                    0xB94BF2A5D5341A60ULL, "eight spaces");
    ASSERT_EQ(H("abcdefghijklmnopqrstuvwxyz"),                                  0x67773BF7A225BE5DULL, "lowercase alphabet");
    ASSERT_EQ(H("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"), 0xCE821AC98900EEA0ULL, "alphanumeric mix");
    ASSERT_EQ(H("0000000000000000000000000000000000000000000000000000000000000000"), 0x3411C1C38205A8E0ULL, "64 zeros");
    ASSERT_EQ(H("1111111111111111111111111111111111111111111111111111111111111111"), 0x067FE50AF384C88EULL, "64 ones");
    ASSERT_EQ(H("123456789012345678901234567890"),                               0xBFD7E4924ACFA323ULL, "numeric sequence");
    ASSERT_EQ(H("0101010101010101010101010101010101010101010101010101010101010101"), 0x7BCC8A21375360E0ULL, "alternating 01 (64)");
    ASSERT_EQ(H("0101011101010111010101010101011101010111000101010001110101010100"), 0xFDFB4707123CF187ULL, "binary-like string");
}

// Benchmark-style inputs: small / medium / large (mirrors Benchmarks.cs).
static void TestBenchmarkInputs() {
    printf("\n--- Benchmark-Style Inputs ---\n");

    // Small: "veni" (4 bytes)
    ASSERT_EQ(H("veni"), 0x8CF2BA1EF820120BULL, "small input 'veni'");

    // Medium: 1 KB filled with i % 256
    std::vector<uint8_t> medium(1024);
    for (int i = 0; i < 1024; ++i) medium[i] = static_cast<uint8_t>(i % 256);
    ASSERT_EQ(H(medium), 0x0B1E05784B4A80E1ULL, "medium input (1 KB, i%256)");

    // Large: 1 MB filled with i % 256
    std::vector<uint8_t> large(1024 * 1024);
    for (int i = 0; i < 1024 * 1024; ++i) large[i] = static_cast<uint8_t>(i % 256);
    ASSERT_EQ(H(large), 0xD1F95722CE050E61ULL, "large input (1 MB, i%256)");
}

// Determinism: same input always produces the same hash.
static void TestDeterminism() {
    printf("\n--- Determinism ---\n");
    const char* inputs[] = {
        "Hello, World!", "", "abc", "veni"
    };
    for (const char* s : inputs) {
        uint64_t h1 = H(s);
        uint64_t h2 = H(s);
        char label[128];
        snprintf(label, sizeof(label), "determinism: \"%s\"", s);
        ASSERT_EQ(h1, h2, label);
    }

    // Determinism via raw-pointer and vector overload
    std::vector<uint8_t> v = ToBytes("repeatme");
    uint64_t h1 = MicroHash::hashPipe::ComputeHash(v.data(), v.size());
    uint64_t h2 = MicroHash::hashPipe::ComputeHash(v);
    ASSERT_EQ(h1, h2, "pointer overload == vector overload");
}

// Sensitivity: different inputs produce different hashes.
static void TestSensitivity() {
    printf("\n--- Sensitivity ---\n");
    ASSERT_NE(H("a"), H("b"),  "a != b");
    ASSERT_NE(H("abc"), H("ABC"), "abc != ABC");
    ASSERT_NE(H("Hello, World!"), H("hello, world!"), "case sensitivity");
    ASSERT_NE(H("ab"), H("ba"), "ab != ba (order matters)");

    // Differ by exactly one byte
    ASSERT_NE(H("aaaa"), H("aaab"), "one-bit-ish difference");

    // Empty vs non-empty
    ASSERT_NE(H(""), H(" "), "empty != space");
    ASSERT_NE(H(""), H("a"), "empty != 'a'");

    // Appending a byte changes the hash
    ASSERT_NE(H("test"), H("test!"), "length sensitivity");
}

// Edge cases: single-byte values, boundary-length strings.
static void TestEdgeCases() {
    printf("\n--- Edge Cases ---\n");

    // All 256 single-byte values should produce distinct hashes (spot-check)
    uint64_t prev = MicroHash::hashPipe::ComputeHash(nullptr, 0);
    for (int b = 0; b < 256; ++b) {
        uint8_t byte = static_cast<uint8_t>(b);
        uint64_t h = MicroHash::hashPipe::ComputeHash(&byte, 1);
        char label[64];
        snprintf(label, sizeof(label), "byte 0x%02X != empty", b);
        ASSERT_NE(h, prev, label);
    }

    // Strings of length exactly 31, 32, 33 (block-boundary sensitivity)
    std::string s31(31, 'x');
    std::string s32(32, 'x');
    std::string s33(33, 'x');
    ASSERT_NE(H(s31.c_str()), H(s32.c_str()), "len 31 != len 32");
    ASSERT_NE(H(s32.c_str()), H(s33.c_str()), "len 32 != len 33");
    ASSERT_NE(H(s31.c_str()), H(s33.c_str()), "len 31 != len 33");
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

int main() {
    printf("=== MicroHash C++ Test Suite ===\n");

    TestVectors();
    TestBenchmarkInputs();
    TestDeterminism();
    TestSensitivity();
    TestEdgeCases();

    printf("\n================================\n");
    printf("Results: %d passed, %d failed\n", s_passed, s_failed);

    return s_failed == 0 ? 0 : 1;
}
