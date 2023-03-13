// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "audio_core/renderer/splitter/splitter_destinations_data.h"
#include "common/common_types.h"

namespace AudioCore::AudioRenderer {
/**
 * Represents a splitter, wraps multiple output destinations to split an input mix into.
 */
class SplitterInfo {
public:
    struct InParameter {
        /* 0x00 */ u32 magic; // 'SNDI'
        /* 0x04 */ s32 id;
        /* 0x08 */ u32 sample_rate;
        /* 0x0C */ u32 destination_count;
    };
    static_assert(sizeof(InParameter) == 0x10, "SplitterInfo::InParameter has the wrong size!");

    explicit SplitterInfo(s32 id);

    /**
     * Initialize the given splitters.
     *
     * @param splitters - Splitters to initialize.
     * @param count     - Number of splitters given.
     */
    static void InitializeInfos(SplitterInfo* splitters, u32 count);

    /**
     * Update this splitter.
     *
     * @param params - Input parameters to update with.
     * @return The size in bytes of this splitter.
     */
    u32 Update(const InParameter* params);

    /**
     * Get a destination in this splitter.
     *
     * @param id - Destination id to get.
     * @return Pointer to the destination, may be nullptr.
     */
    SplitterDestinationData* GetData(u32 id);

    /**
     * Get the number of destinations in this splitter.
     *
     * @return The number of destinations.
     */
    u32 GetDestinationCount() const;

    /**
     * Set the number of destinations in this splitter.
     *
     * @param count - The new number of destinations.
     */
    void SetDestinationCount(u32 count);

    /**
     * Check if the splitter has a new connection.
     *
     * @return True if there is a new connection, otherwise false.
     */
    bool HasNewConnection() const;

    /**
     * Reset the new connection flag.
     */
    void ClearNewConnectionFlag();

    /**
     * Mark as having a new connection.
     */
    void SetNewConnectionFlag();

    /**
     * Update the state of all destinations.
     */
    void UpdateInternalState();

    /**
     * Set this splitter's destinations.
     *
     * @param destinations - The new destination list for this splitter.
     */
    void SetDestinations(SplitterDestinationData* destinations);

private:
    /// Id of this splitter
    s32 id;
    /// Sample rate of this splitter
    u32 sample_rate{};
    /// Number of destinations in this splitter
    u32 destination_count{};
    /// Does this splitter have a new connection?
    bool has_new_connection{true};
    /// Pointer to the destinations of this splitter
    SplitterDestinationData* destinations{};
    /// Number of channels this splitter manages
    u32 channel_count{};
};

} // namespace AudioCore::AudioRenderer
