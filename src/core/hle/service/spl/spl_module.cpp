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
#include "core/hle/api_version.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/spl/csrng.h"
#include "core/hle/service/spl/spl.h"
#include "core/hle/service/spl/spl_module.h"

namespace Service::SPL {

Module::Interface::Interface(Core::System& system_, std::shared_ptr<Module> module_,
                             const char* name)
    : ServiceFramework{system_, name}, module{std::move(module_)},
      rng(Settings::values.rng_seed.GetValue().value_or(std::time(nullptr))) {}

Module::Interface::~Interface() = default;

void Module::Interface::GetConfig(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto config_item = rp.PopEnum<ConfigItem>();

    // This should call svcCallSecureMonitor with the appropriate args.
    // Since we do not have it implemented yet, we will use this for now.
    const auto smc_result = GetConfigImpl(config_item);
    const auto result_code = smc_result.Code();

    if (smc_result.Failed()) {
        LOG_ERROR(Service_SPL, "called, config_item={}, result_code={}", config_item,
                  result_code.raw);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(result_code);
    }

    LOG_DEBUG(Service_SPL, "called, config_item={}, result_code={}, smc_result={}", config_item,
              result_code.raw, *smc_result);

    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(result_code);
    rb.Push(*smc_result);
}

void Module::Interface::ModularExponentiate(Kernel::HLERequestContext& ctx) {
    UNIMPLEMENTED_MSG("ModularExponentiate is not implemented!");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSecureMonitorNotImplemented);
}

void Module::Interface::SetConfig(Kernel::HLERequestContext& ctx) {
    UNIMPLEMENTED_MSG("SetConfig is not implemented!");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSecureMonitorNotImplemented);
}

void Module::Interface::GenerateRandomBytes(Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service_SPL, "called");

    const std::size_t size = ctx.GetWriteBufferSize();

    std::uniform_int_distribution<u16> distribution(0, std::numeric_limits<u8>::max());
    std::vector<u8> data(size);
    std::generate(data.begin(), data.end(), [&] { return static_cast<u8>(distribution(rng)); });

    ctx.WriteBuffer(data);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void Module::Interface::IsDevelopment(Kernel::HLERequestContext& ctx) {
    UNIMPLEMENTED_MSG("IsDevelopment is not implemented!");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSecureMonitorNotImplemented);
}

void Module::Interface::SetBootReason(Kernel::HLERequestContext& ctx) {
    UNIMPLEMENTED_MSG("SetBootReason is not implemented!");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSecureMonitorNotImplemented);
}

void Module::Interface::GetBootReason(Kernel::HLERequestContext& ctx) {
    UNIMPLEMENTED_MSG("GetBootReason is not implemented!");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSecureMonitorNotImplemented);
}

ResultVal<u64> Module::Interface::GetConfigImpl(ConfigItem config_item) const {
    switch (config_item) {
    case ConfigItem::DisableProgramVerification:
    case ConfigItem::DramId:
    case ConfigItem::SecurityEngineInterruptNumber:
    case ConfigItem::FuseVersion:
    case ConfigItem::HardwareType:
    case ConfigItem::HardwareState:
    case ConfigItem::IsRecoveryBoot:
    case ConfigItem::DeviceId:
    case ConfigItem::BootReason:
    case ConfigItem::MemoryMode:
    case ConfigItem::IsDevelopmentFunctionEnabled:
    case ConfigItem::KernelConfiguration:
    case ConfigItem::IsChargerHiZModeEnabled:
    case ConfigItem::QuestState:
    case ConfigItem::RegulatorType:
    case ConfigItem::DeviceUniqueKeyGeneration:
    case ConfigItem::Package2Hash:
        return ResultSecureMonitorNotImplemented;
    case ConfigItem::ExosphereApiVersion:
        // Get information about the current exosphere version.
        return MakeResult((u64{HLE::ApiVersion::ATMOSPHERE_RELEASE_VERSION_MAJOR} << 56) |
                          (u64{HLE::ApiVersion::ATMOSPHERE_RELEASE_VERSION_MINOR} << 48) |
                          (u64{HLE::ApiVersion::ATMOSPHERE_RELEASE_VERSION_MICRO} << 40) |
                          (static_cast<u64>(HLE::ApiVersion::GetTargetFirmware())));
    case ConfigItem::ExosphereNeedsReboot:
        // We are executing, so we aren't in the process of rebooting.
        return MakeResult(u64{0});
    case ConfigItem::ExosphereNeedsShutdown:
        // We are executing, so we aren't in the process of shutting down.
        return MakeResult(u64{0});
    case ConfigItem::ExosphereGitCommitHash:
        // Get information about the current exosphere git commit hash.
        return MakeResult(u64{0});
    case ConfigItem::ExosphereHasRcmBugPatch:
        // Get information about whether this unit has the RCM bug patched.
        return MakeResult(u64{0});
    case ConfigItem::ExosphereBlankProdInfo:
        // Get whether this unit should simulate a "blanked" PRODINFO.
        return MakeResult(u64{0});
    case ConfigItem::ExosphereAllowCalWrites:
        // Get whether this unit should allow writing to the calibration partition.
        return MakeResult(u64{0});
    case ConfigItem::ExosphereEmummcType:
        // Get what kind of emummc this unit has active.
        return MakeResult(u64{0});
    case ConfigItem::ExospherePayloadAddress:
        // Gets the physical address of the reboot payload buffer, if one exists.
        return ResultSecureMonitorNotInitialized;
    case ConfigItem::ExosphereLogConfiguration:
        // Get the log configuration.
        return MakeResult(u64{0});
    case ConfigItem::ExosphereForceEnableUsb30:
        // Get whether usb 3.0 should be force-enabled.
        return MakeResult(u64{0});
    default:
        return ResultSecureMonitorInvalidArgument;
    }
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
