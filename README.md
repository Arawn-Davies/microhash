# microhash

A lightweight, non-cryptographic 64-bit hash function designed to run on anything — from x86-64 servers down to 8-bit microcontrollers like the Z80 and 6502.

Prioritises simplicity, minimal memory footprint, and portability over cryptographic strength. Suitable for hash tables, checksums, and data fingerprinting where a fast, deterministic digest is needed.

---

## Files

```
src/cpp/microhash.hpp   — header-only C++ implementation (include this)
src/cpp/main.cpp        — C++ CLI tool
src/csharp/microhash.cs — C# implementation
src/csharp/Program.cs   — C# CLI tool
```

---

## Building

**GCC / Linux / WSL**
```bash
g++ -std=c++17 -O2 -o microhash src/cpp/main.cpp
```

**Clang / macOS**
```bash
clang++ -std=c++17 -O2 -o microhash src/cpp/main.cpp
```

**MSVC / Windows (Developer Command Prompt)**
```bat
cl /std:c++17 /O2 /EHsc src\cpp\main.cpp /Fe:microhash.exe
```

No dependencies. No build system required. Run from the repository root.

---

## Usage

```bash
# Hash a string directly
./microhash "Hello, World!"

# Hash multiple words (joined with spaces)
./microhash The quick brown fox

# Interactive prompt
./microhash

# Run built-in test vectors
./microhash --test
```

---

## Using the Header

```cpp
#include "microhash.hpp"

// From a raw pointer + length (no STL required)
const uint8_t* data = ...;
uint64_t hash = MicroHash::hashPipe::ComputeHash(data, length);

// From a std::vector
std::vector<uint8_t> bytes = ...;
uint64_t hash = MicroHash::hashPipe::ComputeHash(bytes);
```

The core `ComputeHash(const uint8_t*, size_t)` overload has zero heap allocation, no OS calls, and no STL dependencies beyond `<cstdint>` and `<cstddef>`. Safe to call from interrupt handlers and bare-metal environments.

---

## Algorithm

**State** — two 32-bit words initialised from the fractional part of π:
```
state[0] = 0x243F6A88
state[1] = 0x85A308D3
```

**Padding** — input is extended to a multiple of 32 bytes:
- Byte immediately after data → `0x80`
- Intermediate bytes → `0x00`
- Last 4 bytes → big-endian 32-bit encoding of original length

```
paddedLength = ceil((inputLength + 5) / 32) * 32
```

**Mixing** — the first 16 bytes of each 32-byte block are consumed as four 4-byte little-endian words (`word[0]`…`word[3]`, bytes 0–15). The second 16 bytes of every block are not read. Per word:
```
state[0] = ROL32(state[0] XOR word,  5) + state[1]
state[1] = ROL32(state[1]  + word,  11) XOR state[0]
```

**Finalisation:**
```
final  = state[0] XOR ROL32(state[1], 3)
digest = (final << 32) | state[1]
```

| Property | Value |
|----------|-------|
| Output | 64 bits |
| Internal state | 2 × 32-bit words |
| Block size | 32 bytes |
| Min RAM (core) | ~40 bytes (32-byte block buffer + 8 bytes state) |
| Cryptographic | No |
| Endianness | Little-endian word assembly; identical output on any host |

---

## Truncated Output

The full 64-bit output can be narrowed without any extra computation:

| Width | How | Notes |
|-------|-----|-------|
| 64-bit | `(final << 32) \| state[1]` | Full digest |
| 32-bit | `state[1]` | Most-mixed accumulator |
| 16-bit | Lower 16 bits of `state[1]` | For small hash tables |
| 8-bit | Low byte of `state[1]` | Tiny lookup structures only |

---

## Embedded / Constrained Targets

The header is intentionally written to be portable to any target with a C++ compiler that supports fixed-width integer types. The core overload uses only bitwise arithmetic, addition, and a stack-allocated 32-byte buffer.

**Very low RAM (<32 bytes):** eliminate the block buffer by processing one 4-byte word at a time directly from the input stream. Working set shrinks to ~16 bytes (8 state + 4 accumulator + 4 counter).

**8-bit CPUs (Z80, 6502, etc.):** the 32-bit state words and rotate operations must be synthesised from narrower primitives. Rotate-left by 8 or 16 is a free byte-lane rotate on these architectures and can be used to reduce shift costs. See the specification document for per-architecture notes.

**16/32-bit CPUs (M68K):** `ROL.L #5,Dn` and `ROL.L #11,Dn` are single native instructions. The inner loop is approximately 30–50 instructions.

---

## Known Limitations

- **Not cryptographic.** No resistance to preimage, collision, or length-extension attacks.
- **Length field unreachable.** The big-endian length encoded in the final padding block falls in the second 16 bytes of the block, which the mixing step does not read. The length field does not currently influence the output.
- **Little-endian word loading.** Output is consistent across all platforms because words are assembled with explicit byte shifts, but the design is inherently little-endian oriented.

---

## Test Vectors

Run `./microhash --test` to verify output. Expected values:

```
MicroHash64("Hello, World!")                                                     = 0x352256EFEDC72BD1
MicroHash64("The quick brown fox jumps over the lazy dog")                       = 0x37876396F9CCB637
MicroHash64("")                                                                  = 0xFD1FADBB7E12CB96
MicroHash64("a")                                                                 = 0x9B1F9089AF49253E
MicroHash64("abc")                                                               = 0x8874CA7BE18B8218
MicroHash64("        ")                                                          = 0xB94BF2A5D5341A60
MicroHash64("abcdefghijklmnopqrstuvwxyz")                                        = 0x67773BF7A225BE5D
MicroHash64("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789")   = 0xCE821AC98900EEA0
MicroHash64("0000000000000000000000000000000000000000000000000000000000000000")  = 0x3411C1C38205A8E0
MicroHash64("1111111111111111111111111111111111111111111111111111111111111111")  = 0x067FE50AF384C88E
MicroHash64("123456789012345678901234567890")                                    = 0xBFD7E4924ACFA323
MicroHash64("0101010101010101010101010101010101010101010101010101010101010101")  = 0x7BCC8A21375360E0
MicroHash64("0101011101010111010101010101011101010111000101010001110101010100")  = 0xFDFB4707123CF187
```
