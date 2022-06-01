// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "core/core.h"
#include "core/core_timing.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/k_shared_memory.h"
#include "core/hle/kernel/k_transfer_memory.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/service/hid/errors.h"
#include "core/hle/service/hid/irs.h"

namespace Service::HID {

IRS::IRS(Core::System& system_) : ServiceFramework{system_, "irs"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {302, &IRS::ActivateIrsensor, "ActivateIrsensor"},
        {303, &IRS::DeactivateIrsensor, "DeactivateIrsensor"},
        {304, &IRS::GetIrsensorSharedMemoryHandle, "GetIrsensorSharedMemoryHandle"},
        {305, &IRS::StopImageProcessor, "StopImageProcessor"},
        {306, &IRS::RunMomentProcessor, "RunMomentProcessor"},
        {307, &IRS::RunClusteringProcessor, "RunClusteringProcessor"},
        {308, &IRS::RunImageTransferProcessor, "RunImageTransferProcessor"},
        {309, &IRS::GetImageTransferProcessorState, "GetImageTransferProcessorState"},
        {310, &IRS::RunTeraPluginProcessor, "RunTeraPluginProcessor"},
        {311, &IRS::GetNpadIrCameraHandle, "GetNpadIrCameraHandle"},
        {312, &IRS::RunPointingProcessor, "RunPointingProcessor"},
        {313, &IRS::SuspendImageProcessor, "SuspendImageProcessor"},
        {314, &IRS::CheckFirmwareVersion, "CheckFirmwareVersion"},
        {315, &IRS::SetFunctionLevel, "SetFunctionLevel"},
        {316, &IRS::RunImageTransferExProcessor, "RunImageTransferExProcessor"},
        {317, &IRS::RunIrLedProcessor, "RunIrLedProcessor"},
        {318, &IRS::StopImageProcessorAsync, "StopImageProcessorAsync"},
        {319, &IRS::ActivateIrsensorWithFunctionLevel, "ActivateIrsensorWithFunctionLevel"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

void IRS::ActivateIrsensor(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto applet_resource_user_id{rp.Pop<u64>()};

    LOG_WARNING(Service_HID, "(STUBBED) called, applet_resource_user_id={}",
                applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void IRS::DeactivateIrsensor(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto applet_resource_user_id{rp.Pop<u64>()};

    LOG_WARNING(Service_HID, "(STUBBED) called, applet_resource_user_id={}",
                applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void IRS::GetIrsensorSharedMemoryHandle(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto applet_resource_user_id{rp.Pop<u64>()};

    LOG_DEBUG(Service_IRS, "called, applet_resource_user_id={}", applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2, 1};
    rb.Push(ResultSuccess);
    rb.PushCopyObjects(&system.Kernel().GetIrsSharedMem());
}

void IRS::StopImageProcessor(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        IrCameraHandle camera_handle;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    LOG_WARNING(Service_IRS,
                "(STUBBED) called, npad_type={}, npad_id={}, applet_resource_user_id={}",
                parameters.camera_handle.npad_type, parameters.camera_handle.npad_id,
                parameters.applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void IRS::RunMomentProcessor(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        IrCameraHandle camera_handle;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
        PackedMomentProcessorConfig processor_config;
    };
    static_assert(sizeof(Parameters) == 0x30, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    LOG_WARNING(Service_IRS,
                "(STUBBED) called, npad_type={}, npad_id={}, applet_resource_user_id={}",
                parameters.camera_handle.npad_type, parameters.camera_handle.npad_id,
                parameters.applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void IRS::RunClusteringProcessor(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        IrCameraHandle camera_handle;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
        PackedClusteringProcessorConfig processor_config;
    };
    static_assert(sizeof(Parameters) == 0x40, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    LOG_WARNING(Service_IRS,
                "(STUBBED) called, npad_type={}, npad_id={}, applet_resource_user_id={}",
                parameters.camera_handle.npad_type, parameters.camera_handle.npad_id,
                parameters.applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void IRS::RunImageTransferProcessor(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        IrCameraHandle camera_handle;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
        PackedImageTransferProcessorConfig processor_config;
        u32 transfer_memory_size;
    };
    static_assert(sizeof(Parameters) == 0x30, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};
    const auto t_mem_handle{ctx.GetCopyHandle(0)};

    auto t_mem =
        system.CurrentProcess()->GetHandleTable().GetObject<Kernel::KTransferMemory>(t_mem_handle);

    LOG_WARNING(Service_IRS,
                "(STUBBED) called, npad_type={}, npad_id={}, transfer_memory_size={}, "
                "applet_resource_user_id={}",
                parameters.camera_handle.npad_type, parameters.camera_handle.npad_id,
                parameters.transfer_memory_size, parameters.applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void IRS::GetImageTransferProcessorState(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        IrCameraHandle camera_handle;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    LOG_WARNING(Service_IRS,
                "(STUBBED) called, npad_type={}, npad_id={}, applet_resource_user_id={}",
                parameters.camera_handle.npad_type, parameters.camera_handle.npad_id,
                parameters.applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 5};
    rb.Push(ResultSuccess);
    rb.PushRaw<u64>(system.CoreTiming().GetCPUTicks());
    rb.PushRaw<u32>(0);
}

void IRS::RunTeraPluginProcessor(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto camera_handle{rp.PopRaw<IrCameraHandle>()};
    const auto processor_config{rp.PopRaw<PackedTeraPluginProcessorConfig>()};
    const auto applet_resource_user_id{rp.Pop<u64>()};

    LOG_WARNING(Service_IRS,
                "(STUBBED) called, npad_type={}, npad_id={}, mode={}, mcu_version={}.{}, "
                "applet_resource_user_id={}",
                camera_handle.npad_type, camera_handle.npad_id, processor_config.mode,
                processor_config.required_mcu_version.major,
                processor_config.required_mcu_version.minor, applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void IRS::GetNpadIrCameraHandle(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto npad_id{rp.PopEnum<Core::HID::NpadIdType>()};

    if (npad_id > Core::HID::NpadIdType::Player8 && npad_id != Core::HID::NpadIdType::Invalid &&
        npad_id != Core::HID::NpadIdType::Handheld) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(InvalidNpadId);
        return;
    }

    IrCameraHandle camera_handle{
        .npad_id = static_cast<u8>(NpadIdTypeToIndex(npad_id)),
        .npad_type = Core::HID::NpadStyleIndex::None,
    };

    LOG_WARNING(Service_IRS, "(STUBBED) called, npad_id={}, camera_npad_id={}, camera_npad_type={}",
                npad_id, camera_handle.npad_id, camera_handle.npad_type);

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.PushRaw(camera_handle);
}

void IRS::RunPointingProcessor(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto camera_handle{rp.PopRaw<IrCameraHandle>()};
    const auto processor_config{rp.PopRaw<PackedPointingProcessorConfig>()};
    const auto applet_resource_user_id{rp.Pop<u64>()};

    LOG_WARNING(
        Service_IRS,
        "(STUBBED) called, npad_type={}, npad_id={}, mcu_version={}.{}, applet_resource_user_id={}",
        camera_handle.npad_type, camera_handle.npad_id, processor_config.required_mcu_version.major,
        processor_config.required_mcu_version.minor, applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void IRS::SuspendImageProcessor(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        IrCameraHandle camera_handle;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    LOG_WARNING(Service_IRS,
                "(STUBBED) called, npad_type={}, npad_id={}, applet_resource_user_id={}",
                parameters.camera_handle.npad_type, parameters.camera_handle.npad_id,
                parameters.applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void IRS::CheckFirmwareVersion(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto camera_handle{rp.PopRaw<IrCameraHandle>()};
    const auto mcu_version{rp.PopRaw<PackedMcuVersion>()};
    const auto applet_resource_user_id{rp.Pop<u64>()};

    LOG_WARNING(
        Service_IRS,
        "(STUBBED) called, npad_type={}, npad_id={}, applet_resource_user_id={}, mcu_version={}.{}",
        camera_handle.npad_type, camera_handle.npad_id, applet_resource_user_id, mcu_version.major,
        mcu_version.minor);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void IRS::SetFunctionLevel(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        IrCameraHandle camera_handle;
        PackedFunctionLevel function_level;
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    LOG_WARNING(Service_IRS,
                "(STUBBED) called, npad_type={}, npad_id={}, applet_resource_user_id={}",
                parameters.camera_handle.npad_type, parameters.camera_handle.npad_id,
                parameters.applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void IRS::RunImageTransferExProcessor(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        IrCameraHandle camera_handle;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
        PackedImageTransferProcessorExConfig processor_config;
        u64 transfer_memory_size;
    };
    static_assert(sizeof(Parameters) == 0x38, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};
    const auto t_mem_handle{ctx.GetCopyHandle(0)};

    auto t_mem =
        system.CurrentProcess()->GetHandleTable().GetObject<Kernel::KTransferMemory>(t_mem_handle);

    LOG_WARNING(Service_IRS,
                "(STUBBED) called, npad_type={}, npad_id={}, transfer_memory_size={}, "
                "applet_resource_user_id={}",
                parameters.camera_handle.npad_type, parameters.camera_handle.npad_id,
                parameters.transfer_memory_size, parameters.applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void IRS::RunIrLedProcessor(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto camera_handle{rp.PopRaw<IrCameraHandle>()};
    const auto processor_config{rp.PopRaw<PackedIrLedProcessorConfig>()};
    const auto applet_resource_user_id{rp.Pop<u64>()};

    LOG_WARNING(Service_IRS,
                "(STUBBED) called, npad_type={}, npad_id={}, light_target={}, mcu_version={}.{} "
                "applet_resource_user_id={}",
                camera_handle.npad_type, camera_handle.npad_id, processor_config.light_target,
                processor_config.required_mcu_version.major,
                processor_config.required_mcu_version.minor, applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void IRS::StopImageProcessorAsync(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        IrCameraHandle camera_handle;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    LOG_WARNING(Service_IRS,
                "(STUBBED) called, npad_type={}, npad_id={}, applet_resource_user_id={}",
                parameters.camera_handle.npad_type, parameters.camera_handle.npad_id,
                parameters.applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void IRS::ActivateIrsensorWithFunctionLevel(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        PackedFunctionLevel function_level;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    LOG_WARNING(Service_IRS, "(STUBBED) called, function_level={}, applet_resource_user_id={}",
                parameters.function_level.function_level, parameters.applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

IRS::~IRS() = default;

IRS_SYS::IRS_SYS(Core::System& system_) : ServiceFramework{system_, "irs:sys"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {500, nullptr, "SetAppletResourceUserId"},
        {501, nullptr, "RegisterAppletResourceUserId"},
        {502, nullptr, "UnregisterAppletResourceUserId"},
        {503, nullptr, "EnableAppletToGetInput"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

IRS_SYS::~IRS_SYS() = default;

} // namespace Service::HID
