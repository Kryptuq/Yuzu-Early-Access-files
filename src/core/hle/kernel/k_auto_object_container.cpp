// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

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
