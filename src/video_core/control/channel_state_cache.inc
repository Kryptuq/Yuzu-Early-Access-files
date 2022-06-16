
#include <algorithm>

#include "video_core/control/channel_state.h"
#include "video_core/control/channel_state_cache.h"
#include "video_core/engines/kepler_compute.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/memory_manager.h"

namespace VideoCommon {

template <class P>
ChannelSetupCaches<P>::~ChannelSetupCaches() = default;

template <class P>
void ChannelSetupCaches<P>::CreateChannel(struct Tegra::Control::ChannelState& channel) {
    std::unique_lock<std::mutex> lk(config_mutex);
    ASSERT(channel_map.find(channel.bind_id) == channel_map.end() && channel.bind_id >= 0);
    auto new_id = [this, &channel]() {
        if (!free_channel_ids.empty()) {
            auto id = free_channel_ids.front();
            free_channel_ids.pop_front();
            new (&channel_storage[id]) P(channel);
            return id;
        }
        channel_storage.emplace_back(channel);
        return channel_storage.size() - 1;
    }();
    channel_map.emplace(channel.bind_id, new_id);
    if (current_channel_id != UNSET_CHANNEL) {
        channel_state = &channel_storage[current_channel_id];
    }
    active_channel_ids.push_back(new_id);
    auto as_it = address_spaces.find(channel.memory_manager->GetID());
    if (as_it != address_spaces.end()) {
        as_it->second.ref_count++;
        return;
    }
    AddresSpaceRef new_gpu_mem_ref{
        .ref_count = 1,
        .storage_id = address_spaces.size(),
        .gpu_memory = channel.memory_manager.get(),
    };
    address_spaces.emplace(channel.memory_manager->GetID(), new_gpu_mem_ref);
    OnGPUASRegister(channel.memory_manager->GetID());
}

/// Bind a channel for execution.
template <class P>
void ChannelSetupCaches<P>::BindToChannel(s32 id) {
    std::unique_lock<std::mutex> lk(config_mutex);
    auto it = channel_map.find(id);
    ASSERT(it != channel_map.end() && id >= 0);
    current_channel_id = it->second;
    channel_state = &channel_storage[current_channel_id];
    maxwell3d = &channel_state->maxwell3d;
    kepler_compute = &channel_state->kepler_compute;
    gpu_memory = &channel_state->gpu_memory;
    current_address_space = gpu_memory->GetID();
}

/// Erase channel's channel_state.
template <class P>
void ChannelSetupCaches<P>::EraseChannel(s32 id) {
    std::unique_lock<std::mutex> lk(config_mutex);
    const auto it = channel_map.find(id);
    ASSERT(it != channel_map.end() && id >= 0);
    const auto this_id = it->second;
    free_channel_ids.push_back(this_id);
    channel_map.erase(it);
    if (this_id == current_channel_id) {
        current_channel_id = UNSET_CHANNEL;
        channel_state = nullptr;
        maxwell3d = nullptr;
        kepler_compute = nullptr;
        gpu_memory = nullptr;
    } else if (current_channel_id != UNSET_CHANNEL) {
        channel_state = &channel_storage[current_channel_id];
    }
    active_channel_ids.erase(
        std::find(active_channel_ids.begin(), active_channel_ids.end(), this_id));
}

} // namespace VideoCommon
