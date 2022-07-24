// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/hle/kernel/k_auto_object.h"
#include "core/hle/kernel/kernel.h"

namespace Kernel {

KAutoObject* KAutoObject::Create(KAutoObject* obj) {
    obj->m_ref_count = 1;
    return obj;
}

void KAutoObject::RegisterWithKernel() {
    kernel.RegisterKernelObject(this);
}

void KAutoObject::UnregisterWithKernel() {
    kernel.UnregisterKernelObject(this);
}

} // namespace Kernel
