// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cinttypes>
#include "common/logging/log.h"
#include "core/core.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/k_readable_event.h"
#include "core/hle/service/nvdrv/nvdata.h"
#include "core/hle/service/nvdrv/nvdrv.h"
#include "core/hle/service/nvdrv/nvdrv_interface.h"

namespace Service::Nvidia {

void NVDRV::SignalGPUInterruptSyncpt(const u32 syncpoint_id, const u32 value) {
    nvdrv->SignalSyncpt(syncpoint_id, value);
}

void NVDRV::Open(Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service_NVDRV, "called");
    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(ResultSuccess);

    if (!is_initialized) {
        rb.Push<DeviceFD>(0);
        rb.PushEnum(NvResult::NotInitialized);

        LOG_ERROR(Service_NVDRV, "NvServices is not initalized!");
        return;
    }

    const auto& buffer = ctx.ReadBuffer();
    const std::string device_name(buffer.begin(), buffer.end());

    if (device_name == "/dev/nvhost-prof-gpu") {
        rb.Push<DeviceFD>(0);
        rb.PushEnum(NvResult::NotSupported);

        LOG_WARNING(Service_NVDRV, "/dev/nvhost-prof-gpu cannot be opened in production");
        return;
    }

    DeviceFD fd = nvdrv->Open(device_name);

    rb.Push<DeviceFD>(fd);
    rb.PushEnum(fd != INVALID_NVDRV_FD ? NvResult::Success : NvResult::FileOperationFailed);
}

void NVDRV::ServiceError(Kernel::HLERequestContext& ctx, NvResult result) {
    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.PushEnum(result);
}

void NVDRV::Ioctl1(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto fd = rp.Pop<DeviceFD>();
    const auto command = rp.PopRaw<Ioctl>();
    LOG_DEBUG(Service_NVDRV, "called fd={}, ioctl=0x{:08X}", fd, command.raw);

    if (!is_initialized) {
        ServiceError(ctx, NvResult::NotInitialized);
        LOG_ERROR(Service_NVDRV, "NvServices is not initalized!");
        return;
    }

    // Check device
    std::vector<u8> output_buffer(ctx.GetWriteBufferSize(0));
    const auto input_buffer = ctx.ReadBuffer(0);

    const auto nv_result = nvdrv->Ioctl1(fd, command, input_buffer, output_buffer);
    if (command.is_out != 0) {
        ctx.WriteBuffer(output_buffer);
    }

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.PushEnum(nv_result);
}

void NVDRV::Ioctl2(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto fd = rp.Pop<DeviceFD>();
    const auto command = rp.PopRaw<Ioctl>();
    LOG_DEBUG(Service_NVDRV, "called fd={}, ioctl=0x{:08X}", fd, command.raw);

    if (!is_initialized) {
        ServiceError(ctx, NvResult::NotInitialized);
        LOG_ERROR(Service_NVDRV, "NvServices is not initalized!");
        return;
    }

    const auto input_buffer = ctx.ReadBuffer(0);
    const auto input_inlined_buffer = ctx.ReadBuffer(1);
    std::vector<u8> output_buffer(ctx.GetWriteBufferSize(0));

    const auto nv_result =
        nvdrv->Ioctl2(fd, command, input_buffer, input_inlined_buffer, output_buffer);
    if (command.is_out != 0) {
        ctx.WriteBuffer(output_buffer);
    }

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.PushEnum(nv_result);
}

void NVDRV::Ioctl3(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto fd = rp.Pop<DeviceFD>();
    const auto command = rp.PopRaw<Ioctl>();
    LOG_DEBUG(Service_NVDRV, "called fd={}, ioctl=0x{:08X}", fd, command.raw);

    if (!is_initialized) {
        ServiceError(ctx, NvResult::NotInitialized);
        LOG_ERROR(Service_NVDRV, "NvServices is not initalized!");
        return;
    }

    const auto input_buffer = ctx.ReadBuffer(0);
    std::vector<u8> output_buffer(ctx.GetWriteBufferSize(0));
    std::vector<u8> output_buffer_inline(ctx.GetWriteBufferSize(1));

    const auto nv_result =
        nvdrv->Ioctl3(fd, command, input_buffer, output_buffer, output_buffer_inline);
    if (command.is_out != 0) {
        ctx.WriteBuffer(output_buffer, 0);
        ctx.WriteBuffer(output_buffer_inline, 1);
    }

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.PushEnum(nv_result);
}

