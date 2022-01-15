// Copyright 2022 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/kernel/k_synchronization_object.h"

namespace Kernel {

class KWorkerTask : public KSynchronizationObject {
public:
    explicit KWorkerTask(KernelCore& kernel_);

    void DoWorkerTask();
};

} // namespace Kernel
