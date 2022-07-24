// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "common/common_funcs.h"
#include "common/common_types.h"

namespace Kernel {
class KEvent;
}

namespace Service::android {
class BufferQueueProducer;
}

namespace Service::KernelHelpers {
class ServiceContext;
}

namespace Service::NVFlinger {
class HosBinderDriverServer;
}

namespace Service::Nvidia::NvCore {
class Container;
class NvMap;
} // namespace Service::Nvidia::NvCore

namespace Service::VI {

class Layer;

/// Represents a single display type
class Display {
public:
    YUZU_NON_COPYABLE(Display);
    YUZU_NON_MOVEABLE(Display);

    /// Constructs a display with a given unique ID and name.
    ///
    /// @param id The unique ID for this display.
    /// @param hos_binder_driver_server_ NVFlinger HOSBinderDriver server instance.
    /// @param service_context_ The ServiceContext for the owning service.
    /// @param name_ The name for this display.
    /// @param system_ The global system instance.
    ///
    Display(u64 id, std::string name_, NVFlinger::HosBinderDriverServer& hos_binder_driver_server_,
            KernelHelpers::ServiceContext& service_context_, Core::System& system_);
    ~Display();

    /// Gets the unique ID assigned to this display.
    u64 GetID() const {
        return display_id;
    }

    /// Gets the name of this display
    const std::string& GetName() const {
        return name;
    }

    /// Whether or not this display has any layers added to it.
    bool HasLayers() const {
        return !layers.empty();
    }

    /// Gets a layer for this display based off an index.
    Layer& GetLayer(std::size_t index);

    /// Gets a layer for this display based off an index.
    const Layer& GetLayer(std::size_t index) const;

    std::size_t GetNumLayers() const {
        return layers.size();
    }

    /// Gets the readable vsync event.
    Kernel::KReadableEvent& GetVSyncEvent();

    /// Signals the internal vsync event.
    void SignalVSyncEvent();

    /// Creates and adds a layer to this display with the given ID.
    ///
    /// @param layer_id The ID to assign to the created layer.
    /// @param binder_id The ID assigned to the buffer queue.
    ///
    void CreateLayer(u64 layer_id, u32 binder_id, Service::Nvidia::NvCore::Container& core);

    /// Closes and removes a layer from this display with the given ID.
    ///
    /// @param layer_id The ID assigned to the layer to close.
    ///
    void CloseLayer(u64 layer_id);

    /// Attempts to find a layer with the given ID.
    ///
    /// @param layer_id The layer ID.
    ///
    /// @returns If found, the Layer instance with the given ID.
    ///          If not found, then nullptr is returned.
    ///
    Layer* FindLayer(u64 layer_id);

    /// Attempts to find a layer with the given ID.
    ///
    /// @param layer_id The layer ID.
    ///
    /// @returns If found, the Layer instance with the given ID.
    ///          If not found, then nullptr is returned.
    ///
    const Layer* FindLayer(u64 layer_id) const;

private:
    u64 display_id;
    std::string name;
    NVFlinger::HosBinderDriverServer& hos_binder_driver_server;
    KernelHelpers::ServiceContext& service_context;

    std::vector<std::unique_ptr<Layer>> layers;
    Kernel::KEvent* vsync_event{};
};

} // namespace Service::VI
