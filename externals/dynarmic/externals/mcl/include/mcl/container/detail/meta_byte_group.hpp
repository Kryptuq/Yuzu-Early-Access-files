// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

#include <array>
#include <bit>

#include "mcl/assert.hpp"
#include "mcl/container/detail/meta_byte.hpp"
#include "mcl/macro/architecture.hpp"
#include "mcl/stdint.hpp"

#if defined(MCL_ARCHITECTURE_ARM64)
#    include <arm_neon.h>
#elif defined(MCL_ARCHITECTURE_X86_64)
#    include <emmintrin.h>

#    include "mcl/bit_cast.hpp"
#else
#    include <cstring>
#endif

namespace mcl::detail {

#if defined(MCL_ARCHITECTURE_ARM64)

struct meta_byte_group {
    static constexpr size_t max_group_size{16};

    explicit meta_byte_group(meta_byte* ptr)
        : data{vld1q_u8(reinterpret_cast<u8*>(ptr))}
    {}

    explicit meta_byte_group(const std::array<meta_byte, 16>& array)
        : data{vld1q_u8(reinterpret_cast<const u8*>(array.data()))}
    {}

    uint64x2_t match(meta_byte cmp) const
    {
        return vreinterpretq_u64_u8(vandq_u8(vceqq_u8(data,
                                                      vdupq_n_u8(static_cast<u8>(cmp))),
                                             vdupq_n_u8(0x80)));
    }

    uint64x2_t match_empty_or_tombstone() const
    {
        return vreinterpretq_u64_u8(vandq_u8(data,
                                             vdupq_n_u8(0x80)));
    }

    bool is_any_empty() const
    {
        static_assert(meta_byte::empty == static_cast<meta_byte>(0xff), "empty must be maximal u8 value");
        return vmaxvq_u8(data) == 0xff;
    }

    bool is_all_empty_or_tombstone() const
    {
        return vminvq_u8(vandq_u8(data, vdupq_n_u8(0x80))) == 0x80;
    }

    meta_byte get(size_t index) const
    {
        return static_cast<meta_byte>(data[index]);
    }

    void set(size_t index, meta_byte value)
    {
        data[index] = static_cast<u8>(value);
    }

    uint8x16_t data;
};

#    define MCL_HMAP_MATCH_META_BYTE_GROUP(MATCH, ...)                                                             \
        {                                                                                                          \
            const uint64x2_t match_result{MATCH};                                                                  \
                                                                                                                   \
            for (u64 match_result_v{match_result[0]}; match_result_v != 0; match_result_v &= match_result_v - 1) { \
                const size_t match_index{static_cast<size_t>(std::countr_zero(match_result_v) / 8)};               \
                __VA_ARGS__                                                                                        \
            }                                                                                                      \
                                                                                                                   \
            for (u64 match_result_v{match_result[1]}; match_result_v != 0; match_result_v &= match_result_v - 1) { \
                const size_t match_index{static_cast<size_t>(8 + std::countr_zero(match_result_v) / 8)};           \
                __VA_ARGS__                                                                                        \
            }                                                                                                      \
        }

#    define MCL_HMAP_MATCH_META_BYTE_GROUP_EXCEPT_LAST(MATCH, ...)                                                                      \
        {                                                                                                                               \
            const uint64x2_t match_result{MATCH};                                                                                       \
                                                                                                                                        \
            for (u64 match_result_v{match_result[0]}; match_result_v != 0; match_result_v &= match_result_v - 1) {                      \
                const size_t match_index{static_cast<size_t>(std::countr_zero(match_result_v) / 8)};                                    \
                __VA_ARGS__                                                                                                             \
            }                                                                                                                           \
                                                                                                                                        \
            for (u64 match_result_v{match_result[1] & 0x00ffffffffffffff}; match_result_v != 0; match_result_v &= match_result_v - 1) { \
                const size_t match_index{static_cast<size_t>(8 + std::countr_zero(match_result_v) / 8)};                                \
                __VA_ARGS__                                                                                                             \
            }                                                                                                                           \
        }

#elif defined(MCL_ARCHITECTURE_X86_64)

struct meta_byte_group {
    static constexpr size_t max_group_size{16};

    explicit meta_byte_group(meta_byte* ptr)
        : data{_mm_load_si128(reinterpret_cast<__m128i const*>(ptr))}
    {}

    explicit meta_byte_group(const std::array<meta_byte, 16>& array)
        : data{_mm_loadu_si128(reinterpret_cast<__m128i const*>(array.data()))}
    {}

    u16 match(meta_byte cmp) const
    {
        return _mm_movemask_epi8(_mm_cmpeq_epi8(data, _mm_set1_epi8(static_cast<u8>(cmp))));
    }

    u16 match_empty_or_tombstone() const
    {
        return _mm_movemask_epi8(data);
    }

    bool is_any_empty() const
    {
        return match(meta_byte::empty);
    }

    bool is_all_empty_or_tombstone() const
    {
        return match_empty_or_tombstone() == 0xffff;
    }

