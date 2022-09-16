// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <chrono>
#include <memory>
#include <span>

#include "audio_core/common/audio_renderer_parameter.h"
#include "audio_core/renderer/performance/performance_detail.h"
#include "audio_core/renderer/performance/performance_entry.h"
#include "audio_core/renderer/performance/performance_entry_addresses.h"
#include "audio_core/renderer/performance/performance_frame_header.h"
#include "common/common_types.h"

namespace AudioCore::AudioRenderer {
class BehaviorInfo;
class MemoryPoolInfo;

enum class PerformanceVersion {
    Version1,
    Version2,
};

enum class PerformanceSysDetailType {
    PcmInt16 = 15,
    PcmFloat = 16,
    Adpcm = 17,
    LightLimiter = 37,
};

enum class PerformanceState {
    Invalid,
    Start,
    Stop,
};

/**
 * Manages performance information.
 *
 * The performance buffer is split into frames, each comprised of:
 *     Frame header - Information about the number of entries/details and some others
 *     Entries      - Created when starting to generate types of commands, such as voice
 * commands, mix commands, sink commands etc. Details      - Created for specific commands
 * within each group. Up to MaxDetailEntries per frame.
 *
 * A current frame is written to by the AudioRenderer, and before it processes the next command
 * list, the current frame is copied to a ringbuffer of history frames. These frames are then
 * output back to the game if it supplies a performance buffer to RequestUpdate.
 *
 * Two versions currently exist, version 2 adds a few extra fields to the header, and a new
 * SysDetail type which is seemingly unused.
 */
class PerformanceManager {
public:
    static constexpr size_t MaxDetailEntries = 100;

    struct InParameter {
        /* 0x00 */ s32 target_node_id;
        /* 0x04 */ char unk04[0xC];
    };
    static_assert(sizeof(InParameter) == 0x10,
                  "PerformanceManager::InParameter has the wrong size!");

    struct OutStatus {
        /* 0x00 */ s32 history_size;
        /* 0x04 */ char unk04[0xC];
    };
    static_assert(sizeof(OutStatus) == 0x10, "PerformanceManager::OutStatus has the wrong size!");

    /**
     * Calculate the required size for the performance workbuffer.
     *
     * @param behavior - Check which version is supported.
     * @param params   - Input parameters.
     *
     * @return Required workbuffer size.
     */
    static u64 GetRequiredBufferSizeForPerformanceMetricsPerFrame(
        const BehaviorInfo& behavior, const AudioRendererParameterInternal& params) {
        u64 entry_count{params.voices + params.effects + params.sub_mixes + params.sinks + 1};
        switch (behavior.GetPerformanceMetricsDataFormat()) {
        case 1:
            return sizeof(PerformanceFrameHeaderVersion1) +
                   PerformanceManager::MaxDetailEntries * sizeof(PerformanceDetailVersion1) +
                   entry_count * sizeof(PerformanceEntryVersion1);
        case 2:
            return sizeof(PerformanceFrameHeaderVersion2) +
                   PerformanceManager::MaxDetailEntries * sizeof(PerformanceDetailVersion2) +
                   entry_count * sizeof(PerformanceEntryVersion2);
        }

        LOG_WARNING(Service_Audio, "Invalid PerformanceMetrics version, assuming version 1");
        return sizeof(PerformanceFrameHeaderVersion1) +
               PerformanceManager::MaxDetailEntries * sizeof(PerformanceDetailVersion1) +
               entry_count * sizeof(PerformanceEntryVersion1);
    }

    virtual ~PerformanceManager() = default;

    /**
     * Initialize the performance manager.
     *
     * @param workbuffer      - Workbuffer to use for performance frames.
     * @param workbuffer_size - Size of the workbuffer.
     * @param params          - Input parameters.
     * @param behavior        - Behaviour to check version and data format.
     * @param memory_pool     - Used to translate the workbuffer address for the DSP.
     */
    virtual void Initialize(std::span<u8> workbuffer, u64 workbuffer_size,
                            const AudioRendererParameterInternal& params,
                            const BehaviorInfo& behavior, const MemoryPoolInfo& memory_pool);

    /**
     * Check if the manager is initialized.
     *
     * @return True if initialized, otherwise false.
     */
    virtual bool IsInitialized() const;

    /**
     * Copy the waiting performance frames to the output buffer.
     *
     * @param out_buffer - Output buffer to store performance frames.
     * @param out_size   - Size of the output buffer.
     * @return Size in bytes that were written to the buffer.
     */
    virtual u32 CopyHistories(u8* out_buffer, u64 out_size);

    /**
     * Setup a new sys detail in the current frame, filling in addresses with offsets to the
     * current workbuffer, to be written by the AudioRenderer. Note: This version is
     * unused/incomplete.
     *
     * @param addresses       - Filled with pointers to the new entry, which should be passed to
     * the AudioRenderer with Performance commands to be written.
     * @param unk             - Unknown.
     * @param sys_detail_type - Sys detail type.
     * @param node_id         - Node id for this entry.
     * @return True if a new entry was created and the offsets are valid, otherwise false.
     */
    virtual bool GetNextEntry(PerformanceEntryAddresses& addresses, u32** unk,
                              PerformanceSysDetailType sys_detail_type, s32 node_id);

