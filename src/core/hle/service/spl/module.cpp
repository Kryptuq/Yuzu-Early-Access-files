// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <vector>
#include "common/logging/log.h"
#include "common/settings.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/spl/csrng.h"
#include "core/hle/service/spl/module.h"
#include "core/hle/service/spl/spl.h"

namespace Service::SPL {

Module::Interface::Interface(Core::System& system_, std::shared_ptr<Module> module_,
                             const char* name)
    : ServiceFramework{system_, name}, module{std::move(module_)},
      rng(Settings::values.rng_seed.GetValue().value_or(std::time(nullptr))) {}

Module::Interface::~Interface() = default;

void Module::Interface::GetRandomBytes(Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service_SPL, "called");

    const std::size_t size = ctx.GetWriteBufferSize();

    std::uniform_int_distribution<u16> distribution(0, std::numeric_limits<u8>::max());
    std::vector<u8> data(size);
    std::generate(data.begin(), data.end(), [&] { return static_cast<u8>(distribution(rng)); });

    ctx.WriteBuffer(data);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void InstallInterfaces(SM::ServiceManager& service_manager, Core::System& system) {
    auto module = std::make_shared<Module>();
    std::make_shared<CSRNG>(system, module)->InstallAsService(service_manager);
    std::make_shared<SPL>(system, module)->InstallAsService(service_manager);
    std::make_shared<SPL_MIG>(system, module)->InstallAsService(service_manager);
    std::make_shared<SPL_FS>(system, module)->InstallAsService(service_manager);
    std::make_shared<SPL_SSL>(system, module)->InstallAsService(service_manager);
    std::make_shared<SPL_ES>(system, module)->InstallAsService(service_manager);
    std::make_shared<SPL_MANU>(system, module)->InstallAsService(service_manager);
}

} // namespace Service::SPL
