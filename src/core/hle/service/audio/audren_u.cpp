// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <memory>

#include "audio_core/audio_core.h"
#include "audio_core/common/audio_renderer_parameter.h"
#include "audio_core/common/feature_support.h"
#include "audio_core/renderer/audio_device.h"
#include "audio_core/renderer/audio_renderer.h"
#include "audio_core/renderer/voice/voice_info.h"
#include "common/alignment.h"
#include "common/bit_util.h"
#include "common/common_funcs.h"
#include "common/logging/log.h"
#include "common/polyfill_ranges.h"
#include "common/string_util.h"
#include "core/core.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/k_event.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/k_transfer_memory.h"
#include "core/hle/service/audio/audren_u.h"
#include "core/hle/service/audio/errors.h"
#include "core/memory.h"

using namespace AudioCore::AudioRenderer;

namespace Service::Audio {

class IAudioRenderer final : public ServiceFramework<IAudioRenderer> {
public:
    explicit IAudioRenderer(Core::System& system_, Manager& manager_,
                            AudioCore::AudioRendererParameterInternal& params,
                            Kernel::KTransferMemory* transfer_memory, u64 transfer_memory_size,
                            u32 process_handle, u64 applet_resource_user_id, s32 session_id)
        : ServiceFramework{system_, "IAudioRenderer"}, service_context{system_, "IAudioRenderer"},
          rendered_event{service_context.CreateEvent("IAudioRendererEvent")}, manager{manager_},
          impl{std::make_unique<Renderer>(system_, manager, rendered_event)} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &IAudioRenderer::GetSampleRate, "GetSampleRate"},
            {1, &IAudioRenderer::GetSampleCount, "GetSampleCount"},
            {2, &IAudioRenderer::GetMixBufferCount, "GetMixBufferCount"},
            {3, &IAudioRenderer::GetState, "GetState"},
            {4, &IAudioRenderer::RequestUpdate, "RequestUpdate"},
            {5, &IAudioRenderer::Start, "Start"},
            {6, &IAudioRenderer::Stop, "Stop"},
            {7, &IAudioRenderer::QuerySystemEvent, "QuerySystemEvent"},
            {8, &IAudioRenderer::SetRenderingTimeLimit, "SetRenderingTimeLimit"},
            {9, &IAudioRenderer::GetRenderingTimeLimit, "GetRenderingTimeLimit"},
            {10, &IAudioRenderer::RequestUpdate, "RequestUpdateAuto"},
            {11, nullptr, "ExecuteAudioRendererRendering"},
            {12, &IAudioRenderer::SetVoiceDropParameter, "SetVoiceDropParameter"},
            {13, &IAudioRenderer::GetVoiceDropParameter, "GetVoiceDropParameter"},
        };
        // clang-format on
        RegisterHandlers(functions);

        impl->Initialize(params, transfer_memory, transfer_memory_size, process_handle,
                         applet_resource_user_id, session_id);
    }

    ~IAudioRenderer() override {
        impl->Finalize();
        service_context.CloseEvent(rendered_event);
    }

