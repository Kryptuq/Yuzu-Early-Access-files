/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <array>
#include <cstdint>
#include <memory>

namespace Dynarmic {
namespace A32 {

struct Context {
public:
    Context();
    ~Context();
    Context(const Context&);
    Context(Context&&) noexcept;
    Context& operator=(const Context&);
    Context& operator=(Context&&) noexcept;

    /// View and modify registers.
    std::array<std::uint32_t, 16>& Regs();
    const std::array<std::uint32_t, 16>& Regs() const;
    std::array<std::uint32_t, 64>& ExtRegs();
    const std::array<std::uint32_t, 64>& ExtRegs() const;

    /// View and modify CPSR.
    std::uint32_t Cpsr() const;
    void SetCpsr(std::uint32_t value);

    /// View and modify FPSCR.
    std::uint32_t Fpscr() const;
    void SetFpscr(std::uint32_t value);

private:
    friend class Jit;
    struct Impl;
    std::unique_ptr<Impl> impl;
};

}  // namespace A32
}  // namespace Dynarmic
