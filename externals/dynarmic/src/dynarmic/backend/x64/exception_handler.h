/* This file is part of the dynarmic project.
 * Copyright (c) 2020 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <functional>
#include <memory>

#include <mcl/stdint.hpp>

namespace Dynarmic::Backend::X64 {

class BlockOfCode;

struct FakeCall {
    u64 call_rip;
    u64 ret_rip;
};

class ExceptionHandler final {
public:
    ExceptionHandler();
    ~ExceptionHandler();

    void Register(BlockOfCode& code);

    bool SupportsFastmem() const noexcept;
    void SetFastmemCallback(std::function<FakeCall(u64)> cb);

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

}  // namespace Dynarmic::Backend::X64