void NVDRV::Close(Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service_NVDRV, "called");

    if (!is_initialized) {
        ServiceError(ctx, NvResult::NotInitialized);
        LOG_ERROR(Service_NVDRV, "NvServices is not initalized!");
        return;
    }

    IPC::RequestParser rp{ctx};
    const auto fd = rp.Pop<DeviceFD>();
    const auto result = nvdrv->Close(fd);

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.PushEnum(result);
}

void NVDRV::Initialize(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service_NVDRV, "(STUBBED) called");

    is_initialized = true;

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.PushEnum(NvResult::Success);
}

void NVDRV::QueryEvent(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto fd = rp.Pop<DeviceFD>();
    const auto event_id = rp.Pop<u32>() & 0x00FF;
    LOG_WARNING(Service_NVDRV, "(STUBBED) called, fd={:X}, event_id={:X}", fd, event_id);

    if (!is_initialized) {
        ServiceError(ctx, NvResult::NotInitialized);
        LOG_ERROR(Service_NVDRV, "NvServices is not initalized!");
        return;
    }

    const auto nv_result = nvdrv->VerifyFD(fd);
    if (nv_result != NvResult::Success) {
        LOG_ERROR(Service_NVDRV, "Invalid FD specified DeviceFD={}!", fd);
        ServiceError(ctx, nv_result);
        return;
    }

    if (event_id < MaxNvEvents) {
        IPC::ResponseBuilder rb{ctx, 3, 1};
        rb.Push(ResultSuccess);
        auto& event = nvdrv->GetEvent(event_id);
        event.Clear();
        rb.PushCopyObjects(event);
        rb.PushEnum(NvResult::Success);
    } else {
        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.PushEnum(NvResult::BadParameter);
    }
}

void NVDRV::SetAruid(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    pid = rp.Pop<u64>();
    LOG_WARNING(Service_NVDRV, "(STUBBED) called, pid=0x{:X}", pid);

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.PushEnum(NvResult::Success);
}

void NVDRV::SetGraphicsFirmwareMemoryMarginEnabled(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service_NVDRV, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

void NVDRV::GetStatus(Kernel::HLERequestContext& ctx) {
    LOG_WARNING(Service_NVDRV, "(STUBBED) called");

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.PushEnum(NvResult::Success);
}

void NVDRV::DumpGraphicsMemoryInfo(Kernel::HLERequestContext& ctx) {
    // According to SwitchBrew, this has no inputs and no outputs, so effectively does nothing on
    // retail hardware.
    LOG_DEBUG(Service_NVDRV, "called");

    IPC::ResponseBuilder rb{ctx, 2};
    rb.Push(ResultSuccess);
}

NVDRV::NVDRV(Core::System& system_, std::shared_ptr<Module> nvdrv_, const char* name)
    : ServiceFramework{system_, name, ServiceThreadType::CreateNew}, nvdrv{std::move(nvdrv_)} {
    static const FunctionInfo functions[] = {
        {0, &NVDRV::Open, "Open"},
        {1, &NVDRV::Ioctl1, "Ioctl"},
        {2, &NVDRV::Close, "Close"},
        {3, &NVDRV::Initialize, "Initialize"},
        {4, &NVDRV::QueryEvent, "QueryEvent"},
        {5, nullptr, "MapSharedMem"},
        {6, &NVDRV::GetStatus, "GetStatus"},
        {7, nullptr, "SetAruidForTest"},
        {8, &NVDRV::SetAruid, "SetAruid"},
        {9, &NVDRV::DumpGraphicsMemoryInfo, "DumpGraphicsMemoryInfo"},
        {10, nullptr, "InitializeDevtools"},
        {11, &NVDRV::Ioctl2, "Ioctl2"},
        {12, &NVDRV::Ioctl3, "Ioctl3"},
        {13, &NVDRV::SetGraphicsFirmwareMemoryMarginEnabled,
         "SetGraphicsFirmwareMemoryMarginEnabled"},
    };
    RegisterHandlers(functions);
}

NVDRV::~NVDRV() = default;

} // namespace Service::Nvidia
