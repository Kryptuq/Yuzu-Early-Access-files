// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>

#include "audio_core/common/common.h"
#include "audio_core/renderer/behavior/behavior_info.h"
#include "audio_core/renderer/effect/effect_result_state.h"
#include "audio_core/renderer/memory/address_info.h"
#include "audio_core/renderer/memory/pool_mapper.h"
#include "common/common_types.h"

namespace AudioCore::AudioRenderer {
/**
 * Base of all effects. Holds various data and functions used for all derived effects.
 * Should not be used directly.
 */
class EffectInfoBase {
public:
    enum class Type : u8 {
        Invalid,
        Mix,
        Aux,
        Delay,
        Reverb,
        I3dl2Reverb,
        BiquadFilter,
        LightLimiter,
        Capture,
    };

    enum class UsageState {
        Invalid,
        New,
        Enabled,
        Disabled,
    };

    enum class OutStatus : u8 {
        Invalid,
        New,
        Initialized,
        Used,
        Removed,
    };

    enum class ParameterState : u8 {
        Initialized,
        Updating,
        Updated,
    };

    struct InParameterVersion1 {
        /* 0x00 */ Type type;
        /* 0x01 */ bool is_new;
        /* 0x02 */ bool enabled;
        /* 0x04 */ u32 mix_id;
        /* 0x08 */ CpuAddr workbuffer;
        /* 0x10 */ CpuAddr workbuffer_size;
        /* 0x18 */ u32 process_order;
        /* 0x1C */ char unk1C[0x4];
        /* 0x20 */ std::array<u8, 0xA0> specific;
    };
    static_assert(sizeof(InParameterVersion1) == 0xC0,
                  "EffectInfoBase::InParameterVersion1 has the wrong size!");

    struct InParameterVersion2 {
        /* 0x00 */ Type type;
        /* 0x01 */ bool is_new;
        /* 0x02 */ bool enabled;
        /* 0x04 */ u32 mix_id;
        /* 0x08 */ CpuAddr workbuffer;
        /* 0x10 */ CpuAddr workbuffer_size;
        /* 0x18 */ u32 process_order;
        /* 0x1C */ char unk1C[0x4];
        /* 0x20 */ std::array<u8, 0xA0> specific;
    };
    static_assert(sizeof(InParameterVersion2) == 0xC0,
                  "EffectInfoBase::InParameterVersion2 has the wrong size!");

    struct OutStatusVersion1 {
        /* 0x00 */ OutStatus state;
        /* 0x01 */ char unk01[0xF];
    };
    static_assert(sizeof(OutStatusVersion1) == 0x10,
                  "EffectInfoBase::OutStatusVersion1 has the wrong size!");

    struct OutStatusVersion2 {
        /* 0x00 */ OutStatus state;
        /* 0x01 */ char unk01[0xF];
        /* 0x10 */ EffectResultState result_state;
    };
    static_assert(sizeof(OutStatusVersion2) == 0x90,
                  "EffectInfoBase::OutStatusVersion2 has the wrong size!");

    struct State {
        std::array<u8, 0x500> buffer;
    };
    static_assert(sizeof(State) == 0x500, "EffectInfoBase::State has the wrong size!");

    EffectInfoBase() {
        Cleanup();
    }

    virtual ~EffectInfoBase() = default;

    /**
     * Cleanup this effect, resetting it to a starting state.
     */
    void Cleanup() {
        type = Type::Invalid;
        enabled = false;
        mix_id = UnusedMixId;
        process_order = InvalidProcessOrder;
        buffer_unmapped = false;
        parameter = {};
        for (auto& workbuffer : workbuffers) {
            workbuffer.Setup(CpuAddr(0), 0);
        }
    }

    /**
     * Forcibly unmap all assigned workbuffers from the AudioRenderer.
     *
     * @param pool_mapper - Mapper to unmap the buffers.
     */
    void ForceUnmapBuffers(const PoolMapper& pool_mapper) {
        for (auto& workbuffer : workbuffers) {
            if (workbuffer.GetReference(false) != 0) {
                pool_mapper.ForceUnmapPointer(workbuffer);
            }
        }
    }

