// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <mutex>
#include <span>
#include <vector>

#include "audio_buffer.h"
#include "audio_core/device/device_session.h"
#include "core/core_timing.h"

namespace AudioCore {

constexpr s32 BufferAppendLimit = 4;

/**
 * A ringbuffer of N audio buffers.
 * The buffer contains 3 sections:
 *     Appended   - Buffers added to the ring, but have yet to be sent to the audio backend.
 *     Registered - Buffers sent to the backend and queued for playback.
 *     Released   - Buffers which have been played, and can now be recycled.
 * Any others are free/untracked.
 *
 * @tparam N - Maximum number of buffers in the ring.
 */
template <size_t N>
class AudioBuffers {
public:
    explicit AudioBuffers(size_t limit) : append_limit{static_cast<u32>(limit)} {}

    /**
     * Append a new audio buffer to the ring.
     *
     * @param buffer - The new buffer.
     */
    void AppendBuffer(AudioBuffer& buffer) {
        std::scoped_lock l{lock};
        buffers[appended_index] = buffer;
        appended_count++;
        appended_index = (appended_index + 1) % append_limit;
    }

    /**
     * Register waiting buffers, up to a maximum of BufferAppendLimit.
     *
     * @param out_buffers - The buffers which were registered.
     */
    void RegisterBuffers(std::vector<AudioBuffer>& out_buffers) {
        std::scoped_lock l{lock};
        const s32 to_register{std::min(std::min(appended_count, BufferAppendLimit),
                                       BufferAppendLimit - registered_count)};

        for (s32 i = 0; i < to_register; i++) {
            s32 index{appended_index - appended_count};
            if (index < 0) {
                index += N;
            }

            out_buffers.push_back(buffers[index]);
            registered_count++;
            registered_index = (registered_index + 1) % append_limit;

            appended_count--;
            if (appended_count == 0) {
                break;
            }
        }
    }

    /**
     * Release a single buffer. Must be already registered.
     *
     * @param index     - The buffer index to release.
     * @param timestamp - The released timestamp for this buffer.
     */
    void ReleaseBuffer(s32 index, s64 timestamp) {
        std::scoped_lock l{lock};
        buffers[index].played_timestamp = timestamp;

        registered_count--;
        released_count++;
        released_index = (released_index + 1) % append_limit;
    }

    /**
     * Release all registered buffers.
     *
     * @param core_timing - The CoreTiming instance
     * @param session     - The device session
     *
     * @return Is the buffer was released.
     */
    bool ReleaseBuffers(Core::Timing::CoreTiming& core_timing, DeviceSession& session) {
        std::scoped_lock l{lock};
        bool buffer_released{false};
        while (registered_count > 0) {
            auto index{registered_index - registered_count};
            if (index < 0) {
                index += N;
            }

            // Check with the backend if this buffer can be released yet.
            if (!session.IsBufferConsumed(buffers[index])) {
                break;
            }

            ReleaseBuffer(index, core_timing.GetGlobalTimeNs().count());
            buffer_released = true;
        }

        return buffer_released || registered_count == 0;
    }

    /**
     * Get all released buffers.
     *
     * @param tags - Container to be filled with the released buffers' tags.
     * @return The number of buffers released.
     */
    u32 GetReleasedBuffers(std::span<u64> tags) {
        std::scoped_lock l{lock};
        u32 released{0};

        while (released_count > 0) {
            auto index{released_index - released_count};
            if (index < 0) {
                index += N;
            }

            auto& buffer{buffers[index]};
            released_count--;

            auto tag{buffer.tag};
            buffer.played_timestamp = 0;
            buffer.samples = 0;
            buffer.tag = 0;
            buffer.size = 0;

            if (tag == 0) {
                break;
            }

            tags[released++] = tag;

            if (released >= tags.size()) {
                break;
            }
        }

        return released;
    }

