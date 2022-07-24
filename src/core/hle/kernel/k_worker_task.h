// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/kernel/k_synchronization_object.h"

namespace Kernel {

class KWorkerTask : public KSynchronizationObject {
public:
    explicit KWorkerTask(KernelCore& kernel_);

    void DoWorkerTask();
};

} // namespace Kernel
