// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

#if defined(__clang) || defined(__GNUC__)
#    define ASSUME(expr) [&] { if (!(expr)) __builtin_unreachable(); }()
#elif defined(_MSC_VER)
#    define ASSUME(expr) __assume(expr)
#else
#    define ASSUME(expr)
#endif
