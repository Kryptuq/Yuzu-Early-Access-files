// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <cstring>
#include <vector>

#include "audio_core/out/audio_out_system.h"
#include "audio_core/renderer/audio_device.h"
#include "common/common_funcs.h"
#include "common/logging/log.h"
#include "common/string_util.h"
#include "common/swap.h"
#include "core/core.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/k_event.h"
#include "core/hle/service/audio/audout_u.h"
#include "core/hle/service/audio/errors.h"
#include "core/memory.h"

namespace Service::Audio {
using namespace AudioCore::AudioOut;

class IAudioOut final : public ServiceFramework<IAudioOut> {
public:
    explicit IAudioOut(Core::System& system_, AudioCore::AudioOut::Manager& manager,
                       size_t session_id, std::string& device_name,
                       const AudioOutParameter& in_params, u32 handle, u64 applet_resource_user_id)
        : ServiceFramework{system_, "IAudioOut", ServiceThreadType::CreateNew},
          service_context{system_, "IAudioOut"}, event{service_context.CreateEvent(
                                                     "AudioOutEvent")},
          impl{std::make_shared<AudioCore::AudioOut::Out>(system_, manager, event, session_id)} {

        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &IAudioOut::GetAudioOutState, "GetAudioOutState"},
            {1, &IAudioOut::Start, "Start"},
            {2, &IAudioOut::Stop, "Stop"},
            {3, &IAudioOut::AppendAudioOutBuffer, "AppendAudioOutBuffer"},
            {4, &IAudioOut::RegisterBufferEvent, "RegisterBufferEvent"},
            {5, &IAudioOut::GetReleasedAudioOutBuffers, "GetReleasedAudioOutBuffers"},
            {6, &IAudioOut::ContainsAudioOutBuffer, "ContainsAudioOutBuffer"},
            {7, &IAudioOut::AppendAudioOutBuffer, "AppendAudioOutBufferAuto"},
            {8, &IAudioOut::GetReleasedAudioOutBuffers, "GetReleasedAudioOutBuffersAuto"},
            {9, &IAudioOut::GetAudioOutBufferCount, "GetAudioOutBufferCount"},
            {10, &IAudioOut::GetAudioOutPlayedSampleCount, "GetAudioOutPlayedSampleCount"},
            {11, &IAudioOut::FlushAudioOutBuffers, "FlushAudioOutBuffers"},
            {12, &IAudioOut::SetAudioOutVolume, "SetAudioOutVolume"},
            {13, &IAudioOut::GetAudioOutVolume, "GetAudioOutVolume"},
        };
        // clang-format on
        RegisterHandlers(functions);

        if (impl->GetSystem()
                .Initialize(device_name, in_params, handle, applet_resource_user_id)
                .IsError()) {
            LOG_ERROR(Service_Audio, "Failed to initialize the AudioOut System!");
        }
    }

    ~IAudioOut() override {
        impl->Free();
        service_context.CloseEvent(event);
    }

    [[nodiscard]] std::shared_ptr<AudioCore::AudioOut::Out> GetImpl() {
        return impl;
    }

private:
    void GetAudioOutState(Kernel::HLERequestContext& ctx) {
        const auto state = static_cast<u32>(impl->GetState());

        LOG_DEBUG(Service_Audio, "called. State={}", state);

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push(state);
    }

    void Start(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_Audio, "called");

        auto result = impl->StartSystem();

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(result);
    }

    void Stop(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_Audio, "called");

        auto result = impl->StopSystem();

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(result);
    }

    void AppendAudioOutBuffer(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        u64 tag = rp.PopRaw<u64>();

        const auto in_buffer_size{ctx.GetReadBufferSize()};
        if (in_buffer_size < sizeof(AudioOutBuffer)) {
            LOG_ERROR(Service_Audio, "Input buffer is too small for an AudioOutBuffer!");
        }

        const auto& in_buffer = ctx.ReadBuffer();
        AudioOutBuffer buffer{};
        std::memcpy(&buffer, in_buffer.data(), sizeof(AudioOutBuffer));

        [[maybe_unused]] auto sessionid{impl->GetSystem().GetSessionId()};
        LOG_TRACE(Service_Audio, "called. Session {} Appending buffer {:08X}", sessionid, tag);

        auto result = impl->AppendBuffer(buffer, tag);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(result);
    }

    void RegisterBufferEvent(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_Audio, "called");

        auto& buffer_event = impl->GetBufferEvent();

        IPC::ResponseBuilder rb{ctx, 2, 1};
        rb.Push(ResultSuccess);
        rb.PushCopyObjects(buffer_event);
    }

    void GetReleasedAudioOutBuffers(Kernel::HLERequestContext& ctx) {
        auto write_buffer_size = ctx.GetWriteBufferSize() / sizeof(u64);
        std::vector<u64> released_buffers(write_buffer_size, 0);

        auto count = impl->GetReleasedBuffers(released_buffers);

        [[maybe_unused]] std::string tags{};
        for (u32 i = 0; i < count; i++) {
            tags += fmt::format("{:08X}, ", released_buffers[i]);
        }
        [[maybe_unused]] auto sessionid{impl->GetSystem().GetSessionId()};
        LOG_TRACE(Service_Audio, "called. Session {} released {} buffers: {}", sessionid, count,
                  tags);

        ctx.WriteBuffer(released_buffers);
        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push(count);
    }

    void ContainsAudioOutBuffer(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};

        const u64 tag{rp.Pop<u64>()};
        const auto buffer_queued{impl->ContainsAudioBuffer(tag)};

        LOG_DEBUG(Service_Audio, "called. Is buffer {:08X} registered? {}", tag, buffer_queued);

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push(buffer_queued);
    }

    void GetAudioOutBufferCount(Kernel::HLERequestContext& ctx) {
        const auto buffer_count = impl->GetBufferCount();

        LOG_DEBUG(Service_Audio, "called. Buffer count={}", buffer_count);

        IPC::ResponseBuilder rb{ctx, 3};

        rb.Push(ResultSuccess);
        rb.Push(buffer_count);
    }

    void GetAudioOutPlayedSampleCount(Kernel::HLERequestContext& ctx) {
        const auto samples_played = impl->GetPlayedSampleCount();

        LOG_DEBUG(Service_Audio, "called. Played samples={}", samples_played);

        IPC::ResponseBuilder rb{ctx, 4};

        rb.Push(ResultSuccess);
        rb.Push(samples_played);
    }

    void FlushAudioOutBuffers(Kernel::HLERequestContext& ctx) {
        bool flushed{impl->FlushAudioOutBuffers()};

        LOG_DEBUG(Service_Audio, "called. Were any buffers flushed? {}", flushed);

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push(flushed);
    }

    void SetAudioOutVolume(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const auto volume = rp.Pop<f32>();

        LOG_DEBUG(Service_Audio, "called. Volume={}", volume);

        impl->SetVolume(volume);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void GetAudioOutVolume(Kernel::HLERequestContext& ctx) {
        const auto volume = impl->GetVolume();

        LOG_DEBUG(Service_Audio, "called. Volume={}", volume);

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push(volume);
    }

    KernelHelpers::ServiceContext service_context;
    Kernel::KEvent* event;
    std::shared_ptr<AudioCore::AudioOut::Out> impl;
};