private:
    void GetSampleRate(Kernel::HLERequestContext& ctx) {
        const auto sample_rate{impl->GetSystem().GetSampleRate()};

        LOG_DEBUG(Service_Audio, "called. Sample rate {}", sample_rate);

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push(sample_rate);
    }

    void GetSampleCount(Kernel::HLERequestContext& ctx) {
        const auto sample_count{impl->GetSystem().GetSampleCount()};

        LOG_DEBUG(Service_Audio, "called. Sample count {}", sample_count);

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push(sample_count);
    }

    void GetState(Kernel::HLERequestContext& ctx) {
        const u32 state{!impl->GetSystem().IsActive()};

        LOG_DEBUG(Service_Audio, "called, state {}", state);

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push(state);
    }

    void GetMixBufferCount(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_Audio, "called");

        const auto buffer_count{impl->GetSystem().GetMixBufferCount()};

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push(buffer_count);
    }

    void RequestUpdate(Kernel::HLERequestContext& ctx) {
        LOG_TRACE(Service_Audio, "called");

        std::vector<u8> input{ctx.ReadBuffer(0)};

        // These buffers are written manually to avoid an issue with WriteBuffer throwing errors for
        // checking size 0. Performance size is 0 for most games.

        std::vector<u8> output{};
        std::vector<u8> performance{};
        auto is_buffer_b{ctx.BufferDescriptorB()[0].Size() != 0};
        if (is_buffer_b) {
            const auto buffersB{ctx.BufferDescriptorB()};
            output.resize(buffersB[0].Size(), 0);
            performance.resize(buffersB[1].Size(), 0);
        } else {
            const auto buffersC{ctx.BufferDescriptorC()};
            output.resize(buffersC[0].Size(), 0);
            performance.resize(buffersC[1].Size(), 0);
        }

        auto result = impl->RequestUpdate(input, performance, output);

        if (result.IsSuccess()) {
            if (is_buffer_b) {
                ctx.WriteBufferB(output.data(), output.size(), 0);
                ctx.WriteBufferB(performance.data(), performance.size(), 1);
            } else {
                ctx.WriteBufferC(output.data(), output.size(), 0);
                ctx.WriteBufferC(performance.data(), performance.size(), 1);
            }
        } else {
            LOG_ERROR(Service_Audio, "RequestUpdate failed error 0x{:02X}!", result.description);
        }

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(result);
    }

    void Start(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_Audio, "called");

        impl->Start();

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void Stop(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_Audio, "called");

        impl->Stop();

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void QuerySystemEvent(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_Audio, "called");

        if (impl->GetSystem().GetExecutionMode() == AudioCore::ExecutionMode::Manual) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ERR_NOT_SUPPORTED);
            return;
        }

        IPC::ResponseBuilder rb{ctx, 2, 1};
        rb.Push(ResultSuccess);
        rb.PushCopyObjects(rendered_event->GetReadableEvent());
    }

    void SetRenderingTimeLimit(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_Audio, "called");

        IPC::RequestParser rp{ctx};
        auto limit = rp.PopRaw<u32>();

        auto& system_ = impl->GetSystem();
        system_.SetRenderingTimeLimit(limit);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void GetRenderingTimeLimit(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_Audio, "called");

        auto& system_ = impl->GetSystem();
        auto time = system_.GetRenderingTimeLimit();

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push(time);
    }

    void ExecuteAudioRendererRendering(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_Audio, "called");
    }

    void SetVoiceDropParameter(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_Audio, "called");

        IPC::RequestParser rp{ctx};
        auto voice_drop_param{rp.Pop<f32>()};

        auto& system_ = impl->GetSystem();
        system_.SetVoiceDropParameter(voice_drop_param);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void GetVoiceDropParameter(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_Audio, "called");

        auto& system_ = impl->GetSystem();
        auto voice_drop_param{system_.GetVoiceDropParameter()};

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push(voice_drop_param);
    }

    KernelHelpers::ServiceContext service_context;
    Kernel::KEvent* rendered_event;
    Manager& manager;
    std::unique_ptr<Renderer> impl;
};

class IAudioDevice final : public ServiceFramework<IAudioDevice> {

public:
    explicit IAudioDevice(Core::System& system_, u64 applet_resource_user_id, u32 revision,
                          u32 device_num)
        : ServiceFramework{system_, "IAudioDevice"}, service_context{system_, "IAudioDevice"},
          impl{std::make_unique<AudioDevice>(system_, applet_resource_user_id, revision)},
          event{service_context.CreateEvent(fmt::format("IAudioDeviceEvent-{}", device_num))} {
        static const FunctionInfo functions[] = {
            {0, &IAudioDevice::ListAudioDeviceName, "ListAudioDeviceName"},
            {1, &IAudioDevice::SetAudioDeviceOutputVolume, "SetAudioDeviceOutputVolume"},
            {2, &IAudioDevice::GetAudioDeviceOutputVolume, "GetAudioDeviceOutputVolume"},
            {3, &IAudioDevice::GetActiveAudioDeviceName, "GetActiveAudioDeviceName"},
            {4, &IAudioDevice::QueryAudioDeviceSystemEvent, "QueryAudioDeviceSystemEvent"},
            {5, &IAudioDevice::GetActiveChannelCount, "GetActiveChannelCount"},
            {6, &IAudioDevice::ListAudioDeviceName, "ListAudioDeviceNameAuto"},
            {7, &IAudioDevice::SetAudioDeviceOutputVolume, "SetAudioDeviceOutputVolumeAuto"},
            {8, &IAudioDevice::GetAudioDeviceOutputVolume, "GetAudioDeviceOutputVolumeAuto"},
            {10, &IAudioDevice::GetActiveAudioDeviceName, "GetActiveAudioDeviceNameAuto"},
            {11, &IAudioDevice::QueryAudioDeviceInputEvent, "QueryAudioDeviceInputEvent"},
            {12, &IAudioDevice::QueryAudioDeviceOutputEvent, "QueryAudioDeviceOutputEvent"},
            {13, &IAudioDevice::GetActiveAudioDeviceName, "GetActiveAudioOutputDeviceName"},
            {14, &IAudioDevice::ListAudioOutputDeviceName, "ListAudioOutputDeviceName"},
        };
        RegisterHandlers(functions);

        event->Signal();
    }

