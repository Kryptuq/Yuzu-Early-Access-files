// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>

#include "audio_core/common/common.h"
#include "audio_core/renderer/behavior/behavior_info.h"
#include "audio_core/renderer/memory/address_info.h"
#include "common/common_types.h"
#include "common/fixed_point.h"

namespace AudioCore::AudioRenderer {
struct UpsamplerInfo;
class PoolMapper;

/**
 * Base for the circular buffer and device sinks, holding their states for the AudioRenderer and
 * their parametetrs for generating sink commands.
 */
class SinkInfoBase {
public:
    enum class Type : u8 {
        Invalid,
        DeviceSink,
        CircularBufferSink,
    };

    struct DeviceInParameter {
        /* 0x000 */ char name[0x100];
        /* 0x100 */ u32 input_count;
        /* 0x104 */ std::array<s8, MaxChannels> inputs;
        /* 0x10A */ char unk10A[0x1];
        /* 0x10B */ bool downmix_enabled;
        /* 0x10C */ std::array<f32, 4> downmix_coeff;
    };
    static_assert(sizeof(DeviceInParameter) == 0x11C, "DeviceInParameter has the wrong size!");

    struct DeviceState {
        /* 0x00 */ UpsamplerInfo* upsampler_info;
        /* 0x08 */ std::array<Common::FixedPoint<16, 16>, 4> downmix_coeff;
        /* 0x18 */ char unk18[0x18];
    };
    static_assert(sizeof(DeviceState) == 0x30, "DeviceState has the wrong size!");

    struct CircularBufferInParameter {
        /* 0x00 */ u64 cpu_address;
        /* 0x08 */ u32 size;
        /* 0x0C */ u32 input_count;
        /* 0x10 */ u32 sample_count;
        /* 0x14 */ u32 previous_pos;
        /* 0x18 */ SampleFormat format;
        /* 0x1C */ std::array<s8, MaxChannels> inputs;
        /* 0x22 */ bool in_use;
        /* 0x23 */ char unk23[0x5];
    };
    static_assert(sizeof(CircularBufferInParameter) == 0x28,
                  "CircularBufferInParameter has the wrong size!");

    struct CircularBufferState {
        /* 0x00 */ u32 last_pos2;
        /* 0x04 */ s32 current_pos;
        /* 0x08 */ u32 last_pos;
        /* 0x0C */ char unk0C[0x4];
        /* 0x10 */ AddressInfo address_info;
    };
    static_assert(sizeof(CircularBufferState) == 0x30, "CircularBufferState has the wrong size!");

    struct InParameter {
        /* 0x000 */ Type type;
        /* 0x001 */ bool in_use;
        /* 0x004 */ u32 node_id;
        /* 0x008 */ char unk08[0x18];
        union {
            /* 0x020 */ DeviceInParameter device;
            /* 0x020 */ CircularBufferInParameter circular_buffer;
        };
    };
    static_assert(sizeof(InParameter) == 0x140, "SinkInfoBase::InParameter has the wrong size!");

    struct OutStatus {
        /* 0x00 */ u32 writeOffset;
        /* 0x04 */ char unk04[0x1C];
    }; // size == 0x20
    static_assert(sizeof(OutStatus) == 0x20, "SinkInfoBase::OutStatus has the wrong size!");

    virtual ~SinkInfoBase() = default;

    /**
     * Clean up for info, resetting it to a default state.
     */
    virtual void CleanUp();

    /**
     * Update the info according to parameters, and write the current state to out_status.
     *
     * @param error_info  - Output error code.
     * @param out_status  - Output status.
     * @param in_params   - Input parameters.
     * @param pool_mapper - Used to map the circular buffer.
     */
    virtual void Update(BehaviorInfo::ErrorInfo& error_info, OutStatus& out_status,
                        [[maybe_unused]] const InParameter& in_params,
                        [[maybe_unused]] const PoolMapper& pool_mapper);

    /**
     * Update the circular buffer on command generation, incrementing its current offsets.
     */
    virtual void UpdateForCommandGeneration();

    /**
     * Get the state as a device sink.
     *
     * @return Device state.
     */
    DeviceState* GetDeviceState();

    /**
     * Get the type of this sink.
     *
     * @return Either Device, Circular, or Invalid.
     */
    Type GetType() const;

    /**
     * Check if this sink is in use.
     *
     * @return True if used, otherwise false.
     */
    bool IsUsed() const;

    /**
     * Check if this sink should be skipped for updates.
     *
     * @return True if it should be skipped, otherwise false.
     */
    bool ShouldSkip() const;

    /**
     * Get the node if of this sink.
     *
     * @return Node id for this sink.
     */
    u32 GetNodeId() const;

    /**
     * Get the state of this sink.
     *
     * @return Pointer to the state, must be cast to the correct type.
     */
    u8* GetState();

    /**
     * Get the parameters of this sink.
     *
     * @return Pointer to the parameters, must be cast to the correct type.
     */
    u8* GetParameter();

protected:
    /// Type of this sink
    Type type{Type::Invalid};
    /// Is this sink in use?
    bool in_use{};
    /// Is this sink's buffer unmapped? Circular only
    bool buffer_unmapped{};
    /// Node id for this sink
    u32 node_id{};
    /// State buffer for this sink
    std::array<u8, std::max(sizeof(DeviceState), sizeof(CircularBufferState))> state{};
    /// Parameter buffer for this sink
    std::array<u8, std::max(sizeof(DeviceInParameter), sizeof(CircularBufferInParameter))>
        parameter{};
};

} // namespace AudioCore::AudioRenderer
