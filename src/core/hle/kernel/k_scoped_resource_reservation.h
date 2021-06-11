// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

// This file references various implementation details from Atmosphere, an open-source firmware for
// the Nintendo Switch. Copyright 2018-2020 Atmosphere-NX.

#pragma once

#include "common/common_types.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/k_resource_limit.h"

namespace Kernel {

class KScopedResourceReservation {
public:
    explicit KScopedResourceReservation(KResourceLimit* l, LimitableResource r, s64 v, s64 timeout)
        : resource_limit(std::move(l)), value(v), resource(r) {
        if (resource_limit && value) {
            success = resource_limit->Reserve(resource, value, timeout);
        } else {
            success = true;
        }
    }

    explicit KScopedResourceReservation(KResourceLimit* l, LimitableResource r, s64 v = 1)
        : resource_limit(std::move(l)), value(v), resource(r) {
        if (resource_limit && value) {
            success = resource_limit->Reserve(resource, value);
        } else {
            success = true;
        }
    }

    explicit KScopedResourceReservation(const KProcess* p, LimitableResource r, s64 v, s64 t)
        : KScopedResourceReservation(p->GetResourceLimit(), r, v, t) {}

    explicit KScopedResourceReservation(const KProcess* p, LimitableResource r, s64 v = 1)
        : KScopedResourceReservation(p->GetResourceLimit(), r, v) {}

    ~KScopedResourceReservation() noexcept {
        if (resource_limit && value && success) {
            // resource was not committed, release the reservation.
            resource_limit->Release(resource, value);
        }
    }

    /// Commit the resource reservation, destruction of this object does not release the resource
    void Commit() {
        resource_limit = nullptr;
    }

    [[nodiscard]] bool Succeeded() const {
        return success;
    }

private:
    KResourceLimit* resource_limit{};
    s64 value;
    LimitableResource resource;
    bool success;
};

} // namespace Kernel