    ~IAudioDevice() override {
        service_context.CloseEvent(event);
    }

private:
    void ListAudioDeviceName(Kernel::HLERequestContext& ctx) {
        const size_t in_count = ctx.GetWriteBufferSize() / sizeof(AudioDevice::AudioDeviceName);

        std::vector<AudioDevice::AudioDeviceName> out_names{};

        const u32 out_count = impl->ListAudioDeviceName(out_names, in_count);

        std::string out{};
        for (u32 i = 0; i < out_count; i++) {
            std::string a{};
            u32 j = 0;
            while (out_names[i].name[j] != '\0') {
                a += out_names[i].name[j];
                j++;
            }
            out += "\n\t" + a;
        }

        LOG_DEBUG(Service_Audio, "called.\nNames={}", out);

        IPC::ResponseBuilder rb{ctx, 3};

        ctx.WriteBuffer(out_names);

        rb.Push(ResultSuccess);
        rb.Push(out_count);
    }

    void SetAudioDeviceOutputVolume(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const f32 volume = rp.Pop<f32>();

        const auto device_name_buffer = ctx.ReadBuffer();
        const std::string name = Common::StringFromBuffer(device_name_buffer);

        LOG_DEBUG(Service_Audio, "called. name={}, volume={}", name, volume);

        if (name == "AudioTvOutput") {
            impl->SetDeviceVolumes(volume);
        }

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void GetAudioDeviceOutputVolume(Kernel::HLERequestContext& ctx) {
        const auto device_name_buffer = ctx.ReadBuffer();
        const std::string name = Common::StringFromBuffer(device_name_buffer);

        LOG_DEBUG(Service_Audio, "called. Name={}", name);

        f32 volume{1.0f};
        if (name == "AudioTvOutput") {
            volume = impl->GetDeviceVolume(name);
        }

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push(volume);
    }

    void GetActiveAudioDeviceName(Kernel::HLERequestContext& ctx) {
        const auto write_size = ctx.GetWriteBufferSize() / sizeof(char);
        std::string out_name{"AudioTvOutput"};

        LOG_DEBUG(Service_Audio, "(STUBBED) called. Name={}", out_name);

        out_name.resize(write_size);

        ctx.WriteBuffer(out_name);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void QueryAudioDeviceSystemEvent(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_Audio, "(STUBBED) called");

        event->Signal();

        IPC::ResponseBuilder rb{ctx, 2, 1};
        rb.Push(ResultSuccess);
        rb.PushCopyObjects(event->GetReadableEvent());
    }

    void GetActiveChannelCount(Kernel::HLERequestContext& ctx) {
        const auto& sink{system.AudioCore().GetOutputSink()};
        u32 channel_count{sink.GetDeviceChannels()};

        LOG_DEBUG(Service_Audio, "(STUBBED) called. Channels={}", channel_count);

        IPC::ResponseBuilder rb{ctx, 3};

        rb.Push(ResultSuccess);
        rb.Push<u32>(channel_count);
    }

    void QueryAudioDeviceInputEvent(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_Audio, "(STUBBED) called");

        IPC::ResponseBuilder rb{ctx, 2, 1};
        rb.Push(ResultSuccess);
        rb.PushCopyObjects(event->GetReadableEvent());
    }

    void QueryAudioDeviceOutputEvent(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_Audio, "called");

        IPC::ResponseBuilder rb{ctx, 2, 1};
        rb.Push(ResultSuccess);
        rb.PushCopyObjects(event->GetReadableEvent());
    }

    void ListAudioOutputDeviceName(Kernel::HLERequestContext& ctx) {
        const size_t in_count = ctx.GetWriteBufferSize() / sizeof(AudioDevice::AudioDeviceName);

        std::vector<AudioDevice::AudioDeviceName> out_names{};

        const u32 out_count = impl->ListAudioOutputDeviceName(out_names, in_count);

        std::string out{};
        for (u32 i = 0; i < out_count; i++) {
            std::string a{};
            u32 j = 0;
            while (out_names[i].name[j] != '\0') {
                a += out_names[i].name[j];
                j++;
            }
            out += "\n\t" + a;
        }

        LOG_DEBUG(Service_Audio, "called.\nNames={}", out);

        IPC::ResponseBuilder rb{ctx, 3};

        ctx.WriteBuffer(out_names);

        rb.Push(ResultSuccess);
        rb.Push(out_count);
    }

    KernelHelpers::ServiceContext service_context;
    std::unique_ptr<AudioDevice> impl;
    Kernel::KEvent* event;
};

