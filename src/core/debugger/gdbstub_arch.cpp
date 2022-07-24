// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/hex_util.h"
#include "core/debugger/gdbstub_arch.h"
#include "core/hle/kernel/k_thread.h"

namespace Core {

template <typename T>
static T HexToValue(std::string_view hex) {
    static_assert(std::is_trivially_copyable_v<T>);
    T value{};
    const auto mem{Common::HexStringToVector(hex, false)};
    std::memcpy(&value, mem.data(), std::min(mem.size(), sizeof(T)));
    return value;
}

template <typename T>
static std::string ValueToHex(const T value) {
    static_assert(std::is_trivially_copyable_v<T>);
    std::array<u8, sizeof(T)> mem{};
    std::memcpy(mem.data(), &value, sizeof(T));
    return Common::HexToString(mem);
}

template <typename T>
static T GetSIMDRegister(const std::array<u32, 64>& simd_regs, size_t offset) {
    static_assert(std::is_trivially_copyable_v<T>);
    T value{};
    std::memcpy(&value, reinterpret_cast<const u8*>(simd_regs.data()) + sizeof(T) * offset,
                sizeof(T));
    return value;
}

template <typename T>
static void PutSIMDRegister(std::array<u32, 64>& simd_regs, size_t offset, const T value) {
    static_assert(std::is_trivially_copyable_v<T>);
    std::memcpy(reinterpret_cast<u8*>(simd_regs.data()) + sizeof(T) * offset, &value, sizeof(T));
}

// For sample XML files see the GDB source /gdb/features
// This XML defines what the registers are for this specific ARM device
std::string GDBStubA64::GetTargetXML() const {
    constexpr const char* target_xml =
        R"(<?xml version="1.0"?>
<!DOCTYPE target SYSTEM "gdb-target.dtd">
<target version="1.0">
  <architecture>aarch64</architecture>
  <feature name="org.gnu.gdb.aarch64.core">
    <reg name="x0" bitsize="64"/>
    <reg name="x1" bitsize="64"/>
    <reg name="x2" bitsize="64"/>
    <reg name="x3" bitsize="64"/>
    <reg name="x4" bitsize="64"/>
    <reg name="x5" bitsize="64"/>
    <reg name="x6" bitsize="64"/>
    <reg name="x7" bitsize="64"/>
    <reg name="x8" bitsize="64"/>
    <reg name="x9" bitsize="64"/>
    <reg name="x10" bitsize="64"/>
    <reg name="x11" bitsize="64"/>
    <reg name="x12" bitsize="64"/>
    <reg name="x13" bitsize="64"/>
    <reg name="x14" bitsize="64"/>
    <reg name="x15" bitsize="64"/>
    <reg name="x16" bitsize="64"/>
    <reg name="x17" bitsize="64"/>
    <reg name="x18" bitsize="64"/>
    <reg name="x19" bitsize="64"/>
    <reg name="x20" bitsize="64"/>
    <reg name="x21" bitsize="64"/>
    <reg name="x22" bitsize="64"/>
    <reg name="x23" bitsize="64"/>
    <reg name="x24" bitsize="64"/>
    <reg name="x25" bitsize="64"/>
    <reg name="x26" bitsize="64"/>
    <reg name="x27" bitsize="64"/>
    <reg name="x28" bitsize="64"/>
    <reg name="x29" bitsize="64"/>
    <reg name="x30" bitsize="64"/>
    <reg name="sp" bitsize="64" type="data_ptr"/>
    <reg name="pc" bitsize="64" type="code_ptr"/>
    <flags id="cpsr_flags" size="4">
      <field name="SP" start="0" end="0"/>
      <field name="" start="1" end="1"/>
      <field name="EL" start="2" end="3"/>
      <field name="nRW" start="4" end="4"/>
      <field name="" start="5" end="5"/>
      <field name="F" start="6" end="6"/>
      <field name="I" start="7" end="7"/>
      <field name="A" start="8" end="8"/>
      <field name="D" start="9" end="9"/>
      <field name="IL" start="20" end="20"/>
      <field name="SS" start="21" end="21"/>
      <field name="V" start="28" end="28"/>
      <field name="C" start="29" end="29"/>
      <field name="Z" start="30" end="30"/>
      <field name="N" start="31" end="31"/>
    </flags>
    <reg name="cpsr" bitsize="32" type="cpsr_flags"/>
  </feature>
  <feature name="org.gnu.gdb.aarch64.fpu">
    <vector id="v2d" type="ieee_double" count="2"/>
    <vector id="v2u" type="uint64" count="2"/>
    <vector id="v2i" type="int64" count="2"/>
    <vector id="v4f" type="ieee_single" count="4"/>
    <vector id="v4u" type="uint32" count="4"/>
    <vector id="v4i" type="int32" count="4"/>
    <vector id="v8u" type="uint16" count="8"/>
    <vector id="v8i" type="int16" count="8"/>
    <vector id="v16u" type="uint8" count="16"/>
    <vector id="v16i" type="int8" count="16"/>
    <vector id="v1u" type="uint128" count="1"/>
    <vector id="v1i" type="int128" count="1"/>
    <union id="vnd">
      <field name="f" type="v2d"/>
      <field name="u" type="v2u"/>
      <field name="s" type="v2i"/>
    </union>
    <union id="vns">
      <field name="f" type="v4f"/>
      <field name="u" type="v4u"/>
      <field name="s" type="v4i"/>
    </union>
    <union id="vnh">
      <field name="u" type="v8u"/>
      <field name="s" type="v8i"/>
    </union>
    <union id="vnb">
      <field name="u" type="v16u"/>
      <field name="s" type="v16i"/>
    </union>
    <union id="vnq">
      <field name="u" type="v1u"/>
      <field name="s" type="v1i"/>
    </union>
    <union id="aarch64v">
      <field name="d" type="vnd"/>
      <field name="s" type="vns"/>
      <field name="h" type="vnh"/>
      <field name="b" type="vnb"/>
      <field name="q" type="vnq"/>
    </union>
    <reg name="v0" bitsize="128" type="aarch64v" regnum="34"/>
    <reg name="v1" bitsize="128" type="aarch64v" />
    <reg name="v2" bitsize="128" type="aarch64v" />
    <reg name="v3" bitsize="128" type="aarch64v" />
    <reg name="v4" bitsize="128" type="aarch64v" />
    <reg name="v5" bitsize="128" type="aarch64v" />
    <reg name="v6" bitsize="128" type="aarch64v" />
    <reg name="v7" bitsize="128" type="aarch64v" />
    <reg name="v8" bitsize="128" type="aarch64v" />
    <reg name="v9" bitsize="128" type="aarch64v" />
    <reg name="v10" bitsize="128" type="aarch64v"/>
    <reg name="v11" bitsize="128" type="aarch64v"/>
    <reg name="v12" bitsize="128" type="aarch64v"/>
    <reg name="v13" bitsize="128" type="aarch64v"/>
    <reg name="v14" bitsize="128" type="aarch64v"/>
    <reg name="v15" bitsize="128" type="aarch64v"/>
    <reg name="v16" bitsize="128" type="aarch64v"/>
    <reg name="v17" bitsize="128" type="aarch64v"/>
    <reg name="v18" bitsize="128" type="aarch64v"/>
    <reg name="v19" bitsize="128" type="aarch64v"/>
    <reg name="v20" bitsize="128" type="aarch64v"/>
    <reg name="v21" bitsize="128" type="aarch64v"/>
    <reg name="v22" bitsize="128" type="aarch64v"/>
    <reg name="v23" bitsize="128" type="aarch64v"/>
    <reg name="v24" bitsize="128" type="aarch64v"/>
    <reg name="v25" bitsize="128" type="aarch64v"/>
    <reg name="v26" bitsize="128" type="aarch64v"/>
    <reg name="v27" bitsize="128" type="aarch64v"/>
    <reg name="v28" bitsize="128" type="aarch64v"/>
    <reg name="v29" bitsize="128" type="aarch64v"/>
    <reg name="v30" bitsize="128" type="aarch64v"/>
    <reg name="v31" bitsize="128" type="aarch64v"/>
    <reg name="fpsr" bitsize="32"/>
    <reg name="fpcr" bitsize="32"/>
  </feature>
</target>)";

