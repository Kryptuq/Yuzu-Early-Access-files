// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <random>

#include "core/core.h"
#include "core/core_timing.h"
#include "core/hid/emulated_controller.h"
#include "core/hid/hid_core.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/k_shared_memory.h"
#include "core/hle/kernel/k_transfer_memory.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/service/hid/errors.h"
#include "core/hle/service/hid/irs.h"
#include "core/hle/service/hid/irsensor/clustering_processor.h"
#include "core/hle/service/hid/irsensor/image_transfer_processor.h"
#include "core/hle/service/hid/irsensor/ir_led_processor.h"
#include "core/hle/service/hid/irsensor/moment_processor.h"
#include "core/hle/service/hid/irsensor/pointing_processor.h"
#include "core/hle/service/hid/irsensor/tera_plugin_processor.h"
#include "core/memory.h"

namespace Service::IRS {

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

    u8* raw_shared_memory = system.Kernel().GetIrsSharedMem().GetPointer();
    RegisterHandlers(functions);
    shared_memory = std::construct_at(reinterpret_cast<StatusManager*>(raw_shared_memory));

    npad_device = system.HIDCore().GetEmulatedController(Core::HID::NpadIdType::Player1);
}
IRS::~IRS() = default;

void IRS::ActivateIrsensor(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto applet_resource_user_id{rp.Pop<u64>()};

    LOG_WARNING(Service_IRS, "(STUBBED) called, applet_resource_user_id={}",
                applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void IRS::DeactivateIrsensor(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto applet_resource_user_id{rp.Pop<u64>()};

    LOG_WARNING(Service_IRS, "(STUBBED) called, applet_resource_user_id={}",
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
        Core::IrSensor::IrCameraHandle camera_handle;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    LOG_WARNING(Service_IRS,
                "(STUBBED) called, npad_type={}, npad_id={}, applet_resource_user_id={}",
                parameters.camera_handle.npad_type, parameters.camera_handle.npad_id,
                parameters.applet_resource_user_id);

    auto result = IsIrCameraHandleValid(parameters.camera_handle);
    if (result.IsSuccess()) {
        // TODO: Stop Image processor
        npad_device->SetPollingMode(Core::HID::EmulatedDeviceIndex::RightIndex,
                                    Common::Input::PollingMode::Active);
        result = ResultSuccess;
    }

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void IRS::RunMomentProcessor(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        Core::IrSensor::IrCameraHandle camera_handle;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
        Core::IrSensor::PackedMomentProcessorConfig processor_config;
    };
    static_assert(sizeof(Parameters) == 0x30, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    LOG_WARNING(Service_IRS,
                "(STUBBED) called, npad_type={}, npad_id={}, applet_resource_user_id={}",
                parameters.camera_handle.npad_type, parameters.camera_handle.npad_id,
                parameters.applet_resource_user_id);

    const auto result = IsIrCameraHandleValid(parameters.camera_handle);

    if (result.IsSuccess()) {
        auto& device = GetIrCameraSharedMemoryDeviceEntry(parameters.camera_handle);
        MakeProcessor<MomentProcessor>(parameters.camera_handle, device);
        auto& image_transfer_processor = GetProcessor<MomentProcessor>(parameters.camera_handle);
        image_transfer_processor.SetConfig(parameters.processor_config);
        npad_device->SetPollingMode(Core::HID::EmulatedDeviceIndex::RightIndex,
                                    Common::Input::PollingMode::IR);
    }

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void IRS::RunClusteringProcessor(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        Core::IrSensor::IrCameraHandle camera_handle;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
        Core::IrSensor::PackedClusteringProcessorConfig processor_config;
    };
    static_assert(sizeof(Parameters) == 0x38, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    LOG_WARNING(Service_IRS,
                "(STUBBED) called, npad_type={}, npad_id={}, applet_resource_user_id={}",
                parameters.camera_handle.npad_type, parameters.camera_handle.npad_id,
                parameters.applet_resource_user_id);

    auto result = IsIrCameraHandleValid(parameters.camera_handle);

    if (result.IsSuccess()) {
        auto& device = GetIrCameraSharedMemoryDeviceEntry(parameters.camera_handle);
        MakeProcessorWithCoreContext<ClusteringProcessor>(parameters.camera_handle, device);
        auto& image_transfer_processor =
            GetProcessor<ClusteringProcessor>(parameters.camera_handle);
        image_transfer_processor.SetConfig(parameters.processor_config);
        npad_device->SetPollingMode(Core::HID::EmulatedDeviceIndex::RightIndex,
                                    Common::Input::PollingMode::IR);
    }

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void IRS::RunImageTransferProcessor(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        Core::IrSensor::IrCameraHandle camera_handle;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
        Core::IrSensor::PackedImageTransferProcessorConfig processor_config;
        u32 transfer_memory_size;
    };
    static_assert(sizeof(Parameters) == 0x30, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};
    const auto t_mem_handle{ctx.GetCopyHandle(0)};

    auto t_mem = system.ApplicationProcess()->GetHandleTable().GetObject<Kernel::KTransferMemory>(
        t_mem_handle);

    if (t_mem.IsNull()) {
        LOG_ERROR(Service_IRS, "t_mem is a nullptr for handle=0x{:08X}", t_mem_handle);
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultUnknown);
        return;
    }

    ASSERT_MSG(t_mem->GetSize() == parameters.transfer_memory_size, "t_mem has incorrect size");

    u8* transfer_memory = system.Memory().GetPointer(t_mem->GetSourceAddress());

    LOG_INFO(Service_IRS,
             "called, npad_type={}, npad_id={}, transfer_memory_size={}, transfer_memory_size={}, "
             "applet_resource_user_id={}",
             parameters.camera_handle.npad_type, parameters.camera_handle.npad_id,
             parameters.transfer_memory_size, t_mem->GetSize(), parameters.applet_resource_user_id);

    const auto result = IsIrCameraHandleValid(parameters.camera_handle);

    if (result.IsSuccess()) {
        auto& device = GetIrCameraSharedMemoryDeviceEntry(parameters.camera_handle);
        MakeProcessorWithCoreContext<ImageTransferProcessor>(parameters.camera_handle, device);
        auto& image_transfer_processor =
            GetProcessor<ImageTransferProcessor>(parameters.camera_handle);
        image_transfer_processor.SetConfig(parameters.processor_config);
        image_transfer_processor.SetTransferMemoryPointer(transfer_memory);
        npad_device->SetPollingMode(Core::HID::EmulatedDeviceIndex::RightIndex,
                                    Common::Input::PollingMode::IR);
    }

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void IRS::GetImageTransferProcessorState(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        Core::IrSensor::IrCameraHandle camera_handle;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    LOG_DEBUG(Service_IRS, "(STUBBED) called, npad_type={}, npad_id={}, applet_resource_user_id={}",
              parameters.camera_handle.npad_type, parameters.camera_handle.npad_id,
              parameters.applet_resource_user_id);

    const auto result = IsIrCameraHandleValid(parameters.camera_handle);
    if (result.IsError()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(result);
        return;
    }

    const auto& device = GetIrCameraSharedMemoryDeviceEntry(parameters.camera_handle);

    if (device.mode != Core::IrSensor::IrSensorMode::ImageTransferProcessor) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(InvalidProcessorState);
        return;
    }

    std::vector<u8> data{};
    const auto& image_transfer_processor =
        GetProcessor<ImageTransferProcessor>(parameters.camera_handle);
    const auto& state = image_transfer_processor.GetState(data);

    ctx.WriteBuffer(data);
    IPC::ResponseBuilder rb{ctx, 6};
    rb.Push(ResultSuccess);
    rb.PushRaw(state);
}

void IRS::RunTeraPluginProcessor(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        Core::IrSensor::IrCameraHandle camera_handle;
        Core::IrSensor::PackedTeraPluginProcessorConfig processor_config;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x18, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    LOG_WARNING(
        Service_IRS,
        "(STUBBED) called, npad_type={}, npad_id={}, mode={}, mcu_version={}.{}, "
        "applet_resource_user_id={}",
        parameters.camera_handle.npad_type, parameters.camera_handle.npad_id,
        parameters.processor_config.mode, parameters.processor_config.required_mcu_version.major,
        parameters.processor_config.required_mcu_version.minor, parameters.applet_resource_user_id);

    const auto result = IsIrCameraHandleValid(parameters.camera_handle);

    if (result.IsSuccess()) {
        auto& device = GetIrCameraSharedMemoryDeviceEntry(parameters.camera_handle);
        MakeProcessor<TeraPluginProcessor>(parameters.camera_handle, device);
        auto& image_transfer_processor =
            GetProcessor<TeraPluginProcessor>(parameters.camera_handle);
        image_transfer_processor.SetConfig(parameters.processor_config);
        npad_device->SetPollingMode(Core::HID::EmulatedDeviceIndex::RightIndex,
                                    Common::Input::PollingMode::IR);
    }

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void IRS::GetNpadIrCameraHandle(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto npad_id{rp.PopEnum<Core::HID::NpadIdType>()};

    if (npad_id > Core::HID::NpadIdType::Player8 && npad_id != Core::HID::NpadIdType::Invalid &&
        npad_id != Core::HID::NpadIdType::Handheld) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(Service::HID::InvalidNpadId);
        return;
    }

    Core::IrSensor::IrCameraHandle camera_handle{
        .npad_id = static_cast<u8>(NpadIdTypeToIndex(npad_id)),
        .npad_type = Core::HID::NpadStyleIndex::None,
    };

    LOG_INFO(Service_IRS, "called, npad_id={}, camera_npad_id={}, camera_npad_type={}", npad_id,
             camera_handle.npad_id, camera_handle.npad_type);

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.PushRaw(camera_handle);
}

void IRS::RunPointingProcessor(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto camera_handle{rp.PopRaw<Core::IrSensor::IrCameraHandle>()};
    const auto processor_config{rp.PopRaw<Core::IrSensor::PackedPointingProcessorConfig>()};
    const auto applet_resource_user_id{rp.Pop<u64>()};

    LOG_WARNING(
        Service_IRS,
        "(STUBBED) called, npad_type={}, npad_id={}, mcu_version={}.{}, applet_resource_user_id={}",
        camera_handle.npad_type, camera_handle.npad_id, processor_config.required_mcu_version.major,
        processor_config.required_mcu_version.minor, applet_resource_user_id);

    auto result = IsIrCameraHandleValid(camera_handle);

    if (result.IsSuccess()) {
        auto& device = GetIrCameraSharedMemoryDeviceEntry(camera_handle);
        MakeProcessor<PointingProcessor>(camera_handle, device);
        auto& image_transfer_processor = GetProcessor<PointingProcessor>(camera_handle);
        image_transfer_processor.SetConfig(processor_config);
        npad_device->SetPollingMode(Core::HID::EmulatedDeviceIndex::RightIndex,
                                    Common::Input::PollingMode::IR);
    }

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void IRS::SuspendImageProcessor(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        Core::IrSensor::IrCameraHandle camera_handle;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    LOG_WARNING(Service_IRS,
                "(STUBBED) called, npad_type={}, npad_id={}, applet_resource_user_id={}",
                parameters.camera_handle.npad_type, parameters.camera_handle.npad_id,
                parameters.applet_resource_user_id);

    auto result = IsIrCameraHandleValid(parameters.camera_handle);
    if (result.IsSuccess()) {
        // TODO: Suspend image processor
        result = ResultSuccess;
    }

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void IRS::CheckFirmwareVersion(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto camera_handle{rp.PopRaw<Core::IrSensor::IrCameraHandle>()};
    const auto mcu_version{rp.PopRaw<Core::IrSensor::PackedMcuVersion>()};
    const auto applet_resource_user_id{rp.Pop<u64>()};

    LOG_WARNING(
        Service_IRS,
        "(STUBBED) called, npad_type={}, npad_id={}, applet_resource_user_id={}, mcu_version={}.{}",
        camera_handle.npad_type, camera_handle.npad_id, applet_resource_user_id, mcu_version.major,
        mcu_version.minor);

    auto result = IsIrCameraHandleValid(camera_handle);
    if (result.IsSuccess()) {
        // TODO: Check firmware version
        result = ResultSuccess;
    }

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void IRS::SetFunctionLevel(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto camera_handle{rp.PopRaw<Core::IrSensor::IrCameraHandle>()};
    const auto function_level{rp.PopRaw<Core::IrSensor::PackedFunctionLevel>()};
    const auto applet_resource_user_id{rp.Pop<u64>()};

    LOG_WARNING(
        Service_IRS,
        "(STUBBED) called, npad_type={}, npad_id={}, function_level={}, applet_resource_user_id={}",
        camera_handle.npad_type, camera_handle.npad_id, function_level.function_level,
        applet_resource_user_id);

    auto result = IsIrCameraHandleValid(camera_handle);
    if (result.IsSuccess()) {
        // TODO: Set Function level
        result = ResultSuccess;
    }

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void IRS::RunImageTransferExProcessor(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        Core::IrSensor::IrCameraHandle camera_handle;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
        Core::IrSensor::PackedImageTransferProcessorExConfig processor_config;
        u64 transfer_memory_size;
    };
    static_assert(sizeof(Parameters) == 0x38, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};
    const auto t_mem_handle{ctx.GetCopyHandle(0)};

    auto t_mem = system.ApplicationProcess()->GetHandleTable().GetObject<Kernel::KTransferMemory>(
        t_mem_handle);

    u8* transfer_memory = system.Memory().GetPointer(t_mem->GetSourceAddress());

    LOG_INFO(Service_IRS,
             "called, npad_type={}, npad_id={}, transfer_memory_size={}, "
             "applet_resource_user_id={}",
             parameters.camera_handle.npad_type, parameters.camera_handle.npad_id,
             parameters.transfer_memory_size, parameters.applet_resource_user_id);

    auto result = IsIrCameraHandleValid(parameters.camera_handle);

    if (result.IsSuccess()) {
        auto& device = GetIrCameraSharedMemoryDeviceEntry(parameters.camera_handle);
        MakeProcessorWithCoreContext<ImageTransferProcessor>(parameters.camera_handle, device);
        auto& image_transfer_processor =
            GetProcessor<ImageTransferProcessor>(parameters.camera_handle);
        image_transfer_processor.SetConfig(parameters.processor_config);
        image_transfer_processor.SetTransferMemoryPointer(transfer_memory);
        npad_device->SetPollingMode(Core::HID::EmulatedDeviceIndex::RightIndex,
                                    Common::Input::PollingMode::IR);
    }

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void IRS::RunIrLedProcessor(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto camera_handle{rp.PopRaw<Core::IrSensor::IrCameraHandle>()};
    const auto processor_config{rp.PopRaw<Core::IrSensor::PackedIrLedProcessorConfig>()};
    const auto applet_resource_user_id{rp.Pop<u64>()};

    LOG_WARNING(Service_IRS,
                "(STUBBED) called, npad_type={}, npad_id={}, light_target={}, mcu_version={}.{} "
                "applet_resource_user_id={}",
                camera_handle.npad_type, camera_handle.npad_id, processor_config.light_target,
                processor_config.required_mcu_version.major,
                processor_config.required_mcu_version.minor, applet_resource_user_id);

    auto result = IsIrCameraHandleValid(camera_handle);

    if (result.IsSuccess()) {
        auto& device = GetIrCameraSharedMemoryDeviceEntry(camera_handle);
        MakeProcessor<IrLedProcessor>(camera_handle, device);
        auto& image_transfer_processor = GetProcessor<IrLedProcessor>(camera_handle);
        image_transfer_processor.SetConfig(processor_config);
        npad_device->SetPollingMode(Core::HID::EmulatedDeviceIndex::RightIndex,
                                    Common::Input::PollingMode::IR);
    }

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void IRS::StopImageProcessorAsync(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        Core::IrSensor::IrCameraHandle camera_handle;
        INSERT_PADDING_WORDS_NOINIT(1);
        u64 applet_resource_user_id;
    };
    static_assert(sizeof(Parameters) == 0x10, "Parameters has incorrect size.");

    const auto parameters{rp.PopRaw<Parameters>()};

    LOG_WARNING(Service_IRS,
                "(STUBBED) called, npad_type={}, npad_id={}, applet_resource_user_id={}",
                parameters.camera_handle.npad_type, parameters.camera_handle.npad_id,
                parameters.applet_resource_user_id);

    auto result = IsIrCameraHandleValid(parameters.camera_handle);
    if (result.IsSuccess()) {
        // TODO: Stop image processor async
        npad_device->SetPollingMode(Core::HID::EmulatedDeviceIndex::RightIndex,
                                    Common::Input::PollingMode::Active);
        result = ResultSuccess;
    }

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(result);
}

void IRS::ActivateIrsensorWithFunctionLevel(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    struct Parameters {
        Core::IrSensor::PackedFunctionLevel function_level;
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

Result IRS::IsIrCameraHandleValid(const Core::IrSensor::IrCameraHandle& camera_handle) const {
    if (camera_handle.npad_id >
        static_cast<u8>(NpadIdTypeToIndex(Core::HID::NpadIdType::Handheld))) {
        return InvalidIrCameraHandle;
    }
    if (camera_handle.npad_type != Core::HID::NpadStyleIndex::None) {
        return InvalidIrCameraHandle;
    }
    return ResultSuccess;
}

Core::IrSensor::DeviceFormat& IRS::GetIrCameraSharedMemoryDeviceEntry(
    const Core::IrSensor::IrCameraHandle& camera_handle) {
    const auto npad_id_max_index = static_cast<u8>(sizeof(StatusManager::device));
    ASSERT_MSG(camera_handle.npad_id < npad_id_max_index, "invalid npad_id");
    return shared_memory->device[camera_handle.npad_id];
}

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

} // namespace Service::IRS
