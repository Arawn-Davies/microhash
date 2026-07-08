# microhash

A lightweight, non-cryptographic 64-bit hash function designed to run on anything — from x86-64 servers down to 8-bit microcontrollers like the Z80 and 6502.

Prioritises simplicity, minimal memory footprint, and portability over cryptographic strength. Suitable for hash tables, checksums, and data fingerprinting where a fast, deterministic digest is needed.

> **Which version should I use?** The original algorithm (below) only mixes the first 16 bytes of each 32-byte block — bytes 16–31 and the encoded length are invisible to the digest, so it **must not** be used for change detection or fingerprinting. **[microhash-ng](#microhash-ng)** fixes both defects with the same API, memory footprint, and 64-bit output. New integrations should use microhash-ng; the original is retained for compatibility with existing digests.

---

## Files

```
src/cpp/microhash.hpp   — header-only C++ implementation (include this)
src/cpp/main.cpp        — C++ CLI tool
src/csharp/microhash.cs — C# implementation
src/csharp/Program.cs   — C# CLI tool
src/ruby/microhash.rb   — Ruby implementation (pure Ruby, no dependencies)
src/ruby/main.rb        — Ruby CLI tool
src/ruby/ext/microhash/ — optional native C extension (auto-detected)
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

## Using from Ruby

`src/ruby/microhash.rb` is a single-file, pure-Ruby port with no gem dependencies (Ruby ≥ 2.5). Drop it into a Rails app (e.g. `lib/microhash.rb`) or require it directly:

```ruby
require_relative 'src/ruby/microhash'

MicroHash.compute_hash('Hello, World!')   # => 0x352256EFEDC72BD1 (Integer)
MicroHash.hexdigest('Hello, World!')      # => "352256EFEDC72BD1"

# Strings are hashed as raw bytes (encoding-agnostic); byte arrays also work
MicroHash.compute_hash([0x61, 0x62, 0x63]) == MicroHash.compute_hash('abc')  # => true
```

CLI (mirrors the C++/C# tools):

```bash
ruby src/ruby/main.rb "Hello, World!"
ruby src/ruby/main.rb --test
```

Output is identical to the C++ and C# implementations for the same byte sequence.

### Optional native extension

For OpenSSL-digest-style speed, compile the bundled C extension once:

```bash
cd src/ruby/ext/microhash
ruby extconf.rb && make
```

`microhash.rb` auto-detects the compiled extension (`MicroHash::NATIVE` is `true` when loaded) and transparently delegates hashing to C — the API and output are unchanged, and the pure-Ruby path remains as a fallback wherever no compiler is available. On Ruby 3.1 the native path is roughly 18–33× faster than the (already optimized) pure-Ruby path and ~5–8× faster than `Digest::SHA256`.

---

## Benchmarks

Indicative single-thread numbers on one x86-64 machine (WSL2), hashing the same three inputs in a tight loop after warm-up. C++ built with `g++ -O2`; C# as a .NET Release build; Ruby figures measured on CRuby 3.1.3. `Digest::SHA256` (OpenSSL's C implementation, via Ruby) is included as a reference point. Absolute numbers will vary by machine — the ratios are the point.

| Implementation | 13 B / op | 1 KB / op | 64 KB / op | 64 KB throughput |
|---|---|---|---|---|
| C++ original (`g++ -O2`) | 18 ns | 395 ns | 22.5 µs | ~2,910 MB/s |
| C++ microhash-ng | 30 ns | 584 ns | 38.4 µs | ~1,710 MB/s |
| Ruby native ext — original | 65 ns | 699 ns | 35.7 µs | ~1,840 MB/s |
| Ruby native ext — microhash-ng | 76 ns | 743 ns | 37.8 µs | ~1,730 MB/s |
| C# original (.NET, Release) | ~280 ns | ~5.2 µs | ~50 µs | ~1,310 MB/s |
| `Digest::SHA256` (OpenSSL) | 513 ns | 3.5 µs | 186 µs | ~350 MB/s |
| Ruby pure — original | 1.2 µs | 18.9 µs | 1.19 ms | ~55 MB/s |
| Ruby pure — microhash-ng | 3.0 µs | 61.5 µs | 3.42 ms | ~19 MB/s |

The pure-Ruby paths pad the message once and use `String#unpack('V*')` for word assembly (which runs in C) with the rotates inlined in the mixing loop — about 4.5× faster than the original byte-by-byte implementation. microhash is not cryptographic (see limitations below), so this is not an apples-to-apples comparison with SHA-256 — it is only meant to show where each implementation sits when a fast non-cryptographic digest is sufficient.

### Case study: fingerprinting a Rails view tree

End-to-end feasibility test for file-change-detection tooling (e.g. a WCAG audit gem that fingerprints every view): 594 ERB files, 2.3 MB total, read + hashed per file, CRuby 3.1.3, warm page cache. Wall time for the full tree:

| Pipeline | Full tree | Hash cost over IO floor |
|---|---|---|
| Read only (IO floor) | 4.1 ms | — |
| Read + microhash-ng **native ext** | 5.2 ms | ~1.1 ms |
| Read + `SHA256[0,16]` (truncated) | 12.8 ms | ~8.7 ms |
| Read + `Digest::SHA256` | 14.2 ms | ~10.1 ms |
| Read + microhash-ng **pure Ruby** | 136.5 ms | ~132 ms |

Read honestly: with the native extension, microhash-ng is the fastest option (~2.7× faster end-to-end than SHA-256) and produces 16-char digests; as a pure-Ruby drop-in it is ~10× slower than OpenSSL-backed SHA-256, though 136 ms per full tree is still imperceptible in CI. If a dependency-free install matters more than digest length semantics, truncated SHA-256 remains a perfectly good choice — every option here is far from being a bottleneck at this scale.

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
- **Dead zone: bytes 16–31 of every block are ignored.** Only the first 16 bytes of each 32-byte block are mixed. Half of every input is invisible to the digest — single-byte edits in a dead zone leave the digest unchanged, and trivial collisions exist. **Disqualifying for change detection / fingerprinting** — use [microhash-ng](#microhash-ng) instead.
- **Length field unreachable.** The big-endian length encoded in the final padding block falls in the second 16 bytes of the block, which the mixing step does not read. The length field does not currently influence the output; appends/truncations confined to dead zones go undetected. Also fixed in microhash-ng.
- **Little-endian word loading.** Output is consistent across all platforms because words are assembled with explicit byte shifts, but the design is inherently little-endian oriented.

---

## microhash-ng

`src/microhash-ng/` contains a revised algorithm that fixes the correctness defects above while keeping the API, 32-byte blocks, ~40-byte working memory, **64-bit output (16 hex chars, unchanged)**, and the ARX (add-rotate-XOR) design — still no multiplies or lookup tables, so the 8-bit portability story is intact. Three changes:

1. All **eight** 4-byte words of each block are mixed (no dead zone; the encoded length field is live).
2. Each word is absorbed with **two ARX rounds** (the second reinjects the word rotated by 16), so a difference passes through two nonlinear rounds before the next word could cancel it.
3. Four **finalization rounds** with π-derived constants diffuse the final words through the whole state.

**microhash-ng passes the SMHasher quality battery** (rurban/smhasher: Avalanche, Sparse, Cyclic, TwoBytes, Zeroes, Seed, DiffDist, BIC — zero failures), the same battery xxHash-class hashes are judged by. For comparison, mixing all eight words with single rounds and no finalization still failed 15 sub-tests (sparse-key collisions from absorption-time cancellation), and the original algorithm failed 21 including avalanche biases up to 96%. SMHasher-measured bulk throughput for the hardened C implementation is ~1.85 GB/s (vs ~3 GB/s unhardened) — the cost of passing.

**Digests are not compatible with original microhash** — adopting ng means re-fingerprinting existing data.

```
src/microhash-ng/cpp/microhash_ng.hpp       — header-only C++ implementation
src/microhash-ng/cpp/main.cpp               — C++ CLI tool
src/microhash-ng/csharp/                    — C# implementation + CLI (standalone project)
src/microhash-ng/ruby/microhash_ng.rb       — Ruby implementation (pure, no dependencies)
src/microhash-ng/ruby/ext/microhash_ng/     — optional native C extension (auto-detected)
tests/microhash-ng/                         — C++ and RSpec suites with dead-zone regression tests
```

Verified properties (covered by regression tests in all suites): every single-byte flip at **every** position of 32/64/256-byte inputs changes the digest (original missed 50%); the known 64-byte dead-zone collision pair now differs; appends and truncations are detected via the mixed length field.

### microhash-ng test vectors

```
microhash-ng("Hello, World!")                                                    = 0xA40E5C7D0BFBA07D
microhash-ng("The quick brown fox jumps over the lazy dog")                      = 0x5BD8C52E8C1E2175
microhash-ng("")                                                                 = 0x6CA97D4E1A59E8EC
microhash-ng("a")                                                                = 0xD1EF310FB09DC1DC
microhash-ng("abc")                                                              = 0x1351FEBF7FEDB189
microhash-ng("        ")                                                         = 0x38AFC965BDFDC9EB
microhash-ng("abcdefghijklmnopqrstuvwxyz")                                       = 0xF2A991C82844982F
microhash-ng("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789")   = 0x7AB7F4398A2A0130
microhash-ng("0000000000000000000000000000000000000000000000000000000000000000") = 0xBC55379EAAB952BF
microhash-ng("1111111111111111111111111111111111111111111111111111111111111111") = 0x42AF241530C58F18
microhash-ng("123456789012345678901234567890")                                   = 0x7A78EB4902E77E91
microhash-ng("0101010101010101010101010101010101010101010101010101010101010101") = 0x38232B18B8755FA6
microhash-ng("0101011101010111010101010101011101010111000101010001110101010100") = 0x29E828BAC44A055B
```

Build and run exactly like the originals, e.g. `g++ -std=c++17 -O2 -o microhash-ng src/microhash-ng/cpp/main.cpp`, `ruby src/microhash-ng/ruby/main.rb --test`, or `dotnet run --project src/microhash-ng/csharp -- --test`.

---

## Test Vectors

Run `./microhash --test` to verify output. Expected values:

```
microhash("Hello, World!")                                                     = 0x352256EFEDC72BD1
microhash("The quick brown fox jumps over the lazy dog")                       = 0x37876396F9CCB637
microhash("")                                                                  = 0xFD1FADBB7E12CB96
microhash("a")                                                                 = 0x9B1F9089AF49253E
microhash("abc")                                                               = 0x8874CA7BE18B8218
microhash("        ")                                                          = 0xB94BF2A5D5341A60
microhash("abcdefghijklmnopqrstuvwxyz")                                        = 0x67773BF7A225BE5D
microhash("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789")   = 0xCE821AC98900EEA0
microhash("0000000000000000000000000000000000000000000000000000000000000000")  = 0x3411C1C38205A8E0
microhash("1111111111111111111111111111111111111111111111111111111111111111")  = 0x067FE50AF384C88E
microhash("123456789012345678901234567890")                                    = 0xBFD7E4924ACFA323
microhash("0101010101010101010101010101010101010101010101010101010101010101")  = 0x7BCC8A21375360E0
microhash("0101011101010111010101010101011101010111000101010001110101010100")  = 0xFDFB4707123CF187
```
