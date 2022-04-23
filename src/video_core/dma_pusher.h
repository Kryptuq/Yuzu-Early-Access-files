// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <vector>
#include <queue>

#include "common/bit_field.h"
#include "common/common_types.h"
#include "video_core/engines/engine_interface.h"

namespace Core {
class System;
}

namespace Tegra {

class GPU;

enum class SubmissionMode : u32 {
    IncreasingOld = 0,
    Increasing = 1,
    NonIncreasingOld = 2,
    NonIncreasing = 3,
    Inline = 4,
    IncreaseOnce = 5
};

// Note that, traditionally, methods are treated as 4-byte addressable locations, and hence
// their numbers are written down multiplied by 4 in Docs. Here we are not multiply by 4.
// So the values you see in docs might be multiplied by 4.
enum class BufferMethods : u32 {
    BindObject = 0x0,
    Nop = 0x2,
    SemaphoreAddressHigh = 0x4,
    SemaphoreAddressLow = 0x5,
    SemaphoreSequence = 0x6,
    SemaphoreTrigger = 0x7,
    NotifyIntr = 0x8,
    WrcacheFlush = 0x9,
    Unk28 = 0xA,
    UnkCacheFlush = 0xB,
    RefCnt = 0x14,
    SemaphoreAcquire = 0x1A,
    SemaphoreRelease = 0x1B,
    FenceValue = 0x1C,
    FenceAction = 0x1D,
    WaitForInterrupt = 0x1E,
    Unk7c = 0x1F,
    Yield = 0x20,
    NonPullerMethods = 0x40,
};

struct CommandListHeader {
    union {
        u64 raw;
        BitField<0, 40, GPUVAddr> addr;
        BitField<41, 1, u64> is_non_main;
        BitField<42, 21, u64> size;
    };
};
static_assert(sizeof(CommandListHeader) == sizeof(u64), "CommandListHeader is incorrect size");

union CommandHeader {
    u32 argument;
    BitField<0, 13, u32> method;
    BitField<0, 24, u32> method_count_;
    BitField<13, 3, u32> subchannel;
    BitField<16, 13, u32> arg_count;
    BitField<16, 13, u32> method_count;
    BitField<29, 3, SubmissionMode> mode;
};
static_assert(std::is_standard_layout_v<CommandHeader>, "CommandHeader is not standard layout");
static_assert(sizeof(CommandHeader) == sizeof(u32), "CommandHeader has incorrect size!");

inline CommandHeader BuildCommandHeader(BufferMethods method, u32 arg_count, SubmissionMode mode) {
    CommandHeader result{};
    result.method.Assign(static_cast<u32>(method));
    result.arg_count.Assign(arg_count);
    result.mode.Assign(mode);
    return result;
}

struct CommandList final {
    CommandList() = default;
    explicit CommandList(std::size_t size) : command_lists(size) {}
    explicit CommandList(std::vector<CommandHeader>&& prefetch_command_list_)
        : prefetch_command_list{std::move(prefetch_command_list_)} {}

    std::vector<CommandListHeader> command_lists;
    std::vector<CommandHeader> prefetch_command_list;
};

/**
 * The DmaPusher class implements DMA submission to FIFOs, providing an area of memory that the
 * emulated app fills with commands and tells PFIFO to process. The pushbuffers are then assembled
 * into a "command stream" consisting of 32-bit words that make up "commands".
 * See https://envytools.readthedocs.io/en/latest/hw/fifo/dma-pusher.html#fifo-dma-pusher for
 * details on this implementation.
 */
class DmaPusher final {
public:
    explicit DmaPusher(Core::System& system_, GPU& gpu_);
    ~DmaPusher();

    void Push(CommandList&& entries) {
        dma_pushbuffer.push(std::move(entries));
    }

    void DispatchCalls();

    void BindSubchannel(Engines::EngineInterface* engine, u32 subchannel_id) {
        subchannels[subchannel_id] = engine;
    }

private:
    static constexpr u32 non_puller_methods = 0x40;
    static constexpr u32 max_subchannels = 8;
    bool Step();

    void SetState(const CommandHeader& command_header);

    void CallMethod(u32 argument) const;
    void CallMultiMethod(const u32* base_start, u32 num_methods) const;

    std::vector<CommandHeader> command_headers; ///< Buffer for list of commands fetched at once

    std::queue<CommandList> dma_pushbuffer; ///< Queue of command lists to be processed
    std::size_t dma_pushbuffer_subindex{};  ///< Index within a command list within the pushbuffer

    struct DmaState {
        u32 method;            ///< Current method
        u32 subchannel;        ///< Current subchannel
        u32 method_count;      ///< Current method count
        u32 length_pending;    ///< Large NI command length pending
        bool non_incrementing; ///< Current command's NI flag
        bool is_last_call;
    };

    DmaState dma_state{};
    bool dma_increment_once{};

    bool ib_enable{true}; ///< IB mode enabled

    std::array<Engines::EngineInterface*, max_subchannels> subchannels{};

    GPU& gpu;
    Core::System& system;
};

} // namespace Tegra
