---
layout: default
title: microhash
nav_order: 1
permalink: /
---

# microhash

**microhash** is a lightweight, non-cryptographic 64-bit hash function for
systems ranging from x86-64 servers to resource-constrained embedded targets.
It is designed for hash tables, checksums, and data fingerprinting where a
small, deterministic implementation matters more than cryptographic strength.

[View the specification](Specification.html) |
[Build and test microhash](Building-and-Testing.html) |
[Browse the source on GitHub](https://github.com/Arawn-Davies/microhash)

> microhash is **not** a cryptographic hash. Do not use it for passwords,
> signatures, authentication, or other security-sensitive purposes.

## At a glance

| Property | Value |
|---|---|
| Output | 64-bit digest |
| Internal state | Two 32-bit words |
| Processing block | 32 bytes, with the first 16 bytes actively mixed |
| Core working memory | 32-byte block buffer and 8 bytes of state |
| C++ API | Header-only, with no heap allocation in the core overload |
| Implementations | C++17, C#, and Ruby |
| Cryptographic | No |

The C++ implementation is intentionally small. Its core overload uses fixed
width integer arithmetic, bitwise operations, and a stack-allocated buffer:

```cpp
#include "microhash.hpp"

const uint8_t* data = /* ... */;
size_t length = /* ... */;

uint64_t digest = MicroHash::hashPipe::ComputeHash(data, length);
```

## Quick start

Build the C++ command-line tool from the repository root:

```sh
g++ -std=c++17 -O2 -o microhash src/cpp/main.cpp
./microhash "Hello, World!"
```

Expected output:

```text
microhash("Hello, World!") = 0x352256EFEDC72BD1
```

The CLI also accepts multiple words and interactive input:

```sh
./microhash The quick brown fox
./microhash
./microhash --test
```

For the C# implementation:

```sh
dotnet run --project src/csharp/microhash.csproj -- "Hello, World!"
```

For the Ruby implementation (no build step, no gem dependencies):

```sh
ruby src/ruby/main.rb "Hello, World!"
```

See [Building and Testing](Building-and-Testing.html) for debug and release
builds, the C++, C#, and Ruby test suites, benchmarks, and Docker usage.

## Algorithm summary

microhash starts from two fixed 32-bit constants:

```text
state[0] = 0x243F6A88
state[1] = 0x85A308D3
```

The input is padded to a multiple of 32 bytes. The first 16 bytes of each block
are read as four little-endian words. Each word updates both state values with
rotate, XOR, and addition operations:

```text
state[0] = ROL32(state[0] XOR word,  5) + state[1]
state[1] = ROL32(state[1]  + word, 11) XOR state[0]
```

The final 64-bit digest combines both accumulators:

```text
final  = state[0] XOR ROL32(state[1], 3)
digest = (final << 32) | state[1]
```

The complete [specification](Specification.html) documents padding,
finalisation, output truncation, implementation differences, and porting notes
for constrained platforms.

## Embedded targets

The C++ core has no operating-system dependency and performs no heap
allocation. It can be adapted to environments with less than 32 bytes of
working RAM by processing one word at a time instead of retaining a complete
block buffer.

The specification includes implementation guidance for:

- Z80 and CP/M systems
- Motorola 68000 targets
- MOS 6502 systems
- Other languages with wrapping 32-bit arithmetic and bitwise operators

## Known limitations

- The algorithm is not designed to resist collision, preimage, or
  length-extension attacks.
- Only bytes `0` through `15` of each 32-byte block are mixed. Bytes `16`
  through `31`, including the encoded length field in the final block, do not
  currently influence the output. **This makes the original algorithm
  unsuitable for change detection or fingerprinting** — half of every input
  is invisible to the digest. The revised
  [microhash-ng](https://github.com/Arawn-Davies/microhash/tree/master/src/microhash-ng)
  mixes all 32 bytes per block with hardened double-round absorption and
  finalization rounds (fixing the dead zone, the unread length field, and
  the statistical weaknesses found by the SMHasher battery, which it now
  passes with zero failures) while keeping the same API, memory footprint,
  and 64-bit output; its digests are not compatible with the original.
- The C++ and Ruby implementations assemble words explicitly and are
  host-endian safe. The C# implementation uses `BitConverter.ToUInt32`, so
  matching output is expected on little-endian hosts.

## Documentation

- [Specification](Specification.html): algorithm, implementations, porting
  notes, constrained-target adaptations, and design trade-offs.
- [Building and Testing](Building-and-Testing.html): C++, C#, Ruby, Docker,
  benchmarks, test coverage, and statistical checks.
- [README](https://github.com/Arawn-Davies/microhash#readme): repository
  overview and test vectors.

## License

microhash is distributed under the
[MIT License](https://github.com/Arawn-Davies/microhash/blob/master/LICENSE.txt).
