// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <fmt/format.h>
#include "common/logging/log.h"
#include "core/arm/dynarmic/arm_dynarmic_32.h"
#include "core/arm/dynarmic/arm_dynarmic_cp15.h"
#include "core/core.h"
#include "core/core_timing.h"

using Callback = Dynarmic::A32::Coprocessor::Callback;
using CallbackOrAccessOneWord = Dynarmic::A32::Coprocessor::CallbackOrAccessOneWord;
using CallbackOrAccessTwoWords = Dynarmic::A32::Coprocessor::CallbackOrAccessTwoWords;

template <>
struct fmt::formatter<Dynarmic::A32::CoprocReg> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }
    template <typename FormatContext>
    auto format(const Dynarmic::A32::CoprocReg& reg, FormatContext& ctx) {
        return fmt::format_to(ctx.out(), "cp{}", static_cast<size_t>(reg));
    }
};

namespace Core {

static u32 dummy_value;

std::optional<Callback> DynarmicCP15::CompileInternalOperation(bool two, unsigned opc1,
                                                               CoprocReg CRd, CoprocReg CRn,
                                                               CoprocReg CRm, unsigned opc2) {
    LOG_CRITICAL(Core_ARM, "CP15: cdp{} p15, {}, {}, {}, {}, {}", two ? "2" : "", opc1, CRd, CRn,
                 CRm, opc2);
    return std::nullopt;
}

CallbackOrAccessOneWord DynarmicCP15::CompileSendOneWord(bool two, unsigned opc1, CoprocReg CRn,
                                                         CoprocReg CRm, unsigned opc2) {
    if (!two && CRn == CoprocReg::C7 && opc1 == 0 && CRm == CoprocReg::C5 && opc2 == 4) {
        // CP15_FLUSH_PREFETCH_BUFFER
        // This is a dummy write, we ignore the value written here.
        return &dummy_value;
    }

    if (!two && CRn == CoprocReg::C7 && opc1 == 0 && CRm == CoprocReg::C10) {
        switch (opc2) {
        case 4:
            // CP15_DATA_SYNC_BARRIER
            // This is a dummy write, we ignore the value written here.
            return &dummy_value;
        case 5:
            // CP15_DATA_MEMORY_BARRIER
            // This is a dummy write, we ignore the value written here.
            return &dummy_value;
        }
    }

    if (!two && CRn == CoprocReg::C13 && opc1 == 0 && CRm == CoprocReg::C0 && opc2 == 2) {
        // CP15_THREAD_UPRW
        return &uprw;
    }

    LOG_CRITICAL(Core_ARM, "CP15: mcr{} p15, {}, <Rt>, {}, {}, {}", two ? "2" : "", opc1, CRn, CRm,
                 opc2);
    return {};
}

CallbackOrAccessTwoWords DynarmicCP15::CompileSendTwoWords(bool two, unsigned opc, CoprocReg CRm) {
    LOG_CRITICAL(Core_ARM, "CP15: mcrr{} p15, {}, <Rt>, <Rt2>, {}", two ? "2" : "", opc, CRm);
    return {};
}

CallbackOrAccessOneWord DynarmicCP15::CompileGetOneWord(bool two, unsigned opc1, CoprocReg CRn,
                                                        CoprocReg CRm, unsigned opc2) {
    if (!two && CRn == CoprocReg::C13 && opc1 == 0 && CRm == CoprocReg::C0) {
        switch (opc2) {
        case 2:
            // CP15_THREAD_UPRW
            return &uprw;
        case 3:
            // CP15_THREAD_URO
            return &uro;
        }
    }

    LOG_CRITICAL(Core_ARM, "CP15: mrc{} p15, {}, <Rt>, {}, {}, {}", two ? "2" : "", opc1, CRn, CRm,
                 opc2);
    return {};
}

CallbackOrAccessTwoWords DynarmicCP15::CompileGetTwoWords(bool two, unsigned opc, CoprocReg CRm) {
    if (!two && opc == 0 && CRm == CoprocReg::C14) {
        // CNTPCT
        const auto callback = [](Dynarmic::A32::Jit*, void* arg, u32, u32) -> u64 {
            const auto& parent_arg = *static_cast<ARM_Dynarmic_32*>(arg);
            return parent_arg.system.CoreTiming().GetClockTicks();
        };
        return Callback{callback, &parent};
    }

    LOG_CRITICAL(Core_ARM, "CP15: mrrc{} p15, {}, <Rt>, <Rt2>, {}", two ? "2" : "", opc, CRm);
    return {};
}

std::optional<Callback> DynarmicCP15::CompileLoadWords(bool two, bool long_transfer, CoprocReg CRd,
                                                       std::optional<u8> option) {
    if (option) {
        LOG_CRITICAL(Core_ARM, "CP15: mrrc{}{} p15, {}, [...], {}", two ? "2" : "",
                     long_transfer ? "l" : "", CRd, *option);
    } else {
        LOG_CRITICAL(Core_ARM, "CP15: mrrc{}{} p15, {}, [...]", two ? "2" : "",
                     long_transfer ? "l" : "", CRd);
    }
    return std::nullopt;
}

std::optional<Callback> DynarmicCP15::CompileStoreWords(bool two, bool long_transfer, CoprocReg CRd,
                                                        std::optional<u8> option) {
    if (option) {
        LOG_CRITICAL(Core_ARM, "CP15: mrrc{}{} p15, {}, [...], {}", two ? "2" : "",
                     long_transfer ? "l" : "", CRd, *option);
    } else {
        LOG_CRITICAL(Core_ARM, "CP15: mrrc{}{} p15, {}, [...]", two ? "2" : "",
                     long_transfer ? "l" : "", CRd);
    }
    return std::nullopt;
}

} // namespace Core
