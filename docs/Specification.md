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

**Target platforms:** Any platform with a C++ compiler that supports fixed-width integer types and basic bitwise arithmetic — modern desktop/server (x86-64, ARM64), embedded microcontrollers (ARM Cortex-M, AVR, RISC-V), and, via porting (see §Porting the C++ Implementation), classic and retro architectures such as the Z80, M68K, and 6502.

**Key characteristics:**

- Header-only single-file library; no compilation step required beyond including the header.
- The core `ComputeHash(const uint8_t* data, size_t length)` overload uses only standard fixed-width integer types and basic bitwise operations — no heap allocation, no OS calls, no standard library beyond the integer-type headers.
- The 32-byte block buffer is stack-allocated, making the function safe to call from interrupt handlers or bare-metal environments.
- Words are assembled manually from individual bytes using explicit bit-shifts (`block[base] | block[base+1]<<8 | …`), guaranteeing identical behaviour on both little-endian and big-endian hosts.
- A convenience overload accepts `std::vector<uint8_t>` for use in hosted environments; the core pointer/length overload can be used without the STL entirely.
- `main.cpp` provides a CLI tool that can hash strings supplied as command-line arguments, via stdin, or run a fixed set of test vectors with `--test`.

### C# (`src/csharp/microhash64.cs`)

**Target platforms:** Any C# runtime — modern .NET, legacy .NET Framework, and Mono. This covers Windows, Linux, macOS, iOS, and Android. The core hash logic uses only fundamental C# language features (integer arithmetic, arrays, and bitwise operators) that have been stable since the earliest versions of the language and runtime.

**Key characteristics:**

- Implemented as a static class (`hashPipe`) in a standard console project.
- Word construction uses `BitConverter.ToUInt32`, which is correct on the little-endian architectures targeted by every common .NET runtime; on a big-endian host the result would differ from the C++ implementation.
- Input is taken as a `byte[]`; callers convert strings via `Encoding.UTF8.GetBytes`, making the encoding explicit.
- The hash logic itself (`hashPipe.cs`) has no dependencies beyond `System` and can be dropped into any C# project, including ones targeting .NET Framework 2.0 or later, without modification.
- The benchmarking harness (`Benchmarks.cs`) depends on the BenchmarkDotNet package and a modern SDK build system. If cross-platform build tooling is unavailable (for example, when targeting .NET Framework on Windows without the .NET SDK), the benchmarks can be removed and the core logic compiled independently.
- `Program.cs` exposes a CLI interface matching the C++ tool and additionally runs the hash three times in a feedback loop before displaying the final result for the default interactive mode.

---

## Porting the C++ Implementation

Because the core algorithm uses only 32-bit integer arithmetic, bitwise XOR, addition, and bit-rotation, it can be translated to virtually any language or assembly dialect. The sections below describe what is required to port MicroHash64 to architectures that pre-date modern C++ compilers.

### General requirements for any port

1. **Two 32-bit unsigned accumulators** (`state[0]` and `state[1]`). On 8-bit or 16-bit CPUs these must be synthesised from multiple narrower registers or memory locations.
2. **A 32-byte scratch buffer** for the current block. On very constrained systems (e.g. 6502 with 256 bytes of zero-page) this buffer can be placed in RAM outside zero-page; only 16 bytes are read per block so it is also possible to process four bytes at a time without a full block buffer.
3. **32-bit rotate-left** by 5, 11, and 3 bits. On CPUs without a barrel shifter this is implemented as a sequence of single-bit shifts and rotates.
4. **32-bit addition** — straightforward with carry-propagation on any architecture.
5. **Padding logic** — a simple byte counter that appends `0x80`, then zeros, then a big-endian 32-bit length in the last four bytes of the final block.

### Z80 / CP/M

The Z80 is an 8-bit processor. All 32-bit quantities must be maintained in pairs of 16-bit register pairs (or in memory). The standard approach:

- Represent each state word as a four-byte region in memory (e.g. in the BSS segment of a CP/M `.COM` file).
- The rotate-left by 5 is equivalent to rotate-left by 8 (a byte swap, which is free) followed by rotate-left by 5 minus 8 = rotate-right by 3, both achievable with `RLA`/`RRA` instructions and carry manipulation.
- Alternatively: rotate-left by 5 = shift-left-by-4 (four `ADD HL,HL` / `ADC` sequences) then rotate-left-by-1; rotate-left by 11 = rotate-right by 21.
- 32-bit addition is done in two 16-bit `ADD HL,DE` + `ADC HL,BC` steps.
- Under CP/M, the input string can be read from the default file control block (FCB) or the command tail at `0x0080`, and the hash can be printed with BDOS call 2 or 9.
- A complete `.COM` implementation requires approximately 100–200 Z80 instructions and fits comfortably in the 64 KB TPA.

### Motorola 68000 (M68K)

The M68K is a 32-bit processor with eight data registers (`D0`–`D7`), making it a natural fit:

- Each state word occupies a single data register; the rotate-left instructions (`ROL.L #5,Dn` and `ROL.L #11,Dn`) are single native instructions.
- 32-bit XOR and ADD are single instructions (`EOR.L`, `ADD.L`).
- The block buffer can live on the stack (`SUBQ.L #32,SP`).
- On Amiga, Atari ST, or a bare 68k SBC the function can be a position-independent relocatable module with no OS dependencies.
- A straightforward assembly port is around 30–50 instructions for the inner loop.

### MOS 6502

The 6502 is an 8-bit processor with no native 32-bit arithmetic:

- State words are stored as four consecutive bytes in zero-page (8 addresses each) for fast access.
- 32-bit XOR is four `EOR zpg` instructions.
- 32-bit addition uses `CLC` + four `ADC` instructions with carry.
- Rotate-left by 5: easiest as rotate-right by 3 (three `ROR` instructions cycling through the four bytes and carry). Alternatively shift-left-by-5 using a loop.
- Rotate-left by 11: rotate-right by 21, or equivalently rotate-left by 8 (a byte rotation — three `LDA/STA` moves) then rotate-left by 3.
- On Apple II or Commodore 64 the 256-byte zero-page makes it practical to keep both state words and the 32-byte block buffer in zero-page; on a bare 6502 any RAM location works.
- Performance will be slow (each 32-bit rotate is 20–40 cycles), but correctness is achievable.

### Other languages / platforms

The algorithm translates directly to any language that provides unsigned 32-bit integers and bitwise operators — C, Pascal, BASIC (with `UINT32` or library support), Forth, assembly, Lua, Python, Rust, Go, and so on. The minimum requirements are:

- An unsigned (or wrapping) 32-bit add.
- Bitwise XOR.
- Left and right bit-shift by a constant amount.
- A byte array or equivalent for the 32-byte block buffer.

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
