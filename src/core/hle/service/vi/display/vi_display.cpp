// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <utility>

#include <fmt/format.h>

#include "common/assert.h"
#include "core/core.h"
#include "core/hle/kernel/k_event.h"
#include "core/hle/kernel/k_readable_event.h"
#include "core/hle/kernel/k_writable_event.h"
#include "core/hle/service/kernel_helpers.h"
#include "core/hle/service/vi/display/vi_display.h"
#include "core/hle/service/vi/layer/vi_layer.h"

namespace Service::VI {

Display::Display(u64 id, std::string name_, KernelHelpers::ServiceContext& service_context_,
                 Core::System& system_)
    : display_id{id}, name{std::move(name_)}, service_context{service_context_} {
    vsync_event = service_context.CreateEvent(fmt::format("Display VSync Event {}", id));
}

Display::~Display() {
    service_context.CloseEvent(vsync_event);
}

Layer& Display::GetLayer(std::size_t index) {
    return *layers.at(index);
}

const Layer& Display::GetLayer(std::size_t index) const {
    return *layers.at(index);
}

Kernel::KReadableEvent& Display::GetVSyncEvent() {
    return vsync_event->GetReadableEvent();
}

void Display::SignalVSyncEvent() {
    vsync_event->GetWritableEvent().Signal();
}

void Display::CreateLayer(u64 layer_id, NVFlinger::BufferQueue& buffer_queue) {
    // TODO(Subv): Support more than 1 layer.
    ASSERT_MSG(layers.empty(), "Only one layer is supported per display at the moment");

    layers.emplace_back(std::make_shared<Layer>(layer_id, buffer_queue));
}

void Display::CloseLayer(u64 layer_id) {
    std::erase_if(layers, [layer_id](const auto& layer) { return layer->GetID() == layer_id; });
}

Layer* Display::FindLayer(u64 layer_id) {
    const auto itr =
        std::find_if(layers.begin(), layers.end(), [layer_id](const std::shared_ptr<Layer>& layer) {
            return layer->GetID() == layer_id;
        });

    if (itr == layers.end()) {
        return nullptr;
    }

    return itr->get();
}

const Layer* Display::FindLayer(u64 layer_id) const {
    const auto itr =
        std::find_if(layers.begin(), layers.end(), [layer_id](const std::shared_ptr<Layer>& layer) {
            return layer->GetID() == layer_id;
        });

    if (itr == layers.end()) {
        return nullptr;
    }

    return itr->get();
}

} // namespace Service::VI
