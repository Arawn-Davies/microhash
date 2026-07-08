#include "microhash_ng.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>

namespace {

uint64_t hashString(const std::string& s) {
    return MicroHashNG::hashPipe::ComputeHash(
        reinterpret_cast<const uint8_t*>(s.data()), s.size());
}

struct TestVector { const char* input; uint64_t expected; };

const TestVector kTestVectors[] = {
    {"Hello, World!",                                                    0x3BC7B2EA7D9D9143ull},
    {"The quick brown fox jumps over the lazy dog",                      0x0BC09723C9A7F509ull},
    {"",                                                                 0x40D6DE95FA68D791ull},
    {"a",                                                                0xD04B9EC77726AB0Full},
    {"abc",                                                              0x8D4B24AB0DD63EDBull},
    {"        ",                                                         0xCF7285AB13D90778ull},
    {"abcdefghijklmnopqrstuvwxyz",                                       0x2008399202128668ull},
    {"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",   0x1662B9BA9DAC92CDull},
    {"0000000000000000000000000000000000000000000000000000000000000000", 0x81E4F9F09184C9CAull},
    {"1111111111111111111111111111111111111111111111111111111111111111", 0xA5046C4D639E45C6ull},
    {"123456789012345678901234567890",                                   0x96F4DBA8A6596732ull},
    {"0101010101010101010101010101010101010101010101010101010101010101", 0x9FEACB10CEA370AEull},
    {"0101011101010111010101010101011101010111000101010001110101010100", 0x38F625D205173523ull},
};

int runTests() {
    int failures = 0;
    for (const auto& v : kTestVectors) {
        uint64_t actual = hashString(v.input);
        bool pass = actual == v.expected;
        if (!pass) failures++;
        printf("[%s] microhash-ng(\"%s\") = 0x%016llX\n",
               pass ? "PASS" : "FAIL", v.input, (unsigned long long)actual);
    }
    printf(failures == 0 ? "All tests passed.\n" : "%d test(s) FAILED.\n", failures);
    return failures == 0 ? 0 : 1;
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc > 1 && std::strcmp(argv[1], "--test") == 0) {
        return runTests();
    }

    std::string input;
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            if (i > 1) input += ' ';
            input += argv[i];
        }
    } else {
        std::cout << "Enter a string to hash using microhash-ng:\n> ";
        std::getline(std::cin, input);
    }

    printf("microhash-ng(\"%s\") = 0x%016llX\n",
           input.c_str(), (unsigned long long)hashString(input));
    return 0;
}
