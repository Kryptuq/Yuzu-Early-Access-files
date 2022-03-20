// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2021 yuzu Emulator Project

#include <algorithm>
#include <optional>

#include "common/assert.h"
#include "common/logging/log.h"
#include "common/microprofile.h"
#include "common/scope_exit.h"
#include "common/settings.h"
#include "common/thread.h"
#include "core/core.h"
#include "core/core_timing.h"
#include "core/hle/kernel/k_readable_event.h"
#include "core/hle/service/nvdrv/devices/nvdisp_disp0.h"
#include "core/hle/service/nvdrv/nvdrv.h"
#include "core/hle/service/nvflinger/buffer_item_consumer.h"
#include "core/hle/service/nvflinger/buffer_queue_core.h"
#include "core/hle/service/nvflinger/hos_binder_driver_server.h"
#include "core/hle/service/nvflinger/nvflinger.h"
#include "core/hle/service/nvflinger/ui/graphic_buffer.h"
#include "core/hle/service/vi/display/vi_display.h"
#include "core/hle/service/vi/layer/vi_layer.h"
#include "video_core/gpu.h"

namespace Service::NVFlinger {

constexpr auto frame_ns = std::chrono::nanoseconds{1000000000 / 60};

void NVFlinger::SplitVSync(std::stop_token stop_token) {
    system.RegisterHostThread();
    std::string name = "yuzu:VSyncThread";
    MicroProfileOnThreadCreate(name.c_str());

    // Cleanup
    SCOPE_EXIT({ MicroProfileOnThreadExit(); });

    Common::SetCurrentThreadName(name.c_str());
    Common::SetCurrentThreadPriority(Common::ThreadPriority::High);
    s64 delay = 0;
    while (!stop_token.stop_requested()) {
        guard->lock();
        const s64 time_start = system.CoreTiming().GetGlobalTimeNs().count();
        Compose();
        const auto ticks = GetNextTicks();
        const s64 time_end = system.CoreTiming().GetGlobalTimeNs().count();
        const s64 time_passed = time_end - time_start;
        const s64 next_time = std::max<s64>(0, ticks - time_passed - delay);
        guard->unlock();
        if (next_time > 0) {
            std::this_thread::sleep_for(std::chrono::nanoseconds{next_time});
        }
        delay = (system.CoreTiming().GetGlobalTimeNs().count() - time_end) - next_time;
    }
}

NVFlinger::NVFlinger(Core::System& system_, HosBinderDriverServer& hos_binder_driver_server_)
    : system(system_), service_context(system_, "nvflinger"),
      hos_binder_driver_server(hos_binder_driver_server_) {
    displays.emplace_back(0, "Default", hos_binder_driver_server, service_context, system);
    displays.emplace_back(1, "External", hos_binder_driver_server, service_context, system);
    displays.emplace_back(2, "Edid", hos_binder_driver_server, service_context, system);
    displays.emplace_back(3, "Internal", hos_binder_driver_server, service_context, system);
    displays.emplace_back(4, "Null", hos_binder_driver_server, service_context, system);
    guard = std::make_shared<std::mutex>();

    // Schedule the screen composition events
    composition_event = Core::Timing::CreateEvent(
        "ScreenComposition", [this](std::uintptr_t, std::chrono::nanoseconds ns_late) {
            const auto lock_guard = Lock();
            Compose();

            const auto ticks = std::chrono::nanoseconds{GetNextTicks()};
            const auto ticks_delta = ticks - ns_late;
            const auto future_ns = std::max(std::chrono::nanoseconds::zero(), ticks_delta);

            this->system.CoreTiming().ScheduleEvent(future_ns, composition_event);
        });

    if (system.IsMulticore()) {
        vsync_thread = std::jthread([this](std::stop_token token) { SplitVSync(token); });
    } else {
        system.CoreTiming().ScheduleEvent(frame_ns, composition_event);
    }
}

NVFlinger::~NVFlinger() {
    if (!system.IsMulticore()) {
        system.CoreTiming().UnscheduleEvent(composition_event, 0);
    }

    for (auto& display : displays) {
        for (size_t layer = 0; layer < display.GetNumLayers(); ++layer) {
            display.GetLayer(layer).Core().NotifyShutdown();
        }
    }
}

void NVFlinger::SetNVDrvInstance(std::shared_ptr<Nvidia::Module> instance) {
    nvdrv = std::move(instance);
}

std::optional<u64> NVFlinger::OpenDisplay(std::string_view name) {
    const auto lock_guard = Lock();

    LOG_DEBUG(Service, "Opening \"{}\" display", name);

    const auto itr =
        std::find_if(displays.begin(), displays.end(),
                     [&](const VI::Display& display) { return display.GetName() == name; });

    if (itr == displays.end()) {
        return std::nullopt;
    }

    return itr->GetID();
}

std::optional<u64> NVFlinger::CreateLayer(u64 display_id) {
    const auto lock_guard = Lock();
    auto* const display = FindDisplay(display_id);

    if (display == nullptr) {
        return std::nullopt;
    }

    const u64 layer_id = next_layer_id++;
    CreateLayerAtId(*display, layer_id);
    return layer_id;
}

void NVFlinger::CreateLayerAtId(VI::Display& display, u64 layer_id) {
    const auto buffer_id = next_buffer_queue_id++;
    display.CreateLayer(layer_id, buffer_id);
}

void NVFlinger::CloseLayer(u64 layer_id) {
    const auto lock_guard = Lock();

    for (auto& display : displays) {
        display.CloseLayer(layer_id);
    }
}

std::optional<u32> NVFlinger::FindBufferQueueId(u64 display_id, u64 layer_id) {
    const auto lock_guard = Lock();
    const auto* const layer = FindOrCreateLayer(display_id, layer_id);

    if (layer == nullptr) {
        return std::nullopt;
    }

    return layer->GetBinderId();
}

Kernel::KReadableEvent* NVFlinger::FindVsyncEvent(u64 display_id) {
    const auto lock_guard = Lock();
    auto* const display = FindDisplay(display_id);

    if (display == nullptr) {
        return nullptr;
    }

    return &display->GetVSyncEvent();
}

VI::Display* NVFlinger::FindDisplay(u64 display_id) {
    const auto itr =
        std::find_if(displays.begin(), displays.end(),
                     [&](const VI::Display& display) { return display.GetID() == display_id; });

    if (itr == displays.end()) {
        return nullptr;
    }

    return &*itr;
}

const VI::Display* NVFlinger::FindDisplay(u64 display_id) const {
    const auto itr =
        std::find_if(displays.begin(), displays.end(),
                     [&](const VI::Display& display) { return display.GetID() == display_id; });

    if (itr == displays.end()) {
        return nullptr;
    }

    return &*itr;
}

VI::Layer* NVFlinger::FindLayer(u64 display_id, u64 layer_id) {
    auto* const display = FindDisplay(display_id);

    if (display == nullptr) {
        return nullptr;
    }

    return display->FindLayer(layer_id);
}

const VI::Layer* NVFlinger::FindLayer(u64 display_id, u64 layer_id) const {
    const auto* const display = FindDisplay(display_id);

    if (display == nullptr) {
        return nullptr;
    }

    return display->FindLayer(layer_id);
}

VI::Layer* NVFlinger::FindOrCreateLayer(u64 display_id, u64 layer_id) {
    auto* const display = FindDisplay(display_id);

    if (display == nullptr) {
        return nullptr;
    }

    auto* layer = display->FindLayer(layer_id);

    if (layer == nullptr) {
        LOG_DEBUG(Service, "Layer at id {} not found. Trying to create it.", layer_id);
        CreateLayerAtId(*display, layer_id);
        return display->FindLayer(layer_id);
    }

    return layer;
}

void NVFlinger::Compose() {
    for (auto& display : displays) {
        // Trigger vsync for this display at the end of drawing
        SCOPE_EXIT({ display.SignalVSyncEvent(); });

        // Don't do anything for displays without layers.
        if (!display.HasLayers())
            continue;

        // TODO(Subv): Support more than 1 layer.
        VI::Layer& layer = display.GetLayer(0);

        android::BufferItem buffer{};
        const auto status = layer.GetConsumer().AcquireBuffer(&buffer, {}, false);

        if (status != android::Status::NoError) {
            continue;
        }

        const auto& igbp_buffer = *buffer.graphic_buffer;

        if (!system.IsPoweredOn()) {
            return; // We are likely shutting down
        }

        auto& gpu = system.GPU();
        const auto& multi_fence = buffer.fence;
        guard->unlock();
        for (u32 fence_id = 0; fence_id < multi_fence.num_fences; fence_id++) {
            const auto& fence = multi_fence.fences[fence_id];
            gpu.WaitFence(fence.id, fence.value);
        }
        guard->lock();

        MicroProfileFlip();

        // Now send the buffer to the GPU for drawing.
        // TODO(Subv): Support more than just disp0. The display device selection is probably based
        // on which display we're drawing (Default, Internal, External, etc)
        auto nvdisp = nvdrv->GetDevice<Nvidia::Devices::nvdisp_disp0>("/dev/nvdisp_disp0");
        ASSERT(nvdisp);

        Common::Rectangle<int> crop_rect{
            static_cast<int>(buffer.crop.Left()), static_cast<int>(buffer.crop.Top()),
            static_cast<int>(buffer.crop.Right()), static_cast<int>(buffer.crop.Bottom())};

        nvdisp->flip(igbp_buffer.BufferId(), igbp_buffer.Offset(), igbp_buffer.ExternalFormat(),
                     igbp_buffer.Width(), igbp_buffer.Height(), igbp_buffer.Stride(),
                     static_cast<android::BufferTransformFlags>(buffer.transform), crop_rect);

        swap_interval = buffer.swap_interval;

        auto fence = android::Fence::NoFence();
        layer.GetConsumer().ReleaseBuffer(buffer, fence);
    }
}

s64 NVFlinger::GetNextTicks() const {
    static constexpr s64 max_hertz = 120LL;

    const auto& settings = Settings::values;
    const bool unlocked_fps = settings.disable_fps_limit.GetValue();
    const s64 fps_cap = unlocked_fps ? static_cast<s64>(settings.fps_cap.GetValue()) : 1;
    return (1000000000 * (1LL << swap_interval)) / (max_hertz * fps_cap);
}

} // namespace Service::NVFlinger