    return target_xml;
}

std::string GDBStubA64::RegRead(const Kernel::KThread* thread, size_t id) const {
    if (!thread) {
        return "";
    }

    const auto& context{thread->GetContext64()};
    const auto& gprs{context.cpu_registers};
    const auto& fprs{context.vector_registers};

    if (id < SP_REGISTER) {
        return ValueToHex(gprs[id]);
    } else if (id == SP_REGISTER) {
        return ValueToHex(context.sp);
    } else if (id == PC_REGISTER) {
        return ValueToHex(context.pc);
    } else if (id == PSTATE_REGISTER) {
        return ValueToHex(context.pstate);
    } else if (id >= Q0_REGISTER && id < FPSR_REGISTER) {
        return ValueToHex(fprs[id - Q0_REGISTER]);
    } else if (id == FPSR_REGISTER) {
        return ValueToHex(context.fpsr);
    } else if (id == FPCR_REGISTER) {
        return ValueToHex(context.fpcr);
    } else {
        return "";
    }
}

void GDBStubA64::RegWrite(Kernel::KThread* thread, size_t id, std::string_view value) const {
    if (!thread) {
        return;
    }

    auto& context{thread->GetContext64()};

    if (id < SP_REGISTER) {
        context.cpu_registers[id] = HexToValue<u64>(value);
    } else if (id == SP_REGISTER) {
        context.sp = HexToValue<u64>(value);
    } else if (id == PC_REGISTER) {
        context.pc = HexToValue<u64>(value);
    } else if (id == PSTATE_REGISTER) {
        context.pstate = HexToValue<u32>(value);
    } else if (id >= Q0_REGISTER && id < FPSR_REGISTER) {
        context.vector_registers[id - Q0_REGISTER] = HexToValue<u128>(value);
    } else if (id == FPSR_REGISTER) {
        context.fpsr = HexToValue<u32>(value);
    } else if (id == FPCR_REGISTER) {
        context.fpcr = HexToValue<u32>(value);
    }
}

