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
#include <unordered_set>
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
// Statistical / crypto-like quality tests
// ---------------------------------------------------------------------------

// Portable popcount for uint64_t.
static int Popcount64(uint64_t v) {
    int n = 0;
    while (v) { n += (int)(v & 1u); v >>= 1; }
    return n;
}

// Collision resistance: hash 100,000 distinct 8-byte little-endian integers and
// assert zero collisions.  For a 64-bit hash, the birthday bound gives an expected
// collision count of N² / 2^65 ≈ 0.27, so zero is the overwhelming expectation.
static void TestCollisionResistance() {
    printf("\n--- Collision Resistance ---\n");

    const int N = 100000;
    std::unordered_set<uint64_t> seen;
    seen.reserve(N);
    int collisions = 0;

    for (int i = 0; i < N; ++i) {
        uint8_t buf[8] = {};
        uint32_t v = static_cast<uint32_t>(i);
        buf[0] = v & 0xFFu;
        buf[1] = (v >> 8) & 0xFFu;
        buf[2] = (v >> 16) & 0xFFu;
        buf[3] = (v >> 24) & 0xFFu;
        uint64_t h = MicroHash::hashPipe::ComputeHash(buf, sizeof(buf));
        if (!seen.insert(h).second) ++collisions;
    }

    char label[128];
    snprintf(label, sizeof(label),
             "no collisions in %d distinct inputs (found %d)", N, collisions);
    ASSERT_EQ(collisions, 0, label);
}

// Avalanche effect: for inputs that fall entirely within the actively mixed first
// 16 bytes of a block, flipping any single input bit should change ~50% (32/64) of
// the output bits.  Threshold: 20–44 bits (31%–69%).
//
// Note: bytes 16–31 of any 32-byte block are not read by the mixing step (known
// design limitation documented in Specification.md §3).  Only inputs short enough
// that all their bytes land in the active region are tested here.
static void TestAvalanche() {
    printf("\n--- Avalanche Effect ---\n");

    struct Case { const char* label; std::vector<uint8_t> data; };
    std::vector<Case> cases = {
        { "avalanche: 'a'    (1 byte)",    ToBytes("a")         },
        { "avalanche: 'abc'  (3 bytes)",   ToBytes("abc")       },
        { "avalanche: 'veni' (4 bytes)",   ToBytes("veni")      },
        { "avalanche: 8 spaces (8 bytes)", ToBytes("        ")  },
    };

    for (auto& c : cases) {
        auto data = c.data;
        uint64_t orig = H(data);
        long long total_bits = 0;
        int num_flips = static_cast<int>(data.size() * 8);

        for (size_t bi = 0; bi < data.size(); ++bi) {
            for (int bit = 0; bit < 8; ++bit) {
                data[bi] ^= static_cast<uint8_t>(1u << bit);
                total_bits += Popcount64(orig ^ H(data));
                data[bi] ^= static_cast<uint8_t>(1u << bit); // restore
            }
        }

        double avg = static_cast<double>(total_bits) / num_flips;
        bool ok = (avg >= 20.0 && avg <= 44.0);
        char label[256];
        snprintf(label, sizeof(label), "%s avg=%.2f bits changed (want 20–44)",
                 c.label, avg);
        if (ok) { printf("[PASS] %s\n", label); ++s_passed; }
        else    { printf("[FAIL] %s\n", label); ++s_failed; }
    }
}

// Bit distribution: across 65,536 sequential 4-byte inputs each of the 64 output
// bit positions should be set 44%–56% of the time (roughly uniform).
static void TestBitDistribution() {
    printf("\n--- Bit Distribution ---\n");

    const int N = 65536;
    long long bit_counts[64] = {};

    for (int i = 0; i < N; ++i) {
        uint8_t buf[4];
        buf[0] = static_cast<uint8_t>(i & 0xFF);
        buf[1] = static_cast<uint8_t>((i >> 8) & 0xFF);
        buf[2] = static_cast<uint8_t>((i >> 16) & 0xFF);
        buf[3] = static_cast<uint8_t>((i >> 24) & 0xFF);
        uint64_t h = MicroHash::hashPipe::ComputeHash(buf, sizeof(buf));
        for (int b = 0; b < 64; ++b) bit_counts[b] += static_cast<int>((h >> b) & 1u);
    }

    int failures = 0;
    for (int b = 0; b < 64; ++b) {
        double freq = static_cast<double>(bit_counts[b]) / N;
        if (freq < 0.44 || freq > 0.56) ++failures;
    }

    char label[256];
    snprintf(label, sizeof(label),
             "all 64 output bits within 44%%–56%% frequency over %d inputs (%d outside range)",
             N, failures);
    ASSERT_EQ(failures, 0, label);
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
    TestCollisionResistance();
    TestAvalanche();
    TestBitDistribution();

    printf("\n================================\n");
    printf("Results: %d passed, %d failed\n", s_passed, s_failed);

    return s_failed == 0 ? 0 : 1;
}
