/* This file is part of the mp project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <cstddef>
#include <cstdint>

namespace mp {

namespace detail {

template<std::size_t size>
struct integer_of_size_impl{};

template<>
struct integer_of_size_impl<8> {
    using unsigned_type = std::uint8_t;
    using signed_type = std::int8_t;
};

template<>
struct integer_of_size_impl<16> {
    using unsigned_type = std::uint16_t;
    using signed_type = std::int16_t;
};

template<>
struct integer_of_size_impl<32> {
    using unsigned_type = std::uint32_t;
    using signed_type = std::int32_t;
};

template<>
struct integer_of_size_impl<64> {
    using unsigned_type = std::uint64_t;
    using signed_type = std::int64_t;
};

} // namespace detail

template<std::size_t size>
using unsigned_integer_of_size = typename detail::integer_of_size_impl<size>::unsigned_type;

template<std::size_t size>
using signed_integer_of_size = typename detail::integer_of_size_impl<size>::signed_type;

} // namespace mp
