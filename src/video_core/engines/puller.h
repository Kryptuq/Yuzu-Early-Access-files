// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <cstddef>
#include <vector>
#include "common/bit_field.h"
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "video_core/engines/engine_interface.h"

namespace Core {
class System;
}

namespace Tegra {
class MemoryManager;
class DmaPusher;

enum class EngineID {
    FERMI_TWOD_A = 0x902D, // 2D Engine
    MAXWELL_B = 0xB197,    // 3D Engine
    KEPLER_COMPUTE_B = 0xB1C0,
    KEPLER_INLINE_TO_MEMORY_B = 0xA140,
    MAXWELL_DMA_COPY_A = 0xB0B5,
};

namespace Control {
struct ChannelState;
}
} // namespace Tegra

namespace VideoCore {
class RasterizerInterface;
}

namespace Tegra::Engines {

class Puller final {
public:
    struct MethodCall {
        u32 method{};
        u32 argument{};
        u32 subchannel{};
        u32 method_count{};

        explicit MethodCall(u32 method_, u32 argument_, u32 subchannel_ = 0, u32 method_count_ = 0)
            : method(method_), argument(argument_), subchannel(subchannel_),
              method_count(method_count_) {}

        [[nodiscard]] bool IsLastCall() const {
            return method_count <= 1;
        }
    };

    enum class FenceOperation : u32 {
        Acquire = 0,
        Increment = 1,
    };

    union FenceAction {
        u32 raw;
        BitField<0, 1, FenceOperation> op;
        BitField<8, 24, u32> syncpoint_id;
    };

    explicit Puller(GPU& gpu_, MemoryManager& memory_manager_, DmaPusher& dma_pusher,
                    Control::ChannelState& channel_state);
    ~Puller();

    void CallMethod(const MethodCall& method_call);

    void CallMultiMethod(u32 method, u32 subchannel, const u32* base_start, u32 amount,
                         u32 methods_pending);

    void BindRasterizer(VideoCore::RasterizerInterface* rasterizer);

    void CallPullerMethod(const MethodCall& method_call);

    void CallEngineMethod(const MethodCall& method_call);

    void CallEngineMultiMethod(u32 method, u32 subchannel, const u32* base_start, u32 amount,
                               u32 methods_pending);

private:
    Tegra::GPU& gpu;

    MemoryManager& memory_manager;
    DmaPusher& dma_pusher;
    Control::ChannelState& channel_state;
    VideoCore::RasterizerInterface* rasterizer = nullptr;

    static constexpr std::size_t NUM_REGS = 0x800;
    struct Regs {
        static constexpr size_t NUM_REGS = 0x40;

        union {
            struct {
                INSERT_PADDING_WORDS_NOINIT(0x4);
                struct {
                    u32 address_high;
                    u32 address_low;

                    [[nodiscard]] GPUVAddr SemaphoreAddress() const {
                        return static_cast<GPUVAddr>((static_cast<GPUVAddr>(address_high) << 32) |
                                                     address_low);
                    }
                } semaphore_address;

                u32 semaphore_sequence;
                u32 semaphore_trigger;
                INSERT_PADDING_WORDS_NOINIT(0xC);

                // The pusher and the puller share the reference counter, the pusher only has read
                // access
                u32 reference_count;
                INSERT_PADDING_WORDS_NOINIT(0x5);

                u32 semaphore_acquire;
                u32 semaphore_release;
                u32 fence_value;
                FenceAction fence_action;
                INSERT_PADDING_WORDS_NOINIT(0xE2);

                // Puller state
                u32 acquire_mode;
                u32 acquire_source;
                u32 acquire_active;
                u32 acquire_timeout;
                u32 acquire_value;
            };
            std::array<u32, NUM_REGS> reg_array;
        };
    } regs{};

    void ProcessBindMethod(const MethodCall& method_call);
    void ProcessFenceActionMethod();
    void ProcessSemaphoreAcquire();
    void ProcessSemaphoreRelease();
    void ProcessSemaphoreTriggerMethod();
    [[nodiscard]] bool ExecuteMethodOnEngine(u32 method);

    /// Mapping of command subchannels to their bound engine ids
    std::array<EngineID, 8> bound_engines{};

    enum class GpuSemaphoreOperation {
        AcquireEqual = 0x1,
        WriteLong = 0x2,
        AcquireGequal = 0x4,
        AcquireMask = 0x8,
    };

#define ASSERT_REG_POSITION(field_name, position)                                                  \
    static_assert(offsetof(Regs, field_name) == position * 4,                                      \
                  "Field " #field_name " has invalid position")

    ASSERT_REG_POSITION(semaphore_address, 0x4);
    ASSERT_REG_POSITION(semaphore_sequence, 0x6);
    ASSERT_REG_POSITION(semaphore_trigger, 0x7);
    ASSERT_REG_POSITION(reference_count, 0x14);
    ASSERT_REG_POSITION(semaphore_acquire, 0x1A);
    ASSERT_REG_POSITION(semaphore_release, 0x1B);
    ASSERT_REG_POSITION(fence_value, 0x1C);
    ASSERT_REG_POSITION(fence_action, 0x1D);

    ASSERT_REG_POSITION(acquire_mode, 0x100);
    ASSERT_REG_POSITION(acquire_source, 0x101);
    ASSERT_REG_POSITION(acquire_active, 0x102);
    ASSERT_REG_POSITION(acquire_timeout, 0x103);
    ASSERT_REG_POSITION(acquire_value, 0x104);

#undef ASSERT_REG_POSITION
};

} // namespace Tegra::Engines
