// This file is part of the mcl project.
// Copyright (c) 2022 merryhime
// SPDX-License-Identifier: MIT

#pragma once

#include <exception>
#include <type_traits>
#include <utility>

#include <mcl/macro/anonymous_variable.hpp>

namespace mcl::detail {

struct scope_exit_tag {};
struct scope_fail_tag {};
struct scope_success_tag {};

template<typename Function>
class scope_exit final {
public:
    explicit scope_exit(Function&& fn)
        : function(std::move(fn)) {}

    ~scope_exit() noexcept
    {
        function();
    }

private:
    Function function;
};

template<typename Function>
class scope_fail final {
public:
    explicit scope_fail(Function&& fn)
        : function(std::move(fn)), exception_count(std::uncaught_exceptions()) {}

    ~scope_fail() noexcept
    {
        if (std::uncaught_exceptions() > exception_count) {
            function();
        }
    }

private:
    Function function;
    int exception_count;
};

template<typename Function>
class scope_success final {
public:
    explicit scope_success(Function&& fn)
        : function(std::move(fn)), exception_count(std::uncaught_exceptions()) {}

    ~scope_success()
    {
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
auto operator->*(scope_exit_tag, Function&& function)
{
    return scope_exit<std::decay_t<Function>>{std::forward<Function>(function)};
}

template<typename Function>
auto operator->*(scope_fail_tag, Function&& function)
{
    return scope_fail<std::decay_t<Function>>{std::forward<Function>(function)};
}

template<typename Function>
auto operator->*(scope_success_tag, Function&& function)
{
    return scope_success<std::decay_t<Function>>{std::forward<Function>(function)};
}

}  // namespace mcl::detail

#define SCOPE_EXIT auto ANONYMOUS_VARIABLE(MCL_SCOPE_EXIT_VAR_) = ::mcl::detail::scope_exit_tag{}->*[&]() noexcept
#define SCOPE_FAIL auto ANONYMOUS_VARIABLE(MCL_SCOPE_FAIL_VAR_) = ::mcl::detail::scope_fail_tag{}->*[&]() noexcept
#define SCOPE_SUCCESS auto ANONYMOUS_VARIABLE(MCL_SCOPE_FAIL_VAR_) = ::mcl::detail::scope_success_tag{}->*[&]()
