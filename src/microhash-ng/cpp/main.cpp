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