std::string GDBStubA64::ReadRegisters(const Kernel::KThread* thread) const {
    std::string output;

    for (size_t reg = 0; reg <= FPCR_REGISTER; reg++) {
        output += RegRead(thread, reg);
    }

    return output;
}

void GDBStubA64::WriteRegisters(Kernel::KThread* thread, std::string_view register_data) const {
    for (size_t i = 0, reg = 0; reg <= FPCR_REGISTER; reg++) {
        if (reg <= SP_REGISTER || reg == PC_REGISTER) {
            RegWrite(thread, reg, register_data.substr(i, 16));
            i += 16;
        } else if (reg == PSTATE_REGISTER || reg == FPCR_REGISTER || reg == FPSR_REGISTER) {
            RegWrite(thread, reg, register_data.substr(i, 8));
            i += 8;
        } else if (reg >= Q0_REGISTER && reg < FPCR_REGISTER) {
            RegWrite(thread, reg, register_data.substr(i, 32));
            i += 32;
        }
    }
}

std::string GDBStubA64::ThreadStatus(const Kernel::KThread* thread, u8 signal) const {
    return fmt::format("T{:02x}{:02x}:{};{:02x}:{};{:02x}:{};thread:{:x};", signal, PC_REGISTER,
                       RegRead(thread, PC_REGISTER), SP_REGISTER, RegRead(thread, SP_REGISTER),
                       LR_REGISTER, RegRead(thread, LR_REGISTER), thread->GetThreadID());
}

u32 GDBStubA64::BreakpointInstruction() const {
    // A64: brk #0
    return 0xd4200000;
}