AudOutU::AudOutU(Core::System& system_)
    : ServiceFramework{system_, "audout:u", ServiceThreadType::CreateNew},
      service_context{system_, "AudOutU"}, impl{std::make_unique<AudioCore::AudioOut::Manager>(
                                               system_)} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, &AudOutU::ListAudioOuts, "ListAudioOuts"},
        {1, &AudOutU::OpenAudioOut, "OpenAudioOut"},
        {2, &AudOutU::ListAudioOuts, "ListAudioOutsAuto"},
        {3, &AudOutU::OpenAudioOut, "OpenAudioOutAuto"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

AudOutU::~AudOutU() = default;

void AudOutU::ListAudioOuts(Kernel::HLERequestContext& ctx) {
    using namespace AudioCore::AudioRenderer;

    std::scoped_lock l{impl->mutex};

    const auto write_count =
        static_cast<u32>(ctx.GetWriteBufferSize() / sizeof(AudioDevice::AudioDeviceName));
    std::vector<AudioDevice::AudioDeviceName> device_names{};
    if (write_count > 0) {
        device_names.emplace_back("DeviceOut");
        LOG_DEBUG(Service_Audio, "called. \nName=DeviceOut");
    } else {
        LOG_DEBUG(Service_Audio, "called. Empty buffer passed in.");
    }

    ctx.WriteBuffer(device_names);

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.Push<u32>(static_cast<u32>(device_names.size()));
}

void AudOutU::OpenAudioOut(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    auto in_params{rp.PopRaw<AudioOutParameter>()};
    auto applet_resource_user_id{rp.PopRaw<u64>()};
    const auto device_name_data{ctx.ReadBuffer()};
    auto device_name = Common::StringFromBuffer(device_name_data);
    auto handle{ctx.GetCopyHandle(0)};

    auto link{impl->LinkToManager()};
    if (link.IsError()) {
        LOG_ERROR(Service_Audio, "Failed to link Audio Out to Audio Manager");
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(link);
        return;
    }

    size_t new_session_id{};
    auto result{impl->AcquireSessionId(new_session_id)};
    if (result.IsError()) {
        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(result);
        return;
    }

    LOG_DEBUG(Service_Audio, "Opening new AudioOut, sessionid={}, free sessions={}", new_session_id,
              impl->num_free_sessions);

    auto audio_out = std::make_shared<IAudioOut>(system, *impl, new_session_id, device_name,
                                                 in_params, handle, applet_resource_user_id);

    impl->sessions[new_session_id] = audio_out->GetImpl();
    impl->applet_resource_user_ids[new_session_id] = applet_resource_user_id;

    auto& out_system = impl->sessions[new_session_id]->GetSystem();
    AudioOutParameterInternal out_params{.sample_rate = out_system.GetSampleRate(),
                                         .channel_count = out_system.GetChannelCount(),
                                         .sample_format =
                                             static_cast<u32>(out_system.GetSampleFormat()),
                                         .state = static_cast<u32>(out_system.GetState())};

    IPC::ResponseBuilder rb{ctx, 6, 0, 1};

    ctx.WriteBuffer(out_system.GetName());

    rb.Push(ResultSuccess);
    rb.PushRaw<AudioOutParameterInternal>(out_params);
    rb.PushIpcInterface<IAudioOut>(audio_out);
}

} // namespace Service::Audio