AudRenU::AudRenU(Core::System& system_)
    : ServiceFramework{system_, "audren:u"},
      service_context{system_, "audren:u"}, impl{std::make_unique<Manager>(system_)} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, &AudRenU::OpenAudioRenderer, "OpenAudioRenderer"},
        {1, &AudRenU::GetWorkBufferSize, "GetWorkBufferSize"},
        {2, &AudRenU::GetAudioDeviceService, "GetAudioDeviceService"},
        {3, nullptr, "OpenAudioRendererForManualExecution"},
        {4, &AudRenU::GetAudioDeviceServiceWithRevisionInfo, "GetAudioDeviceServiceWithRevisionInfo"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

AudRenU::~AudRenU() = default;

void AudRenU::OpenAudioRenderer(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};

    AudioCore::AudioRendererParameterInternal params;
    rp.PopRaw<AudioCore::AudioRendererParameterInternal>(params);
    auto transfer_memory_handle = ctx.GetCopyHandle(0);
    auto process_handle = ctx.GetCopyHandle(1);
    auto transfer_memory_size = rp.Pop<u64>();
    auto applet_resource_user_id = rp.Pop<u64>();

    if (impl->GetSessionCount() + 1 > AudioCore::MaxRendererSessions) {
        LOG_ERROR(Service_Audio, "Too many AudioRenderer sessions open!");
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ERR_MAXIMUM_SESSIONS_REACHED);
        return;
    }

    const auto& handle_table{system.CurrentProcess()->GetHandleTable()};
    auto process{handle_table.GetObject<Kernel::KProcess>(process_handle)};
    auto transfer_memory{
        process->GetHandleTable().GetObject<Kernel::KTransferMemory>(transfer_memory_handle)};

    const auto session_id{impl->GetSessionId()};
    if (session_id == -1) {
        LOG_ERROR(Service_Audio, "Tried to open a session that's already in use!");
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ERR_MAXIMUM_SESSIONS_REACHED);
        return;
    }

    LOG_DEBUG(Service_Audio, "Opened new AudioRenderer session {} sessions open {}", session_id,
              impl->GetSessionCount());

    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(ResultSuccess);
    rb.PushIpcInterface<IAudioRenderer>(system, *impl, params, transfer_memory.GetPointerUnsafe(),
                                        transfer_memory_size, process_handle,
                                        applet_resource_user_id, session_id);
}

void AudRenU::GetWorkBufferSize(Kernel::HLERequestContext& ctx) {
    AudioCore::AudioRendererParameterInternal params;

    IPC::RequestParser rp{ctx};
    rp.PopRaw<AudioCore::AudioRendererParameterInternal>(params);

    u64 size{0};
    auto result = impl->GetWorkBufferSize(params, size);

    std::string output_info{};
    output_info += fmt::format("\tRevision {}", AudioCore::GetRevisionNum(params.revision));
    output_info +=
        fmt::format("\n\tSample Rate {}, Sample Count {}", params.sample_rate, params.sample_count);
    output_info += fmt::format("\n\tExecution Mode {}, Voice Drop Enabled {}",
                               static_cast<u32>(params.execution_mode), params.voice_drop_enabled);
    output_info += fmt::format(
        "\n\tSizes: Effects {:04X}, Mixes {:04X}, Sinks {:04X}, Submixes {:04X}, Splitter Infos "
        "{:04X}, Splitter Destinations {:04X}, Voices {:04X}, Performance Frames {:04X} External "
        "Context {:04X}",
        params.effects, params.mixes, params.sinks, params.sub_mixes, params.splitter_infos,
        params.splitter_destinations, params.voices, params.perf_frames,
        params.external_context_size);

    LOG_DEBUG(Service_Audio, "called.\nInput params:\n{}\nOutput params:\n\tWorkbuffer size {:08X}",
              output_info, size);

    IPC::ResponseBuilder rb{ctx, 4};
    rb.Push(result);
    rb.Push<u64>(size);
}

void AudRenU::GetAudioDeviceService(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};

    const auto applet_resource_user_id = rp.Pop<u64>();

    LOG_DEBUG(Service_Audio, "called. Applet resource id {}", applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2, 0, 1};

    rb.Push(ResultSuccess);
    rb.PushIpcInterface<IAudioDevice>(system, applet_resource_user_id,
                                      ::Common::MakeMagic('R', 'E', 'V', '1'), num_audio_devices++);
}

void AudRenU::OpenAudioRendererForManualExecution(Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service_Audio, "called");
}

void AudRenU::GetAudioDeviceServiceWithRevisionInfo(Kernel::HLERequestContext& ctx) {
    struct Parameters {
        u32 revision;
        u64 applet_resource_user_id;
    };

    IPC::RequestParser rp{ctx};

    const auto [revision, applet_resource_user_id] = rp.PopRaw<Parameters>();

    LOG_DEBUG(Service_Audio, "called. Revision {} Applet resource id {}",
              AudioCore::GetRevisionNum(revision), applet_resource_user_id);

    IPC::ResponseBuilder rb{ctx, 2, 0, 1};

    rb.Push(ResultSuccess);
    rb.PushIpcInterface<IAudioDevice>(system, applet_resource_user_id, revision,
                                      num_audio_devices++);
}

} // namespace Service::Audio
