// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>

#include "audio_core/renderer/splitter/splitter_destinations_data.h"
#include "audio_core/renderer/splitter/splitter_info.h"
#include "common/common_types.h"

namespace AudioCore {
struct AudioRendererParameterInternal;
class WorkbufferAllocator;

namespace AudioRenderer {
class BehaviorInfo;

/**
 * The splitter allows much more control over how sound is mixed together.
 * Previously, one mix can only connect to one other, and you may need
 * more mixes (and duplicate processing) to achieve the same result.
 * With the splitter, many-to-one and one-to-many mixing is possible.
 * This was added in revision 2.
 * Had a bug with incorrect numbers of destinations, fixed in revision 5.
 */
class SplitterContext {
    struct InParameterHeader {
        /* 0x00 */ u32 magic; // 'SNDH'
        /* 0x04 */ s32 info_count;
        /* 0x08 */ s32 destination_count;
        /* 0x0C */ char unk0C[0x14];
    };
    static_assert(sizeof(InParameterHeader) == 0x20,
                  "SplitterContext::InParameterHeader has the wrong size!");

public:
    /**
     * Get a destination mix from the given splitter and destination index.
     *
     * @param splitter_id    - Splitter index to get from.
     * @param destination_id - Destination index within the splitter.
     * @return Pointer to the found destination. May be nullptr.
     */
    SplitterDestinationData* GetDesintationData(s32 splitter_id, s32 destination_id);

    /**
     * Get a splitter from the given index.
     *
     * @param index    - Index of the desired splitter.
     * @return Splitter requested.
     */
    SplitterInfo& GetInfo(s32 index);

    /**
     * Get the total number of splitter destinations.
     *
     * @return Number of destinations.
     */
    u32 GetDataCount() const;

    /**
     * Get the total number of splitters.
     *
     * @return Number of splitters.
     */
    u32 GetInfoCount() const;

    /**
     * Get a specific global destination.
     *
     * @param index - Index of the desired destination.
     * @return The requested destination.
     */
    SplitterDestinationData& GetData(u32 index);

    /**
     * Check if the splitter is in use.
     *
     * @return True if any splitter or destination is in use, otherwise false.
     */
    bool UsingSplitter() const;

    /**
     * Mark all splitters as having new connections.
     */
    void ClearAllNewConnectionFlag();

    /**
     * Initialize the context.
     *
     * @param behavior - Used to check for splitter support.
     * @param params    - Input parameters.
     * @param allocator - Allocator used to allocate workbuffer memory.
     */
    bool Initialize(const BehaviorInfo& behavior, const AudioRendererParameterInternal& params,
                    WorkbufferAllocator& allocator);

    /**
     * Update the context.
     *
     * @param input         - Input buffer with the new info,
     *                        expected to point to a InParameterHeader.
     * @param consumed_size - Output with the number of bytes consumed from input.
     */
    bool Update(const u8* input, u32& consumed_size);

    /**
     * Update the splitters.
     *
     * @param input          - Input buffer with the new info.
     * @param offset         - Current offset within the input buffer,
     *                         input + offset should point to a SplitterInfo::InParameter.
     * @param splitter_count - Number of splitters in the input buffer.
     * @return Number of bytes consumed in input.
     */
    u32 UpdateInfo(const u8* input, u32 offset, u32 splitter_count);

    /**
     * Update the splitters.
     *
     * @param input             - Input buffer with the new info.
     * @param offset            - Current offset within the input buffer,
     *                            input + offset should point to a
     *                            SplitterDestinationData::InParameter.
     * @param destination_count - Number of destinations in the input buffer.
     * @return Number of bytes consumed in input.
     */
    u32 UpdateData(const u8* input, u32 offset, u32 destination_count);

    /**
     * Update the state of all destinations in all splitters.
     */
    void UpdateInternalState();

    /**
     * Replace the given splitter's destinations with new ones.
     *
     * @param out_info    - Splitter to recompose.
     * @param info_header - Input parameters containing new destination ids.
     */
    void RecomposeDestination(SplitterInfo& out_info, const SplitterInfo::InParameter* info_header);

    /**
     * Old calculation for destinations, this is the thing the splitter bug fixes.
     * Left for compatibility, and now min'd with the actual count to not bug.
     *
     * @return Number of splitter destinations.
     */
    u32 GetDestCountPerInfoForCompat() const;

    /**
     * Calculate the size of the required workbuffer for splitters and destinations.
     *
     * @param behavior - Used to check splitter features.
     * @param params    - Input parameters with splitter/destination counts.
     * @return Required buffer size.
     */
    static u64 CalcWorkBufferSize(const BehaviorInfo& behavior,
                                  const AudioRendererParameterInternal& params);

private:
    /**
     * Setup the context.
     *
     * @param splitter_infos        - Workbuffer for splitters.
     * @param splitter_info_count   - Number of splitters in the workbuffer.
     * @param splitter_destinations - Workbuffer for splitter destinations.
     * @param destination_count     - Number of destinations in the workbuffer.
     * @param splitter_bug_fixed    - Is the splitter bug fixed?
     */
    void Setup(std::span<SplitterInfo> splitter_infos, u32 splitter_info_count,
               SplitterDestinationData* splitter_destinations, u32 destination_count,
               bool splitter_bug_fixed);

    /// Workbuffer for splitters
    std::span<SplitterInfo> splitter_infos{};
    /// Number of splitters in buffer
    s32 info_count{};
    /// Workbuffer for destinations
    SplitterDestinationData* splitter_destinations{};
    /// Number of destinations in buffer
    s32 destinations_count{};
    /// Is the splitter bug fixed?
    bool splitter_bug_fixed{};
};

} // namespace AudioRenderer
} // namespace AudioCore