    meta_byte get(size_t index) const
    {
        return mcl::bit_cast<std::array<meta_byte, max_group_size>>(data)[index];
    }

    void set(size_t index, meta_byte value)
    {
        auto array = mcl::bit_cast<std::array<meta_byte, max_group_size>>(data);
        array[index] = value;
        data = mcl::bit_cast<__m128i>(array);
    }

    __m128i data;
};

#    define MCL_HMAP_MATCH_META_BYTE_GROUP(MATCH, ...)                                           \
        {                                                                                        \
            for (u16 match_result{MATCH}; match_result != 0; match_result &= match_result - 1) { \
                const size_t match_index{static_cast<size_t>(std::countr_zero(match_result))};   \
                __VA_ARGS__                                                                      \
            }                                                                                    \
        }

#    define MCL_HMAP_MATCH_META_BYTE_GROUP_EXCEPT_LAST(MATCH, ...)                                                              \
        {                                                                                                                       \
            for (u16 match_result{static_cast<u16>((MATCH) & (0x7fff))}; match_result != 0; match_result &= match_result - 1) { \
                const size_t match_index{static_cast<size_t>(std::countr_zero(match_result))};                                  \
                __VA_ARGS__                                                                                                     \
            }                                                                                                                   \
        }

#else

struct meta_byte_group {
    static constexpr size_t max_group_size{16};

    static constexpr u64 msb{0x8080808080808080};
    static constexpr u64 lsb{0x0101010101010101};
    static constexpr u64 not_msb{0x7f7f7f7f7f7f7f7f};
    static constexpr u64 not_lsb{0xfefefefefefefefe};

    explicit meta_byte_group(meta_byte* ptr)
    {
        std::memcpy(data.data(), ptr, sizeof(data));
    }

    explicit meta_byte_group(const std::array<meta_byte, 16>& array)
        : data{array}
    {}

    std::array<u64, 2> match(meta_byte cmp) const
    {
        DEBUG_ASSERT(is_full(cmp));

        const u64 vcmp{lsb * static_cast<u64>(cmp)};
        return {(msb - ((data[0] ^ vcmp) & not_msb)) & ~data[0] & msb, (msb - ((data[1] ^ vcmp) & not_msb)) & ~data[1] & msb};
    }

    std::array<u64, 2> match_empty_or_tombstone() const
    {
        return {data[0] & msb, data[1] & msb};
    }

    bool is_any_empty() const
    {
        static_assert((static_cast<u8>(meta_byte::empty) & 0xc0) == 0xc0);
        static_assert((static_cast<u8>(meta_byte::tombstone) & 0xc0) == 0x80);

        return (data[0] & (data[0] << 1) & msb) || (data[1] & (data[1] << 1) & msb);
    }

    bool is_all_empty_or_tombstone() const
    {
        return (data[0] & data[1] & msb) == msb;
    }

    meta_byte get(size_t index) const
    {
        return mcl::bit_cast<std::array<meta_byte, max_group_size>>(data)[index];
    }

    void set(size_t index, meta_byte value)
    {
        auto array = mcl::bit_cast<std::array<meta_byte, max_group_size>>(data);
        array[index] = value;
        data = mcl::bit_cast<std::array<u64, 2>>(array);
    }

    std::array<u64, 2> data;
};

#    define MCL_HMAP_MATCH_META_BYTE_GROUP(MATCH, ...)                                                             \
        {                                                                                                          \
            const std::array<u64, 2> match_result{MATCH};                                                          \
                                                                                                                   \
            for (u64 match_result_v{match_result[0]}; match_result_v != 0; match_result_v &= match_result_v - 1) { \
                const size_t match_index{static_cast<size_t>(std::countr_zero(match_result_v) / 8)};               \
                __VA_ARGS__                                                                                        \
            }                                                                                                      \
                                                                                                                   \
            for (u64 match_result_v{match_result[1]}; match_result_v != 0; match_result_v &= match_result_v - 1) { \
                const size_t match_index{static_cast<size_t>(8 + std::countr_zero(match_result_v) / 8)};           \
                __VA_ARGS__                                                                                        \
            }                                                                                                      \
        }

#    define MCL_HMAP_MATCH_META_BYTE_GROUP_EXCEPT_LAST(MATCH, ...)                                                                      \
        {                                                                                                                               \
            const std::array<u64, 2> match_result{MATCH};                                                                               \
                                                                                                                                        \
            for (u64 match_result_v{match_result[0]}; match_result_v != 0; match_result_v &= match_result_v - 1) {                      \
                const size_t match_index{static_cast<size_t>(std::countr_zero(match_result_v) / 8)};                                    \
                __VA_ARGS__                                                                                                             \
            }                                                                                                                           \
                                                                                                                                        \
            for (u64 match_result_v{match_result[1] & 0x00ffffffffffffff}; match_result_v != 0; match_result_v &= match_result_v - 1) { \
                const size_t match_index{static_cast<size_t>(8 + std::countr_zero(match_result_v) / 8)};                                \
                __VA_ARGS__                                                                                                             \
            }                                                                                                                           \
        }

#endif

}  // namespace mcl::detail
