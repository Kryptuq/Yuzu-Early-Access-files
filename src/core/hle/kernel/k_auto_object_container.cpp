// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>

#include "core/hle/kernel/k_auto_object_container.h"

namespace Kernel {

void KAutoObjectWithListContainer::Register(KAutoObjectWithList* obj) {
    KScopedLightLock lk(m_lock);

    m_object_list.insert_unique(*obj);
}

void KAutoObjectWithListContainer::Unregister(KAutoObjectWithList* obj) {
    KScopedLightLock lk(m_lock);

    m_object_list.erase(*obj);
}

size_t KAutoObjectWithListContainer::GetOwnedCount(KProcess* owner) {
    KScopedLightLock lk(m_lock);

    return std::count_if(m_object_list.begin(), m_object_list.end(),
                         [&](const auto& obj) { return obj.GetOwner() == owner; });
}

} // namespace Kernel
