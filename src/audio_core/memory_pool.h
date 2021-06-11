// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/swap.h"

namespace AudioCore {

class ServerMemoryPoolInfo {
public:
    ServerMemoryPoolInfo();
    ~ServerMemoryPoolInfo();

    enum class State : u32_le {
        Invalid = 0x0,
        Aquired = 0x1,
        RequestDetach = 0x2,
        Detached = 0x3,
        RequestAttach = 0x4,
        Attached = 0x5,
        Released = 0x6,
    };

    struct InParams {
        u64_le address{};
        u64_le size{};
        State state{};
        INSERT_PADDING_WORDS(3);
    };
    static_assert(sizeof(InParams) == 0x20, "InParams are an invalid size");

    struct OutParams {
        State state{};
        INSERT_PADDING_WORDS(3);
    };
    static_assert(sizeof(OutParams) == 0x10, "OutParams are an invalid size");

    bool Update(const InParams& in_params, OutParams& out_params);

private:
    // There's another entry here which is the DSP address, however since we're not talking to the
    // DSP we can just use the same address provided by the guest without needing to remap
    u64_le cpu_address{};
    u64_le size{};
    bool used{};
};

} // namespace AudioCore
