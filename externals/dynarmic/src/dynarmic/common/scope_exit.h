/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <exception>
#include <type_traits>
#include <utility>

#include "dynarmic/common/macro_util.h"

namespace Dynarmic::detail {

struct ScopeExitTag {};
struct ScopeFailTag {};
struct ScopeSuccessTag {};

template<typename Function>
class ScopeExit final {
public:
    explicit ScopeExit(Function&& fn)
            : function(std::move(fn)) {}
    ~ScopeExit() noexcept {
        function();
    }

private:
    Function function;
};

template<typename Function>
class ScopeFail final {
public:
    explicit ScopeFail(Function&& fn)
            : function(std::move(fn)), exception_count(std::uncaught_exceptions()) {}
    ~ScopeFail() noexcept {
        if (std::uncaught_exceptions() > exception_count) {
            function();
        }
    }

private:
    Function function;
    int exception_count;
};

template<typename Function>
class ScopeSuccess final {
public:
    explicit ScopeSuccess(Function&& fn)
            : function(std::move(fn)), exception_count(std::uncaught_exceptions()) {}
    ~ScopeSuccess() {
        if (std::uncaught_exceptions() <= exception_count) {
            function();
        }
    }

private:
    Function function;
    int exception_count;
};

// We use ->* here as it has the highest precedence of the operators we can use.

template<typename Function>
auto operator->*(ScopeExitTag, Function&& function) {
    return ScopeExit<std::decay_t<Function>>{std::forward<Function>(function)};
}

template<typename Function>
auto operator->*(ScopeFailTag, Function&& function) {
    return ScopeFail<std::decay_t<Function>>{std::forward<Function>(function)};
}

template<typename Function>
auto operator->*(ScopeSuccessTag, Function&& function) {
    return ScopeSuccess<std::decay_t<Function>>{std::forward<Function>(function)};
}

}  // namespace Dynarmic::detail

#define SCOPE_EXIT auto ANONYMOUS_VARIABLE(_SCOPE_EXIT_) = ::Dynarmic::detail::ScopeExitTag{}->*[&]() noexcept
#define SCOPE_FAIL auto ANONYMOUS_VARIABLE(_SCOPE_FAIL_) = ::Dynarmic::detail::ScopeFailTag{}->*[&]() noexcept
#define SCOPE_SUCCESS auto ANONYMOUS_VARIABLE(_SCOPE_FAIL_) = ::Dynarmic::detail::ScopeSuccessTag{}->*[&]()
