// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>

#include "common/common_types.h"

namespace Kernel {
class KThread;
}

namespace Core {

class GDBStubArch {
public:
    virtual std::string GetTargetXML() const = 0;
    virtual std::string RegRead(const Kernel::KThread* thread, size_t id) const = 0;
    virtual void RegWrite(Kernel::KThread* thread, size_t id, std::string_view value) const = 0;
    virtual std::string ReadRegisters(const Kernel::KThread* thread) const = 0;
    virtual void WriteRegisters(Kernel::KThread* thread, std::string_view register_data) const = 0;
    virtual std::string ThreadStatus(const Kernel::KThread* thread, u8 signal) const = 0;
    virtual u32 BreakpointInstruction() const = 0;
};

class GDBStubA64 final : public GDBStubArch {
public:
    std::string GetTargetXML() const override;
    std::string RegRead(const Kernel::KThread* thread, size_t id) const override;
    void RegWrite(Kernel::KThread* thread, size_t id, std::string_view value) const override;
    std::string ReadRegisters(const Kernel::KThread* thread) const override;
    void WriteRegisters(Kernel::KThread* thread, std::string_view register_data) const override;
    std::string ThreadStatus(const Kernel::KThread* thread, u8 signal) const override;
    u32 BreakpointInstruction() const override;

private:
    static constexpr u32 LR_REGISTER = 30;
    static constexpr u32 SP_REGISTER = 31;
    static constexpr u32 PC_REGISTER = 32;
    static constexpr u32 PSTATE_REGISTER = 33;
    static constexpr u32 Q0_REGISTER = 34;
    static constexpr u32 FPCR_REGISTER = 66;
    static constexpr u32 FPSR_REGISTER = 67;
};

class GDBStubA32 final : public GDBStubArch {
public:
    std::string GetTargetXML() const override;
    std::string RegRead(const Kernel::KThread* thread, size_t id) const override;
    void RegWrite(Kernel::KThread* thread, size_t id, std::string_view value) const override;
    std::string ReadRegisters(const Kernel::KThread* thread) const override;
    void WriteRegisters(Kernel::KThread* thread, std::string_view register_data) const override;
    std::string ThreadStatus(const Kernel::KThread* thread, u8 signal) const override;
    u32 BreakpointInstruction() const override;

private:
    static constexpr u32 SP_REGISTER = 13;
    static constexpr u32 LR_REGISTER = 14;
    static constexpr u32 PC_REGISTER = 15;
    static constexpr u32 CPSR_REGISTER = 25;
    static constexpr u32 D0_REGISTER = 32;
    static constexpr u32 Q0_REGISTER = 64;
    static constexpr u32 FPSCR_REGISTER = 80;
};

} // namespace Core
