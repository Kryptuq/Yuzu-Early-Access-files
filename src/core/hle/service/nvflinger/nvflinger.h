// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright 2021 yuzu Emulator Project

#pragma once

#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

#include "common/common_types.h"
#include "core/hle/service/kernel_helpers.h"

namespace Common {
class Event;
} // namespace Common

namespace Core::Timing {
class CoreTiming;
struct EventType;
} // namespace Core::Timing

namespace Kernel {
class KReadableEvent;
class KWritableEvent;
} // namespace Kernel

namespace Service::Nvidia {
class Module;
} // namespace Service::Nvidia

namespace Service::VI {
class Display;
class Layer;
} // namespace Service::VI

namespace Service::android {
class BufferQueueCore;
class BufferQueueProducer;
} // namespace Service::android

namespace Service::NVFlinger {

class NVFlinger final {
public:
    explicit NVFlinger(Core::System& system_, HosBinderDriverServer& hos_binder_driver_server_);
    ~NVFlinger();

    /// Sets the NVDrv module instance to use to send buffers to the GPU.
    void SetNVDrvInstance(std::shared_ptr<Nvidia::Module> instance);

    /// Opens the specified display and returns the ID.
    ///
    /// If an invalid display name is provided, then an empty optional is returned.
    [[nodiscard]] std::optional<u64> OpenDisplay(std::string_view name);

    /// Creates a layer on the specified display and returns the layer ID.
    ///
    /// If an invalid display ID is specified, then an empty optional is returned.
    [[nodiscard]] std::optional<u64> CreateLayer(u64 display_id);

    /// Closes a layer on all displays for the given layer ID.
    void CloseLayer(u64 layer_id);

    /// Finds the buffer queue ID of the specified layer in the specified display.
    ///
    /// If an invalid display ID or layer ID is provided, then an empty optional is returned.
    [[nodiscard]] std::optional<u32> FindBufferQueueId(u64 display_id, u64 layer_id);

    /// Gets the vsync event for the specified display.
    ///
    /// If an invalid display ID is provided, then nullptr is returned.
    [[nodiscard]] Kernel::KReadableEvent* FindVsyncEvent(u64 display_id);

    /// Performs a composition request to the emulated nvidia GPU and triggers the vsync events when
    /// finished.
    void Compose();

    [[nodiscard]] s64 GetNextTicks() const;

private:
    struct Layer {
        std::unique_ptr<android::BufferQueueCore> core;
        std::unique_ptr<android::BufferQueueProducer> producer;
    };

private:
    [[nodiscard]] std::unique_lock<std::mutex> Lock() const {
        return std::unique_lock{*guard};
    }

    /// Finds the display identified by the specified ID.
    [[nodiscard]] VI::Display* FindDisplay(u64 display_id);

    /// Finds the display identified by the specified ID.
    [[nodiscard]] const VI::Display* FindDisplay(u64 display_id) const;

    /// Finds the layer identified by the specified ID in the desired display.
    [[nodiscard]] VI::Layer* FindLayer(u64 display_id, u64 layer_id);

    /// Finds the layer identified by the specified ID in the desired display.
    [[nodiscard]] const VI::Layer* FindLayer(u64 display_id, u64 layer_id) const;

    /// Finds the layer identified by the specified ID in the desired display,
    /// or creates the layer if it is not found.
    /// To be used when the system expects the specified ID to already exist.
    [[nodiscard]] VI::Layer* FindOrCreateLayer(u64 display_id, u64 layer_id);

    /// Creates a layer with the specified layer ID in the desired display.
    void CreateLayerAtId(VI::Display& display, u64 layer_id);

    void SplitVSync(std::stop_token stop_token);

    std::shared_ptr<Nvidia::Module> nvdrv;

    std::list<VI::Display> displays;

    /// Id to use for the next layer that is created, this counter is shared among all displays.
    u64 next_layer_id = 1;
    /// Id to use for the next buffer queue that is created, this counter is shared among all
    /// layers.
    u32 next_buffer_queue_id = 1;

    u32 swap_interval = 1;

    /// Event that handles screen composition.
    std::shared_ptr<Core::Timing::EventType> composition_event;

    std::shared_ptr<std::mutex> guard;

    Core::System& system;

    std::jthread vsync_thread;

    KernelHelpers::ServiceContext service_context;

    HosBinderDriverServer& hos_binder_driver_server;
};

} // namespace Service::NVFlinger