    /**
     * Check if this effect is enabled.
     *
     * @return True if effect is enabled, otherwise false.
     */
    bool IsEnabled() const {
        return enabled;
    }

    /**
     * Check if this effect should not be generated.
     *
     * @return True if effect should be skipped, otherwise false.
     */
    bool ShouldSkip() const {
        return buffer_unmapped;
    }

    /**
     * Get the type of this effect.
     *
     * @return The type of this effect. See EffectInfoBase::Type
     */
    Type GetType() const {
        return type;
    }

    /**
     * Set the type of this effect.
     *
     * @param type_ - The new type of this effect.
     */
    void SetType(const Type type_) {
        type = type_;
    }

    /**
     * Get the mix id of this effect.
     *
     * @return Mix id of this effect.
     */
    s32 GetMixId() const {
        return mix_id;
    }

    /**
     * Get the processing order of this effect.
     *
     * @return Process order of this effect.
     */
    s32 GetProcessingOrder() const {
        return process_order;
    }

    /**
     * Get this effect's parameter data.
     *
     * @return Pointer to the parametter, must be cast to the correct type.
     */
    u8* GetParameter() {
        return parameter.data();
    }

    /**
     * Get this effect's parameter data.
     *
     * @return Pointer to the parametter, must be cast to the correct type.
     */
    u8* GetStateBuffer() {
        return state.data();
    }

    /**
     * Set this effect's usage state.
     *
     * @param usage - new usage state of this effect.
     */
    void SetUsage(const UsageState usage) {
        usage_state = usage;
    }

    /**
     * Check if this effects need to have its workbuffer information updated.
     * Version 1.
     *
     * @param params - Input parameters.
     * @return True if workbuffers need updating, otherwise false.
     */
    bool ShouldUpdateWorkBufferInfo(const InParameterVersion1& params) const {
        return buffer_unmapped || params.is_new;
    }

    /**
     * Check if this effects need to have its workbuffer information updated.
     * Version 2.
     *
     * @param params - Input parameters.
     * @return True if workbuffers need updating, otherwise false.
     */
    bool ShouldUpdateWorkBufferInfo(const InParameterVersion2& params) const {
        return buffer_unmapped || params.is_new;
    }

    /**
     * Get the current usage state of this effect.
     *
     * @return The current usage state.
     */
    UsageState GetUsage() const {
        return usage_state;
    }

    /**
     * Write the current state. Version 1.
     *
     * @param out_status      - Status to write.
     * @param renderer_active - Is the AudioRenderer active?
     */
    void StoreStatus(OutStatusVersion1& out_status, const bool renderer_active) const {
        if (renderer_active) {
            if (usage_state != UsageState::Disabled) {
                out_status.state = OutStatus::Used;
            } else {
                out_status.state = OutStatus::Removed;
            }
        } else if (usage_state == UsageState::New) {
            out_status.state = OutStatus::Used;
        } else {
            out_status.state = OutStatus::Removed;
        }
    }

    /**
     * Write the current state. Version 2.
     *
     * @param out_status      - Status to write.
     * @param renderer_active - Is the AudioRenderer active?
     */
    void StoreStatus(OutStatusVersion2& out_status, const bool renderer_active) const {
        if (renderer_active) {
            if (usage_state != UsageState::Disabled) {
                out_status.state = OutStatus::Used;
            } else {
                out_status.state = OutStatus::Removed;
            }
        } else if (usage_state == UsageState::New) {
            out_status.state = OutStatus::Used;
        } else {
            out_status.state = OutStatus::Removed;
        }
    }

    /**
     * Update the info with new parameters, version 1.
     *
     * @param error_info  - Used to write call result code.
     * @param in_params   - New parameters to update the info with.
     * @param pool_mapper - Pool for mapping buffers.
     */
    virtual void Update(BehaviorInfo::ErrorInfo& error_info,
                        [[maybe_unused]] const InParameterVersion1& params,
                        [[maybe_unused]] const PoolMapper& pool_mapper) {
        error_info.error_code = ResultSuccess;
        error_info.address = CpuAddr(0);
    }

