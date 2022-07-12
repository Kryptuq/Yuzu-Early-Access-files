// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

#include <stdexcept>
#include <type_traits>

#include <fmt/format.h>

#include "mcl/hint/assume.hpp"

namespace mcl::detail {

[[noreturn]] void assert_terminate_impl(const char* expr_str, fmt::string_view msg, fmt::format_args args);

template<typename... Ts>
[[noreturn]] void assert_terminate(const char* expr_str, fmt::string_view msg, Ts... args)
{
    assert_terminate_impl(expr_str, msg, fmt::make_format_args(args...));
}

}  // namespace mcl::detail

#define UNREACHABLE() ASSERT_FALSE("Unreachable code!")

#define ASSERT(expr)                                                     \
    [&] {                                                                \
        if (std::is_constant_evaluated()) {                              \
            if (!(expr)) {                                               \
                throw std::logic_error{"ASSERT failed at compile time"}; \
            }                                                            \
        } else {                                                         \
            if (!(expr)) [[unlikely]] {                                  \
                ::mcl::detail::assert_terminate(#expr, "(none)");        \
            }                                                            \
        }                                                                \
    }()

#define ASSERT_MSG(expr, ...)                                                \
    [&] {                                                                    \
        if (std::is_constant_evaluated()) {                                  \
            if (!(expr)) {                                                   \
                throw std::logic_error{"ASSERT_MSG failed at compile time"}; \
            }                                                                \
        } else {                                                             \
            if (!(expr)) [[unlikely]] {                                      \
                ::mcl::detail::assert_terminate(#expr, __VA_ARGS__);         \
            }                                                                \
        }                                                                    \
    }()

#define ASSERT_FALSE(...) ::mcl::detail::assert_terminate("false", __VA_ARGS__)

#if defined(NDEBUG) || defined(MCL_IGNORE_ASSERTS)
#    define DEBUG_ASSERT(expr) ASSUME(expr)
#    define DEBUG_ASSERT_MSG(expr, ...) ASSUME(expr)
#else
#    define DEBUG_ASSERT(expr) ASSERT(expr)
#    define DEBUG_ASSERT_MSG(expr, ...) ASSERT_MSG(expr, __VA_ARGS__)
#endif
