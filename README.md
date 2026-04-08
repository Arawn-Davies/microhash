# microhash

A lightweight, non-cryptographic 64-bit hash function designed to run on anything — from x86-64 servers down to 8-bit microcontrollers like the Z80 and 6502.

Prioritises simplicity, minimal memory footprint, and portability over cryptographic strength. Suitable for hash tables, checksums, and data fingerprinting where a fast, deterministic digest is needed.

---

## Files

```
microhash.hpp   — header-only C++ implementation (include this)
main.cpp          — CLI tool
```

---

## Building

**GCC / Linux / WSL**
```bash
g++ -std=c++11 -O2 -o microhash main.cpp
```

**Clang / macOS**
```bash
clang++ -std=c++11 -O2 -o microhash main.cpp
```

**MSVC / Windows (Developer Command Prompt)**
```bat
cl /std:c++11 /O2 /EHsc main.cpp /Fe:microhash.exe
```

No dependencies. No build system required. Both files must be in the same directory.

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
#include "microhash64.hpp"

// From a raw pointer + length (no STL required)
const uint8_t* data = ...;
uint64_t hash = MicroHash::Microhash64::ComputeHash(data, length);

// From a std::vector
std::vector<uint8_t> bytes = ...;
uint64_t hash = MicroHash::Microhash64::ComputeHash(bytes);
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

**Mixing** — each 32-byte block is consumed as four 4-byte little-endian words. Per word:
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
| Min RAM (core) | ~40 bytes (state + one word accumulator) |
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
MicroHash("Hello, World!")                        = 0x352256EFEDC72BD1
MicroHash("The quick brown fox jumps over the lazy dog") = 0x37876396F9CCB637
MicroHash("")                                     = 0xFD1FADBB7E12CB96
MicroHash("a")                                    = 0x9B1F9089AF49253E
MicroHash("abc")                                  = 0x8874CA7BE18B8218
MicroHash("        ")                             = 0xB94BF2A5D5341A60
MicroHash("abcdefghijklmnopqrstuvwxyz")           = 0x67773BF7A225BE5D
MicroHash("ABCDEFGHIJKLMNOPQRSTUVWXYZ...0-9")     = 0xCE821AC98900EEA0
MicroHash("0000...0000")                          = 0x3411C1C38205A8E0
MicroHash("1111...1111")                          = 0x067FE50AF384C88E
MicroHash("123456789012345678901234567890")        = 0xBFD7E4924ACFA323
MicroHash("0101...0101")                          = 0x7BCC8A21375360E0
MicroHash("0101011101010111...")                   = 0xFDFB4707123CF187
```
