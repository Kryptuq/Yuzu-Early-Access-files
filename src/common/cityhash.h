// SPDX-FileCopyrightText: 2011 Google, Inc.
// SPDX-FileContributor: Geoff Pike
// SPDX-FileContributor: Jyrki Alakuijala
// SPDX-License-Identifier: MIT

// CityHash, by Geoff Pike and Jyrki Alakuijala
//
// http://code.google.com/p/cityhash/
//
// This file provides a few functions for hashing strings.  All of them are
// high-quality functions in the sense that they pass standard tests such
// as Austin Appleby's SMHasher.  They are also fast.
//
// For 64-bit x86 code, on short strings, we don't know of anything faster than
// CityHash64 that is of comparable quality.  We believe our nearest competitor
// is Murmur3.  For 64-bit x86 code, CityHash64 is an excellent choice for hash
// tables and most other hashing (excluding cryptography).
//
// For 64-bit x86 code, on long strings, the picture is more complicated.
// On many recent Intel CPUs, such as Nehalem, Westmere, Sandy Bridge, etc.,
// CityHashCrc128 appears to be faster than all competitors of comparable
// quality.  CityHash128 is also good but not quite as fast.  We believe our
// nearest competitor is Bob Jenkins' Spooky.  We don't have great data for
// other 64-bit CPUs, but for long strings we know that Spooky is slightly
// faster than CityHash on some relatively recent AMD x86-64 CPUs, for example.
// Note that CityHashCrc128 is declared in citycrc.h.
//
// For 32-bit x86 code, we don't know of anything faster than CityHash32 that
// is of comparable quality.  We believe our nearest competitor is Murmur3A.
// (On 64-bit CPUs, it is typically faster to use the other CityHash variants.)
//
// Functions in the CityHash family are not suitable for cryptography.
//
// Please see CityHash's README file for more details on our performance
// measurements and so on.
//
// WARNING: This code has been only lightly tested on big-endian platforms!
// It is known to work well on little-endian platforms that have a small penalty
// for unaligned reads, such as current Intel and AMD moderate-to-high-end CPUs.
// It should work on all 32-bit and 64-bit platforms that allow unaligned reads;
// bug reports are welcome.
//
// By the way, for some hash functions, given strings a and b, the hash
// of a+b is easily derived from the hashes of a and b.  This property
// doesn't hold for any hash functions in this file.

#pragma once

#include <cstddef>
#include "common/common_types.h"

namespace Common {

// Hash function for a byte array.
[[nodiscard]] u64 CityHash64(const char* buf, size_t len);

// Hash function for a byte array.  For convenience, a 64-bit seed is also
// hashed into the result.
[[nodiscard]] u64 CityHash64WithSeed(const char* buf, size_t len, u64 seed);

// Hash function for a byte array.  For convenience, two seeds are also
// hashed into the result.
[[nodiscard]] u64 CityHash64WithSeeds(const char* buf, size_t len, u64 seed0, u64 seed1);

// Hash function for a byte array.
[[nodiscard]] u128 CityHash128(const char* s, size_t len);

// Hash function for a byte array.  For convenience, a 128-bit seed is also
// hashed into the result.
[[nodiscard]] u128 CityHash128WithSeed(const char* s, size_t len, u128 seed);

// Hash 128 input bits down to 64 bits of output.
// This is intended to be a reasonably good hash function.
[[nodiscard]] inline u64 Hash128to64(const u128& x) {
    // Murmur-inspired hashing.
    const u64 mul = 0x9ddfea08eb382d69ULL;
    u64 a = (x[0] ^ x[1]) * mul;
    a ^= (a >> 47);
    u64 b = (x[1] ^ a) * mul;
    b ^= (b >> 47);
    b *= mul;
    return b;
}

} // namespace Common
