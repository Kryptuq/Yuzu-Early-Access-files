/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#if defined(__clang__) || defined(__GNUC__)
#    define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#    define UNLIKELY(x) !!(x)
#endif
