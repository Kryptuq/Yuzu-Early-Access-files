// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#include "mcl/assert.hpp"

#include <cstdio>
#include <exception>

#include <fmt/format.h>

namespace mcl::detail {

[[noreturn]] void assert_terminate_impl(fmt::string_view msg, fmt::format_args args) {
    fmt::print(stderr, "assertion failed: ");
    fmt::vprint(stderr, msg, args);
    std::terminate();
}

}  // namespace mcl::detail
