// SPDX-FileCopyrightText: Copyright (c) 2022 merryhime <https://mary.rs>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <cstdint>
#include <variant>

namespace oaknut {

struct Label;

namespace detail {

constexpr std::uint64_t inverse_mask_from_size(std::size_t size)
{
    return (~std::uint64_t{0}) << size;
}

constexpr std::uint64_t mask_from_size(std::size_t size)
{
    return (~std::uint64_t{0}) >> (64 - size);
}

template<std::size_t bit_count>
constexpr std::uint64_t sign_extend(std::uint64_t value)
{
    static_assert(bit_count != 0, "cannot sign-extend zero-sized value");
    constexpr size_t shift_amount = 64 - bit_count;
    return static_cast<std::uint64_t>(static_cast<std::int64_t>(value << shift_amount) >> shift_amount);
}

}  // namespace detail

template<std::size_t bitsize, std::size_t alignment>
struct AddrOffset {
    AddrOffset(std::ptrdiff_t diff)
        : m_payload(encode(diff))
    {}

    AddrOffset(Label& label)
        : m_payload(&label)
    {}

    AddrOffset(void* ptr)
        : m_payload(ptr)
    {}

    static std::uint32_t encode(std::ptrdiff_t diff)
    {
        const std::uint64_t diff_u64 = static_cast<std::uint64_t>(diff);
        if (detail::sign_extend<bitsize>(diff_u64) != diff_u64)
            throw "out of range";
        if (diff_u64 != (diff_u64 & detail::inverse_mask_from_size(alignment)))
            throw "misalignment";

        return static_cast<std::uint32_t>((diff_u64 & detail::mask_from_size(bitsize)) >> alignment);
    }

private:
    template<typename Policy>
    friend class BasicCodeGenerator;
    std::variant<std::uint32_t, Label*, void*> m_payload;
};

template<std::size_t bitsize>
struct PageOffset {
    PageOffset(void* ptr)
        : m_payload(ptr)
    {}

    PageOffset(Label& label)
        : m_payload(&label)
    {}

    static std::uint32_t encode(std::uintptr_t current_addr, std::uintptr_t target)
    {
        const std::int64_t page_diff = (static_cast<std::int64_t>(target) >> 12) - (static_cast<std::int64_t>(current_addr) >> 12);
        if (detail::sign_extend<bitsize>(page_diff) != page_diff)
            throw "out of range";
        return static_cast<std::uint32_t>(page_diff & detail::mask_from_size(bitsize));
    }

private:
    template<typename Policy>
    friend class BasicCodeGenerator;
    std::variant<Label*, void*> m_payload;
};

template<std::size_t bitsize, std::size_t alignment>
struct SOffset {
    SOffset(std::int64_t offset)
    {
        const std::uint64_t diff_u64 = static_cast<std::uint64_t>(offset);
        if (detail::sign_extend<bitsize>(diff_u64) != diff_u64)
            throw "out of range";
        if (diff_u64 != (diff_u64 & detail::inverse_mask_from_size(alignment)))
            throw "misalignment";

        m_encoded = static_cast<std::uint32_t>((diff_u64 & detail::mask_from_size(bitsize)) >> alignment);
    }

private:
    template<typename Policy>
    friend class BasicCodeGenerator;
    std::uint32_t m_encoded;
};

template<std::size_t bitsize, std::size_t alignment>
struct POffset {
    POffset(std::int64_t offset)
    {
        const std::uint64_t diff_u64 = static_cast<std::uint64_t>(offset);
        if (diff_u64 > detail::mask_from_size(bitsize))
            throw "out of range";
        if (diff_u64 != (diff_u64 & detail::inverse_mask_from_size(alignment)))
            throw "misalignment";

        m_encoded = static_cast<std::uint32_t>((diff_u64 & detail::mask_from_size(bitsize)) >> alignment);
    }

private:
    template<typename Policy>
    friend class BasicCodeGenerator;
    std::uint32_t m_encoded;
};

}  // namespace oaknut
