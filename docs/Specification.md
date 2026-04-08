# MicroHash64 Specification

## Overview

MicroHash64 is a lightweight, non-cryptographic 64-bit hash function designed to run on a wide range of platforms — from modern desktops, servers, and mobile devices down to resource-constrained embedded systems and historically constrained 8/16-bit architectures. It prioritises simplicity, minimal memory footprint, and portability over cryptographic strength.

The algorithm is intentionally straightforward so that it can be implemented in any language and compiled for any target without requiring hardware acceleration, operating-system support, or dynamic memory allocation.

---

## Algorithm

### 1. Constants

Two 32-bit state words are initialised from the fractional part of π:

```
state[0] = 0x243F6A88
state[1] = 0x85A308D3
```

Using well-known irrational-number constants ("nothing-up-my-sleeve" numbers) reduces the risk of accidentally choosing pathological initial values.

### 2. Padding

Before processing, the input is logically extended to a multiple of 32 bytes using the following scheme:

| Position relative to original input | Value |
|--------------------------------------|-------|
| Immediately after last data byte | `0x80` |
| Intermediate bytes | `0x00` |
| Last 4 bytes of padded message | Big-endian 32-bit encoding of the original byte length |

The padded length is:

```
paddedLength = ceil((inputLength + 5) / 32) * 32
```

The `+5` overhead reserves one byte for the `0x80` marker and four bytes for the length field, guaranteeing at least one padding block even for empty input.

> **Note:** Although the length is encoded into the padding, only the first 16 bytes of every 32-byte block enter the mixing step (see §3). The length field therefore resides in the unreachable second half of the final block and does not currently influence the output. This is a known limitation of the present design.

### 3. Block Processing

Each 32-byte block is consumed in four 4-byte little-endian words (`word[0]`…`word[3]`, covering bytes 0–15 of the block). For each word the two state registers are updated:

```
state[0] = RotateLeft32(state[0] XOR word, 5)  + state[1]
state[1] = RotateLeft32(state[1]  + word, 11) XOR state[0]
```

Where `RotateLeft32(v, n) = (v << n) | (v >> (32 - n))`.

The alternating XOR/ADD operations with two different rotation amounts spread bit influence across both state words and provide avalanche behaviour without requiring large lookup tables or multiple rounds.

### 4. Finalisation

After all blocks have been processed, the 64-bit digest is constructed as follows:

```
final  = state[0] XOR RotateLeft32(state[1], 3)
digest = (final << 32) | state[1]
```

The upper 32 bits of the digest are a mix of both state words; the lower 32 bits are `state[1]` unchanged. The result is returned as a single unsigned 64-bit integer.

---

## Properties

| Property | Value |
|----------|-------|
| Output size | 64 bits |
| Internal state | 2 × 32-bit words (64 bits) |
| Block size | 32 bytes (16 bytes actively mixed) |
| Minimum memory (core) | 32-byte block buffer + 8 bytes state |
| Cryptographic | No |
| Streaming | No (whole-message) |
| Endianness dependency | Little-endian word assembly (both implementations) |

---

## Implementations

### C++ (`src/cpp/microhash64.hpp`)

**Target platforms:** Any platform with a C++11-compliant compiler — embedded microcontrollers (ARM Cortex-M, AVR, RISC-V), desktop/server (x86-64, ARM64), and retro/hobbyist systems reachable via cross-compilation.

**Key characteristics:**

- Header-only single-file library; no compilation step required beyond including the header.
- Uses only `<cstdint>` and `<cstddef>` in the core function — no heap allocation, no OS calls, no standard library beyond fixed-width integer types.
- The 32-byte block buffer is stack-allocated, making the function safe to call from interrupt handlers or bare-metal environments.
- Words are assembled manually from individual bytes using explicit bit-shifts (`block[base] | block[base+1]<<8 | …`), guaranteeing identical behaviour on both little-endian and big-endian hosts.
- A convenience overload accepts `std::vector<uint8_t>` for use in hosted environments; the core overload takes a raw pointer and length and can be used without the STL.
- `main.cpp` provides a CLI tool that can hash strings supplied as command-line arguments, via stdin, or run a fixed set of test vectors with `--test`.

### C# (`src/csharp/microhash64.cs`)

**Target platforms:** Any runtime supporting .NET (including .NET 6+, .NET Framework, Mono): Windows, Linux, macOS desktops/servers, iOS/Android via Xamarin or .NET MAUI.

**Key characteristics:**

- Implemented as a static class (`hashPipe`) in a standard .NET console project.
- Word construction uses `BitConverter.ToUInt32`, which is correct on the little-endian architectures targeted by .NET; on a big-endian host the result would differ from the C++ implementation.
- Input is taken as a `byte[]`; callers convert strings via `Encoding.UTF8.GetBytes`, making the encoding explicit.
- `Benchmarks.cs` (BenchmarkDotNet) provides reproducible performance measurements for small (4 B), medium (1 KB), and large (1 MB) inputs, and a collision-rate test over one million random 64-character strings.
- `Program.cs` exposes a CLI interface matching the C++ tool and additionally runs the hash three times in a feedback loop before displaying the final result for the default interactive mode.

---

## Cross-Implementation Compatibility

Both implementations produce identical output for the same byte sequence **on little-endian hosts**. The only observable difference is the word-loading path:

| Aspect | C++ | C# |
|--------|-----|----|
| Word loading | Manual byte shifts | `BitConverter.ToUInt32` |
| Length field type | `size_t` (≥32-bit) | `int` (32-bit signed) |
| Input interface | `const uint8_t*` + length, or `std::vector<uint8_t>` | `byte[]` |
| Endianness safe | Yes (explicit shifts) | Only on little-endian hosts |

---

## Design Goals and Trade-offs

| Goal | Approach |
|------|----------|
| Minimal footprint | Fixed 32-byte stack buffer; 8 bytes of state |
| Portability | Standard integer types; no SIMD, no hardware instructions |
| Simplicity | ~30 lines of logic; no lookup tables |
| Speed | Single-pass; rotate-XOR-ADD mixing without multi-round expansion |
| **Not** a goal | Cryptographic security — no resistance to preimage, collision, or length-extension attacks is claimed |

MicroHash64 is suitable for hash tables, checksums, data fingerprinting, and similar non-security-sensitive applications where a fast, deterministic, platform-independent digest is required.