    /**
     * Get all appended and registered buffers.
     *
     * @param buffers_flushed - Output vector for the buffers which are released.
     * @param max_buffers     - Maximum number of buffers to released.
     * @return The number of buffers released.
     */
    u32 GetRegisteredAppendedBuffers(std::vector<AudioBuffer>& buffers_flushed, u32 max_buffers) {
        std::scoped_lock l{lock};
        if (registered_count + appended_count == 0) {
            return 0;
        }

        size_t buffers_to_flush{
            std::min(static_cast<u32>(registered_count + appended_count), max_buffers)};
        if (buffers_to_flush == 0) {
            return 0;
        }

        while (registered_count > 0) {
            auto index{registered_index - registered_count};
            if (index < 0) {
                index += N;
            }

            buffers_flushed.push_back(buffers[index]);

            registered_count--;
            released_count++;
            released_index = (released_index + 1) % append_limit;

            if (buffers_flushed.size() >= buffers_to_flush) {
                break;
            }
        }

        while (appended_count > 0) {
            auto index{appended_index - appended_count};
            if (index < 0) {
                index += N;
            }

            buffers_flushed.push_back(buffers[index]);

            appended_count--;
            released_count++;
            released_index = (released_index + 1) % append_limit;

            if (buffers_flushed.size() >= buffers_to_flush) {
                break;
            }
        }

        return static_cast<u32>(buffers_flushed.size());
    }

    /**
     * Check if the given tag is in the buffers.
     *
     * @param tag - Unique tag of the buffer to search for.
     * @return True if the buffer is still in the ring, otherwise false.
     */
    bool ContainsBuffer(const u64 tag) const {
        std::scoped_lock l{lock};
        const auto registered_buffers{appended_count + registered_count + released_count};

        if (registered_buffers == 0) {
            return false;
        }

        auto index{released_index - released_count};
        if (index < 0) {
            index += append_limit;
        }

        for (s32 i = 0; i < registered_buffers; i++) {
            if (buffers[index].tag == tag) {
                return true;
            }
            index = (index + 1) % append_limit;
        }

        return false;
    }

    /**
     * Get the number of active buffers in the ring.
     * That is, appended, registered and released buffers.
     *
     * @return Number of active buffers.
     */
    u32 GetAppendedRegisteredCount() const {
        std::scoped_lock l{lock};
        return appended_count + registered_count;
    }

    /**
     * Get the total number of active buffers in the ring.
     * That is, appended, registered and released buffers.
     *
     * @return Number of active buffers.
     */
    u32 GetTotalBufferCount() const {
        std::scoped_lock l{lock};
        return static_cast<u32>(appended_count + registered_count + released_count);
    }

    /**
     * Flush all of the currently appended and registered buffers
     *
     * @param buffers_released - Output count for the number of buffers released.
     * @return True if buffers were successfully flushed, otherwise false.
     */
    bool FlushBuffers(u32& buffers_released) {
        std::scoped_lock l{lock};
        std::vector<AudioBuffer> buffers_flushed{};

        buffers_released = GetRegisteredAppendedBuffers(buffers_flushed, append_limit);

        if (registered_count > 0) {
            return false;
        }

        if (static_cast<u32>(released_count + appended_count) > append_limit) {
            return false;
        }

        return true;
    }

    u64 GetNextTimestamp() const {
        // Iterate backwards through the buffer queue, and take the most recent buffer's end
        std::scoped_lock l{lock};
        auto index{appended_index - 1};
        if (index < 0) {
            index += append_limit;
        }
        return buffers[index].end_timestamp;
    }

private:
    /// Buffer lock
    mutable std::recursive_mutex lock{};
    /// The audio buffers
    std::array<AudioBuffer, N> buffers{};
    /// Current released index
    s32 released_index{};
    /// Number of released buffers
    s32 released_count{};
    /// Current registered index
    s32 registered_index{};
    /// Number of registered buffers
    s32 registered_count{};
    /// Current appended index
    s32 appended_index{};
    /// Number of appended buffers
    s32 appended_count{};
    /// Maximum number of buffers (default 32)
    u32 append_limit{};
};

} // namespace AudioCore
