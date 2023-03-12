// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/common_types.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/k_resource_limit.h"

namespace Kernel {

class KScopedResourceReservation {
public:
    explicit KScopedResourceReservation(KResourceLimit* l, LimitableResource r, s64 v, s64 timeout)
        : m_limit(l), m_value(v), m_resource(r) {
        if (m_limit && m_value) {
            m_succeeded = m_limit->Reserve(m_resource, m_value, timeout);
        } else {
            m_succeeded = true;
        }
    }

    explicit KScopedResourceReservation(KResourceLimit* l, LimitableResource r, s64 v = 1)
        : m_limit(l), m_value(v), m_resource(r) {
        if (m_limit && m_value) {
            m_succeeded = m_limit->Reserve(m_resource, m_value);
        } else {
            m_succeeded = true;
        }
    }

    explicit KScopedResourceReservation(const KProcess* p, LimitableResource r, s64 v, s64 t)
        : KScopedResourceReservation(p->GetResourceLimit(), r, v, t) {}

    explicit KScopedResourceReservation(const KProcess* p, LimitableResource r, s64 v = 1)
        : KScopedResourceReservation(p->GetResourceLimit(), r, v) {}

    ~KScopedResourceReservation() noexcept {
        if (m_limit && m_value && m_succeeded) {
            // Resource was not committed, release the reservation.
            m_limit->Release(m_resource, m_value);
        }
    }

    /// Commit the resource reservation, destruction of this object does not release the resource
    void Commit() {
        m_limit = nullptr;
    }

    bool Succeeded() const {
        return m_succeeded;
    }

private:
    KResourceLimit* m_limit{};
    s64 m_value{};
    LimitableResource m_resource{};
    bool m_succeeded{};
};

} // namespace Kernel