std::string GDBStubA32::GetTargetXML() const {
    constexpr const char* target_xml =
        R"(<?xml version="1.0"?>
<!DOCTYPE target SYSTEM "gdb-target.dtd">
<target version="1.0">
  <architecture>arm</architecture>
  <feature name="org.gnu.gdb.arm.core">
    <reg name="r0" bitsize="32" type="uint32"/>
    <reg name="r1" bitsize="32" type="uint32"/>
    <reg name="r2" bitsize="32" type="uint32"/>
    <reg name="r3" bitsize="32" type="uint32"/>
    <reg name="r4" bitsize="32" type="uint32"/>
    <reg name="r5" bitsize="32" type="uint32"/>
    <reg name="r6" bitsize="32" type="uint32"/>
    <reg name="r7" bitsize="32" type="uint32"/>
    <reg name="r8" bitsize="32" type="uint32"/>
    <reg name="r9" bitsize="32" type="uint32"/>
    <reg name="r10" bitsize="32" type="uint32"/>
    <reg name="r11" bitsize="32" type="uint32"/>
    <reg name="r12" bitsize="32" type="uint32"/>
    <reg name="sp" bitsize="32" type="data_ptr"/>
    <reg name="lr" bitsize="32" type="code_ptr"/>
    <reg name="pc" bitsize="32" type="code_ptr"/>
    <!-- The CPSR is register 25, rather than register 16, because
         the FPA registers historically were placed between the PC
         and the CPSR in the "g" packet.  -->
    <reg name="cpsr" bitsize="32" regnum="25"/>
  </feature>
  <feature name="org.gnu.gdb.arm.vfp">
    <vector id="neon_uint8x8" type="uint8" count="8"/>
    <vector id="neon_uint16x4" type="uint16" count="4"/>
    <vector id="neon_uint32x2" type="uint32" count="2"/>
    <vector id="neon_float32x2" type="ieee_single" count="2"/>
    <union id="neon_d">
      <field name="u8" type="neon_uint8x8"/>
      <field name="u16" type="neon_uint16x4"/>
      <field name="u32" type="neon_uint32x2"/>
      <field name="u64" type="uint64"/>
      <field name="f32" type="neon_float32x2"/>
      <field name="f64" type="ieee_double"/>
    </union>
    <vector id="neon_uint8x16" type="uint8" count="16"/>
    <vector id="neon_uint16x8" type="uint16" count="8"/>
    <vector id="neon_uint32x4" type="uint32" count="4"/>
    <vector id="neon_uint64x2" type="uint64" count="2"/>
    <vector id="neon_float32x4" type="ieee_single" count="4"/>
    <vector id="neon_float64x2" type="ieee_double" count="2"/>
    <union id="neon_q">
      <field name="u8" type="neon_uint8x16"/>
      <field name="u16" type="neon_uint16x8"/>
      <field name="u32" type="neon_uint32x4"/>
      <field name="u64" type="neon_uint64x2"/>
      <field name="f32" type="neon_float32x4"/>
      <field name="f64" type="neon_float64x2"/>
    </union>
    <reg name="d0" bitsize="64" type="neon_d" regnum="32"/>
    <reg name="d1" bitsize="64" type="neon_d"/>
    <reg name="d2" bitsize="64" type="neon_d"/>
    <reg name="d3" bitsize="64" type="neon_d"/>
    <reg name="d4" bitsize="64" type="neon_d"/>
    <reg name="d5" bitsize="64" type="neon_d"/>
    <reg name="d6" bitsize="64" type="neon_d"/>
    <reg name="d7" bitsize="64" type="neon_d"/>
    <reg name="d8" bitsize="64" type="neon_d"/>
    <reg name="d9" bitsize="64" type="neon_d"/>
    <reg name="d10" bitsize="64" type="neon_d"/>
    <reg name="d11" bitsize="64" type="neon_d"/>
    <reg name="d12" bitsize="64" type="neon_d"/>
    <reg name="d13" bitsize="64" type="neon_d"/>
    <reg name="d14" bitsize="64" type="neon_d"/>
    <reg name="d15" bitsize="64" type="neon_d"/>
    <reg name="d16" bitsize="64" type="neon_d"/>
    <reg name="d17" bitsize="64" type="neon_d"/>
    <reg name="d18" bitsize="64" type="neon_d"/>
    <reg name="d19" bitsize="64" type="neon_d"/>
    <reg name="d20" bitsize="64" type="neon_d"/>
    <reg name="d21" bitsize="64" type="neon_d"/>
    <reg name="d22" bitsize="64" type="neon_d"/>
    <reg name="d23" bitsize="64" type="neon_d"/>
    <reg name="d24" bitsize="64" type="neon_d"/>
    <reg name="d25" bitsize="64" type="neon_d"/>
    <reg name="d26" bitsize="64" type="neon_d"/>
    <reg name="d27" bitsize="64" type="neon_d"/>
    <reg name="d28" bitsize="64" type="neon_d"/>
    <reg name="d29" bitsize="64" type="neon_d"/>
    <reg name="d30" bitsize="64" type="neon_d"/>
    <reg name="d31" bitsize="64" type="neon_d"/>

    <reg name="q0" bitsize="128" type="neon_q" regnum="64"/>
    <reg name="q1" bitsize="128" type="neon_q"/>
    <reg name="q2" bitsize="128" type="neon_q"/>
    <reg name="q3" bitsize="128" type="neon_q"/>
    <reg name="q4" bitsize="128" type="neon_q"/>
    <reg name="q5" bitsize="128" type="neon_q"/>
    <reg name="q6" bitsize="128" type="neon_q"/>
    <reg name="q7" bitsize="128" type="neon_q"/>
    <reg name="q8" bitsize="128" type="neon_q"/>
    <reg name="q9" bitsize="128" type="neon_q"/>
    <reg name="q10" bitsize="128" type="neon_q"/>
    <reg name="q10" bitsize="128" type="neon_q"/>
    <reg name="q12" bitsize="128" type="neon_q"/>
    <reg name="q13" bitsize="128" type="neon_q"/>
    <reg name="q14" bitsize="128" type="neon_q"/>
    <reg name="q15" bitsize="128" type="neon_q"/>

    <reg name="fpscr" bitsize="32" type="int" group="float" regnum="80"/>
  </feature>
</target>)";

    return target_xml;
}