    /**
     * Setup a new entry in the current frame, filling in addresses with offsets to the current
     * workbuffer, to be written by the AudioRenderer.
     *
     * @param addresses       - Filled with pointers to the new entry, which should be passed to
     * the AudioRenderer with Performance commands to be written.
     * @param entry_type      - The type of this entry. See PerformanceEntryType
     * @param node_id         - Node id for this entry.
     * @return True if a new entry was created and the offsets are valid, otherwise false.
     */
    virtual bool GetNextEntry(PerformanceEntryAddresses& addresses, PerformanceEntryType entry_type,
                              s32 node_id);

    /**
     * Setup a new detail in the current frame, filling in addresses with offsets to the current
     * workbuffer, to be written by the AudioRenderer.
     *
     * @param addresses       - Filled with pointers to the new detail, which should be passed
     *                          to the AudioRenderer with Performance commands to be written.
     * @param detail_type     - Performance detail type.
     * @param entry_type      - The type of this detail. See PerformanceEntryType
     * @param node_id         - Node id for this detail.
     * @return True if a new detail was created and the offsets are valid, otherwise false.
     */
    virtual bool GetNextEntry(PerformanceEntryAddresses& addresses,
                              PerformanceDetailType detail_type, PerformanceEntryType entry_type,
                              s32 node_id);

    /**
     * Save the current frame to the ring buffer.
     *
     * @param dsp_behind           - Did the AudioRenderer fall behind and not
     *                               finish processing the command list?
     * @param voices_dropped       - The number of voices that were dropped.
     * @param rendering_start_tick - The tick rendering started.
     */
    virtual void TapFrame(bool dsp_behind, u32 voices_dropped, u64 rendering_start_tick);

    /**
     * Check if the node id is a detail type.
     *
     * @return True if the node is a detail type, otherwise false.
     */
    virtual bool IsDetailTarget(u32 target_node_id) const;

    /**
     * Set the given node to be a detail type.
     *
     * @param target_node_id - Node to set.
     */
    virtual void SetDetailTarget(u32 target_node_id);

private:
    /**
     * Create the performance manager.
     *
     * @param version - Performance version to create.
     */
    void CreateImpl(size_t version);

    std::unique_ptr<PerformanceManager>
        /// Impl for the performance manager, may be version 1 or 2.
        impl;
};

template <PerformanceVersion Version, typename FrameHeaderVersion, typename EntryVersion,
          typename DetailVersion>
class PerformanceManagerImpl : public PerformanceManager {
public:
    void Initialize(std::span<u8> workbuffer, u64 workbuffer_size,
                    const AudioRendererParameterInternal& params, const BehaviorInfo& behavior,
                    const MemoryPoolInfo& memory_pool) override;
    bool IsInitialized() const override;
    u32 CopyHistories(u8* out_buffer, u64 out_size) override;
    bool GetNextEntry(PerformanceEntryAddresses& addresses, u32** unk,
                      PerformanceSysDetailType sys_detail_type, s32 node_id) override;
    bool GetNextEntry(PerformanceEntryAddresses& addresses, PerformanceEntryType entry_type,
                      s32 node_id) override;
    bool GetNextEntry(PerformanceEntryAddresses& addresses, PerformanceDetailType detail_type,
                      PerformanceEntryType entry_type, s32 node_id) override;
    void TapFrame(bool dsp_behind, u32 voices_dropped, u64 rendering_start_tick) override;
    bool IsDetailTarget(u32 target_node_id) const override;
    void SetDetailTarget(u32 target_node_id) override;

private:
    /// Workbuffer used to store the current performance frame
    std::span<u8> workbuffer{};
    /// DSP address of the workbuffer, used by the AudioRenderer
    CpuAddr translated_buffer{};
    /// Current frame index
    u32 history_frame_index{};
    /// Current frame header
    FrameHeaderVersion* frame_header{};
    /// Current frame entry buffer
    std::span<EntryVersion> entry_buffer{};
    /// Current frame detail buffer
    std::span<DetailVersion> detail_buffer{};
    /// Current frame entry count
    u32 entry_count{};
    /// Current frame detail count
    u32 detail_count{};
    /// Ringbuffer of previous frames
    std::span<u8> frame_history{};
    /// Current history frame header
    FrameHeaderVersion* frame_history_header{};
    /// Current history entry buffer
    std::span<EntryVersion> frame_history_entries{};
    /// Current history detail buffer
    std::span<DetailVersion> frame_history_details{};
    /// Current history ringbuffer write index
    u32 output_frame_index{};
    /// Last history frame index that was written back to the game
    u32 last_output_frame_index{};
    /// Maximum number of history frames in the ringbuffer
    u32 max_frames{};
    /// Number of entries per frame
    u32 entries_per_frame{};
    /// Maximum number of details per frame
    u32 max_detail_count{};
    /// Frame size in bytes
    u64 frame_size{};
    /// Is the performance manager initialized?
    bool is_initialized{};
    /// Target node id
    u32 target_node_id{};
    /// Performance version in use
    PerformanceVersion version{};
};

} // namespace AudioCore::AudioRenderer
