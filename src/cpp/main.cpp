#include "microhash.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::vector<uint8_t> ToBytes(const char* s) {
    size_t len = strlen(s);
    return std::vector<uint8_t>(reinterpret_cast<const uint8_t*>(s),
                                reinterpret_cast<const uint8_t*>(s) + len);
}

static void PrintHash(const char* input, uint64_t hash) {
    printf("microhash(\"%s\") = 0x%016llX\n", input,
           (unsigned long long)hash);
}

// ---------------------------------------------------------------------------
// Test vectors
// ---------------------------------------------------------------------------

static const char* testInputs[] = {
    "Hello, World!",
    "The quick brown fox jumps over the lazy dog",
    "",
    "a",
    "abc",
    "        ",
    "abcdefghijklmnopqrstuvwxyz",
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
    "0000000000000000000000000000000000000000000000000000000000000000",
    "1111111111111111111111111111111111111111111111111111111111111111",
    "123456789012345678901234567890",
    "0101010101010101010101010101010101010101010101010101010101010101",
    "0101011101010111010101010101011101010111000101010001110101010100",
};

static const size_t testInputCount = sizeof(testInputs) / sizeof(testInputs[0]);

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    if (argc == 2 && strcmp(argv[1], "--test") == 0) {
        for (size_t i = 0; i < testInputCount; ++i) {
            std::vector<uint8_t> data = ToBytes(testInputs[i]);
            uint64_t h = MicroHash::hashPipe::ComputeHash(data);
            printf("microhash(\"%s\")\t= 0x%016llX\n",
                   testInputs[i], (unsigned long long)h);
        }
        return 0;
    }

    // CLI hash mode
    char inputBuf[4096] = {};

    if (argc == 1) {
        printf("Enter a string to hash using microhash:\n> ");
        if (!fgets(inputBuf, sizeof(inputBuf), stdin) || inputBuf[0] == '\0') {
            printf("No input provided. Exiting.\n");
            return 1;
        }
        // Strip trailing newline from fgets
        size_t len = strlen(inputBuf);
        if (len > 0 && inputBuf[len - 1] == '\n') inputBuf[len - 1] = '\0';
    } else if (argc == 2) {
        strncpy(inputBuf, argv[1], sizeof(inputBuf) - 1);
    } else {
        // Join all args with spaces
        for (int i = 1; i < argc; ++i) {
            if (i > 1) strncat(inputBuf, " ", sizeof(inputBuf) - strlen(inputBuf) - 1);
            strncat(inputBuf, argv[i], sizeof(inputBuf) - strlen(inputBuf) - 1);
        }
    }

    std::vector<uint8_t> data = ToBytes(inputBuf);
    uint64_t hash = MicroHash::hashPipe::ComputeHash(data);
    PrintHash(inputBuf, hash);

    return 0;
}