std::string GDBStubA32::RegRead(const Kernel::KThread* thread, size_t id) const {
    if (!thread) {
        return "";
    }

    const auto& context{thread->GetContext32()};
    const auto& gprs{context.cpu_registers};
    const auto& fprs{context.extension_registers};

    if (id <= PC_REGISTER) {
        return ValueToHex(gprs[id]);
    } else if (id == CPSR_REGISTER) {
        return ValueToHex(context.cpsr);
    } else if (id >= D0_REGISTER && id < Q0_REGISTER) {
        const u64 dN{GetSIMDRegister<u64>(fprs, id - D0_REGISTER)};
        return ValueToHex(dN);
    } else if (id >= Q0_REGISTER && id < FPSCR_REGISTER) {
        const u128 qN{GetSIMDRegister<u128>(fprs, id - Q0_REGISTER)};
        return ValueToHex(qN);
    } else if (id == FPSCR_REGISTER) {
        return ValueToHex(context.fpscr);
    } else {
        return "";
    }
}

void GDBStubA32::RegWrite(Kernel::KThread* thread, size_t id, std::string_view value) const {
    if (!thread) {
        return;
    }

    auto& context{thread->GetContext32()};
    auto& fprs{context.extension_registers};

    if (id <= PC_REGISTER) {
        context.cpu_registers[id] = HexToValue<u32>(value);
    } else if (id == CPSR_REGISTER) {
        context.cpsr = HexToValue<u32>(value);
    } else if (id >= D0_REGISTER && id < Q0_REGISTER) {
        PutSIMDRegister(fprs, id - D0_REGISTER, HexToValue<u64>(value));
    } else if (id >= Q0_REGISTER && id < FPSCR_REGISTER) {
        PutSIMDRegister(fprs, id - Q0_REGISTER, HexToValue<u128>(value));
    } else if (id == FPSCR_REGISTER) {
        context.fpscr = HexToValue<u32>(value);
    }
}

std::string GDBStubA32::ReadRegisters(const Kernel::KThread* thread) const {
    std::string output;

    for (size_t reg = 0; reg <= FPSCR_REGISTER; reg++) {
        const bool gpr{reg <= PC_REGISTER};
        const bool dfpr{reg >= D0_REGISTER && reg < Q0_REGISTER};
        const bool qfpr{reg >= Q0_REGISTER && reg < FPSCR_REGISTER};

        if (!(gpr || dfpr || qfpr || reg == CPSR_REGISTER || reg == FPSCR_REGISTER)) {
            continue;
        }

        output += RegRead(thread, reg);
    }

    return output;
}

void GDBStubA32::WriteRegisters(Kernel::KThread* thread, std::string_view register_data) const {
    for (size_t i = 0, reg = 0; reg <= FPSCR_REGISTER; reg++) {
        const bool gpr{reg <= PC_REGISTER};
        const bool dfpr{reg >= D0_REGISTER && reg < Q0_REGISTER};
        const bool qfpr{reg >= Q0_REGISTER && reg < FPSCR_REGISTER};

        if (gpr || reg == CPSR_REGISTER || reg == FPSCR_REGISTER) {
            RegWrite(thread, reg, register_data.substr(i, 8));
            i += 8;
        } else if (dfpr) {
            RegWrite(thread, reg, register_data.substr(i, 16));
            i += 16;
        } else if (qfpr) {
            RegWrite(thread, reg, register_data.substr(i, 32));
            i += 32;
        }

        if (reg == PC_REGISTER) {
            reg = CPSR_REGISTER - 1;
        } else if (reg == CPSR_REGISTER) {
            reg = D0_REGISTER - 1;
        }
    }
}

std::string GDBStubA32::ThreadStatus(const Kernel::KThread* thread, u8 signal) const {
    return fmt::format("T{:02x}{:02x}:{};{:02x}:{};{:02x}:{};thread:{:x};", signal, PC_REGISTER,
                       RegRead(thread, PC_REGISTER), SP_REGISTER, RegRead(thread, SP_REGISTER),
                       LR_REGISTER, RegRead(thread, LR_REGISTER), thread->GetThreadID());
}

u32 GDBStubA32::BreakpointInstruction() const {
    // A32: trap
    // T32: trap + b #4
    return 0xe7ffdefe;
}

} // namespace Core
