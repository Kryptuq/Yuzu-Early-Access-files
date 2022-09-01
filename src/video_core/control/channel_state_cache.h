// SPDX-FileCopyrightText: 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <deque>
#include <limits>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

#include "common/common_types.h"

namespace Tegra {

namespace Engines {
class Maxwell3D;
class KeplerCompute;
} // namespace Engines

class MemoryManager;

namespace Control {
struct ChannelState;
}

} // namespace Tegra

namespace VideoCommon {

class ChannelInfo {
public:
    ChannelInfo() = delete;
    explicit ChannelInfo(Tegra::Control::ChannelState& state);
    ChannelInfo(const ChannelInfo& state) = delete;
    ChannelInfo& operator=(const ChannelInfo&) = delete;
    ChannelInfo(ChannelInfo&& other) = default;
    ChannelInfo& operator=(ChannelInfo&& other) = default;

    Tegra::Engines::Maxwell3D& maxwell3d;
    Tegra::Engines::KeplerCompute& kepler_compute;
    Tegra::MemoryManager& gpu_memory;
};

template <class P>
class ChannelSetupCaches {
public:
    /// Operations for seting the channel of execution.
    virtual ~ChannelSetupCaches();

    /// Create channel state.
    virtual void CreateChannel(Tegra::Control::ChannelState& channel);

    /// Bind a channel for execution.
    void BindToChannel(s32 id);

    /// Erase channel's state.
    void EraseChannel(s32 id);

    Tegra::MemoryManager* GetFromID(size_t id) const {
        std::unique_lock<std::mutex> lk(config_mutex);
        const auto ref = address_spaces.find(id);
        return ref->second.gpu_memory;
    }

    std::optional<size_t> getStorageID(size_t id) const {
        std::unique_lock<std::mutex> lk(config_mutex);
        const auto ref = address_spaces.find(id);
        if (ref == address_spaces.end()) {
            return std::nullopt;
        }
        return ref->second.storage_id;
    }

protected:
    static constexpr size_t UNSET_CHANNEL{std::numeric_limits<size_t>::max()};

    P* channel_state;
    size_t current_channel_id{UNSET_CHANNEL};
    size_t current_address_space{};
    Tegra::Engines::Maxwell3D* maxwell3d;
    Tegra::Engines::KeplerCompute* kepler_compute;
    Tegra::MemoryManager* gpu_memory;

    std::deque<P> channel_storage;
    std::deque<size_t> free_channel_ids;
    std::unordered_map<s32, size_t> channel_map;
    std::vector<size_t> active_channel_ids;
    struct AddresSpaceRef {
        size_t ref_count;
        size_t storage_id;
        Tegra::MemoryManager* gpu_memory;
    };
    std::unordered_map<size_t, AddresSpaceRef> address_spaces;
    mutable std::mutex config_mutex;

    virtual void OnGPUASRegister([[maybe_unused]] size_t map_id) {}
};

} // namespace VideoCommon
