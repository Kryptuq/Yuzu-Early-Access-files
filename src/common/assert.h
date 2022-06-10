// SPDX-FileCopyrightText: 2013 Dolphin Emulator Project
// SPDX-FileCopyrightText: 2014 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <functional>

#include "common/logging/log.h"

// Sometimes we want to try to continue even after hitting an assert.
// However touching this file yields a global recompilation as this header is included almost
// everywhere. So let's just move the handling of the failed assert to a single cpp file.

// For asserts we'd like to keep all the junk executed when an assert happens away from the
// important code in the function. One way of doing this is to put all the relevant code inside a
// lambda and force the compiler to not inline it.
void assert_check_condition(bool cond, std::function<void()>&& on_failure);

[[noreturn]] void unreachable_impl();

#define ASSERT(_a_)                                                                                \
    do {                                                                                           \
        if (std::is_constant_evaluated()) {                                                        \
            if (!(_a_)) {                                                                          \
                /* Will trigger compile error here */                                              \
                assert_check_condition(bool(_a_),                                                  \
                                       [] { LOG_CRITICAL(Debug, "Assertion Failed!"); });          \
            }                                                                                      \
        } else {                                                                                   \
            assert_check_condition(bool(_a_), [] { LOG_CRITICAL(Debug, "Assertion Failed!"); });   \
        }                                                                                          \
    } while (0)

#define ASSERT_MSG(_a_, ...)                                                                       \
    do {                                                                                           \
        if (std::is_constant_evaluated()) {                                                        \
            if (!(_a_)) {                                                                          \
                /* Will trigger compile error here */                                              \
                assert_check_condition(bool(_a_),                                                  \
                                       [] { LOG_CRITICAL(Debug, "Assertion Failed!"); });          \
            }                                                                                      \
        } else {                                                                                   \
            assert_check_condition(                                                                \
                bool(_a_), [&] { LOG_CRITICAL(Debug, "Assertion Failed!\n" __VA_ARGS__); });       \
        }                                                                                          \
    } while (0)

#define UNREACHABLE()                                                                              \
    do {                                                                                           \
        LOG_CRITICAL(Debug, "Unreachable code!");                                                  \
        unreachable_impl();                                                                        \
    } while (0)

#define UNREACHABLE_MSG(...)                                                                       \
    do {                                                                                           \
        LOG_CRITICAL(Debug, "Unreachable code!\n" __VA_ARGS__);                                    \
        unreachable_impl();                                                                        \
    } while (0)

#ifdef _DEBUG
#define DEBUG_ASSERT(_a_) ASSERT(_a_)
#define DEBUG_ASSERT_MSG(_a_, ...) ASSERT_MSG(_a_, __VA_ARGS__)
#else // not debug
#define DEBUG_ASSERT(_a_)                                                                          \
    do {                                                                                           \
    } while (0)
#define DEBUG_ASSERT_MSG(_a_, _desc_, ...)                                                         \
    do {                                                                                           \
    } while (0)
#endif

#define UNIMPLEMENTED() ASSERT_MSG(false, "Unimplemented code!")
#define UNIMPLEMENTED_MSG(...) ASSERT_MSG(false, __VA_ARGS__)

#define UNIMPLEMENTED_IF(cond) ASSERT_MSG(!(cond), "Unimplemented code!")
#define UNIMPLEMENTED_IF_MSG(cond, ...) ASSERT_MSG(!(cond), __VA_ARGS__)

// If the assert is ignored, execute _b_
#define ASSERT_OR_EXECUTE(_a_, _b_)                                                                \
    do {                                                                                           \
        ASSERT(_a_);                                                                               \
        if (!(_a_)) {                                                                              \
            _b_                                                                                    \
        }                                                                                          \
    } while (0)

// If the assert is ignored, execute _b_
#define ASSERT_OR_EXECUTE_MSG(_a_, _b_, ...)                                                       \
    do {                                                                                           \
        ASSERT_MSG(_a_, __VA_ARGS__);                                                              \
        if (!(_a_)) {                                                                              \
            _b_                                                                                    \
        }                                                                                          \
    } while (0)
