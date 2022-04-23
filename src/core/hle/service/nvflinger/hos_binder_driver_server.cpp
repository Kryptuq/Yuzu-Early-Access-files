// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include <mutex>

#include "common/common_types.h"
#include "core/hle/service/nvflinger/hos_binder_driver_server.h"

namespace Service::NVFlinger {

HosBinderDriverServer::HosBinderDriverServer(Core::System& system_)
    : service_context(system_, "HosBinderDriverServer") {}

HosBinderDriverServer::~HosBinderDriverServer() {}

u64 HosBinderDriverServer::RegisterProducer(std::unique_ptr<android::IBinder>&& binder) {
    std::scoped_lock lk{lock};

    last_id++;

    producers[last_id] = std::move(binder);

    return last_id;
}

android::IBinder* HosBinderDriverServer::TryGetProducer(u64 id) {
    std::scoped_lock lk{lock};

    if (auto search = producers.find(id); search != producers.end()) {
        return search->second.get();
    }

    return {};
}

} // namespace Service::NVFlinger
