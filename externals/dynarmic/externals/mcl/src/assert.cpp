// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#include "mcl/assert.hpp"

#include <cstdio>
#include <exception>

#include <fmt/format.h>

namespace mcl::detail {

[[noreturn]] void assert_terminate_impl(const char* expr_str, fmt::string_view msg, fmt::format_args args)
{
    fmt::print(stderr, "assertion failed: {}\nMessage:", expr_str);
    fmt::vprint(stderr, msg, args);
    std::fflush(stderr);
    std::terminate();
}

}  // namespace mcl::detail
