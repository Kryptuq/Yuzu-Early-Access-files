// SPDX-FileCopyrightText: 2005-2014 Daniel James
// SPDX-FileCopyrightText: 2016 Austin Appleby
// SPDX-License-Identifier: BSL-1.0

#include <array>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <vector>

namespace Common {

namespace detail {

template <typename T>
    requires std::is_unsigned_v<T>
inline std::size_t HashValue(T val) {
    const unsigned int size_t_bits = std::numeric_limits<std::size_t>::digits;
    const unsigned int length =
        (std::numeric_limits<T>::digits - 1) / static_cast<unsigned int>(size_t_bits);

    std::size_t seed = 0;

    for (unsigned int i = length * size_t_bits; i > 0; i -= size_t_bits) {
        seed ^= static_cast<size_t>(val >> i) + (seed << 6) + (seed >> 2);
    }

    seed ^= static_cast<size_t>(val) + (seed << 6) + (seed >> 2);

    return seed;
}

template <size_t Bits>
struct HashCombineImpl {
    template <typename T>
    static inline T fn(T seed, T value) {
        seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
};

template <>
struct HashCombineImpl<64> {
    static inline std::uint64_t fn(std::uint64_t h, std::uint64_t k) {
        const std::uint64_t m = (std::uint64_t(0xc6a4a793) << 32) + 0x5bd1e995;
        const int r = 47;

        k *= m;
        k ^= k >> r;
        k *= m;

        h ^= k;
        h *= m;

        // Completely arbitrary number, to prevent 0's
        // from hashing to 0.
        h += 0xe6546b64;

        return h;
    }
};

} // namespace detail

template <typename T>
inline void HashCombine(std::size_t& seed, const T& v) {
    seed = detail::HashCombineImpl<sizeof(std::size_t) * CHAR_BIT>::fn(seed, detail::HashValue(v));
}

template <typename It>
inline std::size_t HashRange(It first, It last) {
    std::size_t seed = 0;

    for (; first != last; ++first) {
        HashCombine<typename std::iterator_traits<It>::value_type>(seed, *first);
    }

    return seed;
}

template <typename T, size_t Size>
std::size_t HashValue(const std::array<T, Size>& v) {
    return HashRange(v.cbegin(), v.cend());
}

template <typename T, typename Allocator>
std::size_t HashValue(const std::vector<T, Allocator>& v) {
    return HashRange(v.cbegin(), v.cend());
}

} // namespace Common