    /**
     * Update the info with new parameters, version 2.
     *
     * @param error_info  - Used to write call result code.
     * @param in_params   - New parameters to update the info with.
     * @param pool_mapper - Pool for mapping buffers.
     */
    virtual void Update(BehaviorInfo::ErrorInfo& error_info,
                        [[maybe_unused]] const InParameterVersion2& params,
                        [[maybe_unused]] const PoolMapper& pool_mapper) {
        error_info.error_code = ResultSuccess;
        error_info.address = CpuAddr(0);
    }

    /**
     * Update the info after command generation. Usually only changes its state.
     */
    virtual void UpdateForCommandGeneration() {}

    /**
     * Initialize a new result state. Version 2 only, unused.
     *
     * @param result_state - Result state to initialize.
     */
    virtual void InitializeResultState([[maybe_unused]] EffectResultState& result_state) {}

    /**
     * Update the host-side state with the ADSP-side state. Version 2 only, unused.
     *
     * @param cpu_state - Host-side result state to update.
     * @param dsp_state - AudioRenderer-side result state to update from.
     */
    virtual void UpdateResultState([[maybe_unused]] EffectResultState& cpu_state,
                                   [[maybe_unused]] EffectResultState& dsp_state) {}

    /**
     * Get a workbuffer assigned to this effect with the given index.
     *
     * @param index - Workbuffer index.
     * @return Address of the buffer.
     */
    virtual CpuAddr GetWorkbuffer([[maybe_unused]] s32 index) {
        return 0;
    }

    /**
     * Get the first workbuffer assigned to this effect.
     *
     * @param index - Workbuffer index. Unused.
     * @return Address of the buffer.
     */
    CpuAddr GetSingleBuffer([[maybe_unused]] const s32 index) {
        if (enabled) {
            return workbuffers[0].GetReference(true);
        }

        if (usage_state != UsageState::Disabled) {
            const auto ref{workbuffers[0].GetReference(false)};
            const auto size{workbuffers[0].GetSize()};
            if (ref != 0 && size > 0) {
                // Invalidate DSP cache
            }
        }
        return 0;
    }

    /**
     * Get the send buffer info, used by Aux and Capture.
     *
     * @return Address of the buffer info.
     */
    CpuAddr GetSendBufferInfo() const {
        return send_buffer_info;
    }

    /**
     * Get the send buffer, used by Aux and Capture.
     *
     * @return Address of the buffer.
     */
    CpuAddr GetSendBuffer() const {
        return send_buffer;
    }

    /**
     * Get the return buffer info, used by Aux and Capture.
     *
     * @return Address of the buffer info.
     */
    CpuAddr GetReturnBufferInfo() const {
        return return_buffer_info;
    }

    /**
     * Get the return buffer, used by Aux and Capture.
     *
     * @return Address of the buffer.
     */
    CpuAddr GetReturnBuffer() const {
        return return_buffer;
    }

protected:
    /// Type of this effect. May be changed
    Type type{Type::Invalid};
    /// Is this effect enabled?
    bool enabled{};
    /// Are this effect's buffers unmapped?
    bool buffer_unmapped{};
    /// Current usage state
    UsageState usage_state{UsageState::Invalid};
    /// Mix id of this effect
    s32 mix_id{UnusedMixId};
    /// Process order of this effect
    s32 process_order{InvalidProcessOrder};
    /// Workbuffers assigned to this effect
    std::array<AddressInfo, 2> workbuffers{AddressInfo(CpuAddr(0), 0), AddressInfo(CpuAddr(0), 0)};
    /// Aux/Capture buffer info for reading
    CpuAddr send_buffer_info;
    /// Aux/Capture buffer for reading
    CpuAddr send_buffer;
    /// Aux/Capture buffer info for writing
    CpuAddr return_buffer_info;
    /// Aux/Capture buffer for writing
    CpuAddr return_buffer;
    /// Parameters of this effect
    std::array<u8, sizeof(InParameterVersion2)> parameter{};
    /// State of this effect used by the AudioRenderer across calls
    std::array<u8, sizeof(State)> state{};
};

} // namespace AudioCore::AudioRenderer
