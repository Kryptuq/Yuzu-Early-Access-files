// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

#include "mcl/assert.hpp"
#include "mcl/bitsizeof.hpp"
#include "mcl/concepts/bit_integral.hpp"
#include "mcl/stdint.hpp"

namespace mcl::bit {

/// Create a mask with `count` number of one bits.
template<size_t count, BitIntegral T>
constexpr T ones() {
    static_assert(count <= bitsizeof<T>, "count larger than bitsize of T");

    if constexpr (count == 0) {
        return 0;
    } else {
        return static_cast<T>(~static_cast<T>(0)) >> (bitsizeof<T> - count);
    }
}

/// Create a mask with `count` number of one bits.
template<BitIntegral T>
constexpr T ones(size_t count) {
    ASSERT_MSG(count <= bitsizeof<T>, "count larger than bitsize of T");

    if (count == 0) {
        return 0;
    }
    return static_cast<T>(~static_cast<T>(0)) >> (bitsizeof<T> - count);
}

/// Create a mask of type T for bits [begin_bit, end_bit] inclusive.
template<size_t begin_bit, size_t end_bit, BitIntegral T>
constexpr T mask() {
    static_assert(begin_bit <= end_bit, "invalid bit range (position of beginning bit cannot be greater than that of end bit)");
    static_assert(begin_bit < bitsizeof<T>, "begin_bit must be smaller than size of T");
    static_assert(end_bit < bitsizeof<T>, "end_bit must be smaller than size of T");

    return ones<end_bit - begin_bit + 1, T>() << begin_bit;
}

/// Create a mask of type T for bits [begin_bit, end_bit] inclusive.
template<BitIntegral T>
constexpr T mask(size_t begin_bit, size_t end_bit) {
    ASSERT_MSG(begin_bit <= end_bit, "invalid bit range (position of beginning bit cannot be greater than that of end bit)");
    ASSERT_MSG(begin_bit < bitsizeof<T>, "begin_bit must be smaller than size of T");
    ASSERT_MSG(end_bit < bitsizeof<T>, "end_bit must be smaller than size of T");

    return ones<T>(end_bit - begin_bit + 1) << begin_bit;
}

/// Extract bits [begin_bit, end_bit] inclusive from value of type T.
template<size_t begin_bit, size_t end_bit, BitIntegral T>
constexpr T get_bits(T value) {
    constexpr T m = mask<begin_bit, end_bit, T>();
    return (value & m) >> begin_bit;
}

/// Extract bits [begin_bit, end_bit] inclusive from value of type T.
template<BitIntegral T>
constexpr T get_bits(size_t begin_bit, size_t end_bit, T value) {
    const T m = mask<T>(begin_bit, end_bit);
    return (value & m) >> begin_bit;
}

/// Clears bits [begin_bit, end_bit] inclusive of value of type T.
template<size_t begin_bit, size_t end_bit, BitIntegral T>
constexpr T clear_bits(T value) {
    constexpr T m = mask<begin_bit, end_bit, T>();
    return value & ~m;
}

/// Clears bits [begin_bit, end_bit] inclusive of value of type T.
template<BitIntegral T>
constexpr T clear_bits(size_t begin_bit, size_t end_bit, T value) {
    const T m = mask<T>(begin_bit, end_bit);
    return value & ~m;
}

/// Modifies bits [begin_bit, end_bit] inclusive of value of type T.
template<size_t begin_bit, size_t end_bit, BitIntegral T>
constexpr T set_bits(T value, T new_bits) {
    constexpr T m = mask<begin_bit, end_bit, T>();
    return (value & ~m) | ((new_bits << begin_bit) & m);
}

/// Modifies bits [begin_bit, end_bit] inclusive of value of type T.
template<BitIntegral T>
constexpr T set_bits(size_t begin_bit, size_t end_bit, T value, T new_bits) {
    const T m = mask<T>(begin_bit, end_bit);
    return (value & ~m) | ((new_bits << begin_bit) & m);
}

/// Extract bit at bit_position from value of type T.
template<size_t bit_position, BitIntegral T>
constexpr bool get_bit(T value) {
    constexpr T m = mask<bit_position, bit_position, T>();
    return (value & m) != 0;
}

/// Extract bit at bit_position from value of type T.
template<BitIntegral T>
constexpr bool get_bit(size_t bit_position, T value) {
    const T m = mask<T>(bit_position, bit_position);
    return (value & m) != 0;
}

/// Clears bit at bit_position of value of type T.
template<size_t bit_position, BitIntegral T>
constexpr T clear_bit(T value) {
    constexpr T m = mask<bit_position, bit_position, T>();
    return value & ~m;
}

/// Clears bit at bit_position of value of type T.
template<BitIntegral T>
constexpr T clear_bit(size_t bit_position, T value) {
    const T m = mask<T>(bit_position, bit_position);
    return value & ~m;
}

/// Modifies bit at bit_position of value of type T.
template<size_t bit_position, BitIntegral T>
constexpr T set_bit(T value, bool new_bit) {
    constexpr T m = mask<bit_position, bit_position, T>();
    return (value & ~m) | (new_bit ? m : static_cast<T>(0));
}

/// Modifies bit at bit_position of value of type T.
template<BitIntegral T>
constexpr T set_bit(size_t bit_position, T value, bool new_bit) {
    const T m = mask<T>(bit_position, bit_position);
    return (value & ~m) | (new_bit ? m : static_cast<T>(0));
}

/// Sign-extends a value that has bit_count bits to the full bitwidth of type T.
template<size_t bit_count, BitIntegral T>
constexpr T sign_extend(T value) {
    static_assert(bit_count != 0, "cannot sign-extend zero-sized value");

    constexpr T m = ones<bit_count, T>();
    if (get_bit<bit_count - 1, T>(value)) {
        return value | ~m;
    }
    return value;
}

/// Sign-extends a value that has bit_count bits to the full bitwidth of type T.
template<BitIntegral T>
constexpr T sign_extend(size_t bit_count, T value) {
    ASSERT_MSG(bit_count != 0, "cannot sign-extend zero-sized value");

    const T m = ones<T>(bit_count);
    if (get_bit<T>(bit_count - 1, value)) {
        return value | ~m;
    }
    return value;
}

/// Replicate an element across a value of type T.
template<size_t element_size, BitIntegral T>
constexpr T replicate_element(T value) {
    static_assert(element_size <= bitsizeof<T>, "element_size is too large");
    static_assert(bitsizeof<T> % element_size == 0, "bitsize of T not divisible by element_size");

    if constexpr (element_size == bitsizeof<T>) {
        return value;
    } else {
        return replicate_element<element_size * 2, T>(static_cast<T>(value | (value << element_size)));
    }
}

/// Replicate an element of type U across a value of type T.
template<BitIntegral U, BitIntegral T>
constexpr T replicate_element(T value) {
    static_assert(bitsizeof<U> <= bitsizeof<T>, "element_size is too large");

    return replicate_element<bitsizeof<U>, T>(value);
}

/// Replicate an element across a value of type T.
template<BitIntegral T>
constexpr T replicate_element(size_t element_size, T value) {
    ASSERT_MSG(element_size <= bitsizeof<T>, "element_size is too large");
    ASSERT_MSG(bitsizeof<T> % element_size == 0, "bitsize of T not divisible by element_size");

    if (element_size == bitsizeof<T>) {
        return value;
    }
    return replicate_element<T>(element_size * 2, static_cast<T>(value | (value << element_size)));
}

template<BitIntegral T>
constexpr bool most_significant_bit(T value) {
    return get_bit<bitsizeof<T> - 1, T>(value);
}

}  // namespace mcl::bit
