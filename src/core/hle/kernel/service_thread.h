// Copyright 2020 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <string>

namespace Kernel {

class HLERequestContext;
class KernelCore;
class KSession;

class ServiceThread final {
public:
    explicit ServiceThread(KernelCore& kernel, std::size_t num_threads, const std::string& name);
    ~ServiceThread();

    void QueueSyncRequest(KSession& session, std::shared_ptr<HLERequestContext>&& context);

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace Kernel
