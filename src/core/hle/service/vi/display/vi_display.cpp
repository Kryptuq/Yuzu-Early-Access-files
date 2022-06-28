// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <utility>

#include <fmt/format.h>

#include "common/assert.h"
#include "core/core.h"
#include "core/hle/kernel/k_event.h"
#include "core/hle/kernel/k_readable_event.h"
#include "core/hle/kernel/k_writable_event.h"
#include "core/hle/service/kernel_helpers.h"
#include "core/hle/service/nvdrv/core/container.h"
#include "core/hle/service/nvflinger/buffer_item_consumer.h"
#include "core/hle/service/nvflinger/buffer_queue_consumer.h"
#include "core/hle/service/nvflinger/buffer_queue_core.h"
#include "core/hle/service/nvflinger/buffer_queue_producer.h"
#include "core/hle/service/nvflinger/hos_binder_driver_server.h"
#include "core/hle/service/vi/display/vi_display.h"
#include "core/hle/service/vi/layer/vi_layer.h"

namespace Service::VI {

struct BufferQueue {
    std::shared_ptr<android::BufferQueueCore> core;
    std::unique_ptr<android::BufferQueueProducer> producer;
    std::unique_ptr<android::BufferQueueConsumer> consumer;
};

static BufferQueue CreateBufferQueue(KernelHelpers::ServiceContext& service_context,
                                     Service::Nvidia::NvCore::NvMap& nvmap) {
    auto buffer_queue_core = std::make_shared<android::BufferQueueCore>();
    return {
        buffer_queue_core,
        std::make_unique<android::BufferQueueProducer>(service_context, buffer_queue_core, nvmap),
        std::make_unique<android::BufferQueueConsumer>(buffer_queue_core, nvmap)};
}

Display::Display(u64 id, std::string name_,
                 NVFlinger::HosBinderDriverServer& hos_binder_driver_server_,
                 KernelHelpers::ServiceContext& service_context_, Core::System& system_)
    : display_id{id}, name{std::move(name_)}, hos_binder_driver_server{hos_binder_driver_server_},
      service_context{service_context_} {
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

void Display::CreateLayer(u64 layer_id, u32 binder_id,
                          Service::Nvidia::NvCore::Container& nv_core) {
    ASSERT_MSG(layers.empty(), "Only one layer is supported per display at the moment");

    auto [core, producer, consumer] = CreateBufferQueue(service_context, nv_core.GetNvMapFile());

    auto buffer_item_consumer = std::make_shared<android::BufferItemConsumer>(std::move(consumer));
    buffer_item_consumer->Connect(false);

    layers.emplace_back(std::make_unique<Layer>(layer_id, binder_id, *core, *producer,
                                                std::move(buffer_item_consumer)));

    hos_binder_driver_server.RegisterProducer(std::move(producer));
}

void Display::CloseLayer(u64 layer_id) {
    std::erase_if(layers,
                  [layer_id](const auto& layer) { return layer->GetLayerId() == layer_id; });
}

Layer* Display::FindLayer(u64 layer_id) {
    const auto itr =
        std::find_if(layers.begin(), layers.end(), [layer_id](const std::unique_ptr<Layer>& layer) {
            return layer->GetLayerId() == layer_id;
        });

    if (itr == layers.end()) {
        return nullptr;
    }

    return itr->get();
}

const Layer* Display::FindLayer(u64 layer_id) const {
    const auto itr =
        std::find_if(layers.begin(), layers.end(), [layer_id](const std::unique_ptr<Layer>& layer) {
            return layer->GetLayerId() == layer_id;
        });

    if (itr == layers.end()) {
        return nullptr;
    }

    return itr->get();
}

} // namespace Service::VI
