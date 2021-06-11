
// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "audio_core/memory_pool.h"
#include "common/logging/log.h"

namespace AudioCore {

ServerMemoryPoolInfo::ServerMemoryPoolInfo() = default;
ServerMemoryPoolInfo::~ServerMemoryPoolInfo() = default;

bool ServerMemoryPoolInfo::Update(const InParams& in_params, OutParams& out_params) {
    // Our state does not need to be changed
    if (in_params.state != State::RequestAttach && in_params.state != State::RequestDetach) {
        return true;
    }

    // Address or size is null
    if (in_params.address == 0 || in_params.size == 0) {
        LOG_ERROR(Audio, "Memory pool address or size is zero! address={:X}, size={:X}",
                  in_params.address, in_params.size);
        return false;
    }

    // Address or size is not aligned
    if ((in_params.address % 0x1000) != 0 || (in_params.size % 0x1000) != 0) {
        LOG_ERROR(Audio, "Memory pool address or size is not aligned! address={:X}, size={:X}",
                  in_params.address, in_params.size);
        return false;
    }

    if (in_params.state == State::RequestAttach) {
        cpu_address = in_params.address;
        size = in_params.size;
        used = true;
        out_params.state = State::Attached;
    } else {
        // Unexpected address
        if (cpu_address != in_params.address) {
            LOG_ERROR(Audio, "Memory pool address differs! Expecting {:X} but address is {:X}",
                      cpu_address, in_params.address);
            return false;
        }

        if (size != in_params.size) {
            LOG_ERROR(Audio, "Memory pool size differs! Expecting {:X} but size is {:X}", size,
                      in_params.size);
            return false;
        }

        cpu_address = 0;
        size = 0;
        used = false;
        out_params.state = State::Detached;
    }
    return true;
}

} // namespace AudioCore
