// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/in/audio_in_system.h"
#include "audio_core/renderer/audio_device.h"
#include "common/common_funcs.h"
#include "common/logging/log.h"
#include "common/string_util.h"
#include "core/core.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/k_event.h"
#include "core/hle/service/audio/audin_u.h"

namespace Service::Audio {
using namespace AudioCore::AudioIn;

class IAudioIn final : public ServiceFramework<IAudioIn> {
public:
    explicit IAudioIn(Core::System& system_, Manager& manager, size_t session_id,
                      const std::string& device_name, const AudioInParameter& in_params, u32 handle,
                      u64 applet_resource_user_id)
        : ServiceFramework{system_, "IAudioIn"},
          service_context{system_, "IAudioIn"}, event{service_context.CreateEvent("AudioInEvent")},
          impl{std::make_shared<In>(system_, manager, event, session_id)} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &IAudioIn::GetAudioInState, "GetAudioInState"},
            {1, &IAudioIn::Start, "Start"},
            {2, &IAudioIn::Stop, "Stop"},
            {3, &IAudioIn::AppendAudioInBuffer, "AppendAudioInBuffer"},
            {4, &IAudioIn::RegisterBufferEvent, "RegisterBufferEvent"},
            {5, &IAudioIn::GetReleasedAudioInBuffer, "GetReleasedAudioInBuffer"},
            {6, &IAudioIn::ContainsAudioInBuffer, "ContainsAudioInBuffer"},
            {7, &IAudioIn::AppendAudioInBuffer, "AppendUacInBuffer"},
            {8, &IAudioIn::AppendAudioInBuffer, "AppendAudioInBufferAuto"},
            {9, &IAudioIn::GetReleasedAudioInBuffer, "GetReleasedAudioInBuffersAuto"},
            {10, &IAudioIn::AppendAudioInBuffer, "AppendUacInBufferAuto"},
            {11, &IAudioIn::GetAudioInBufferCount, "GetAudioInBufferCount"},
            {12, &IAudioIn::SetDeviceGain, "SetDeviceGain"},
            {13, &IAudioIn::GetDeviceGain, "GetDeviceGain"},
            {14, &IAudioIn::FlushAudioInBuffers, "FlushAudioInBuffers"},
        };
        // clang-format on

        RegisterHandlers(functions);

        if (impl->GetSystem()
                .Initialize(device_name, in_params, handle, applet_resource_user_id)
                .IsError()) {
            LOG_ERROR(Service_Audio, "Failed to initialize the AudioIn System!");
        }
    }

    ~IAudioIn() override {
        impl->Free();
        service_context.CloseEvent(event);
    }

    [[nodiscard]] std::shared_ptr<In> GetImpl() {
        return impl;
    }

private:
    void GetAudioInState(Kernel::HLERequestContext& ctx) {
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

    void AppendAudioInBuffer(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        u64 tag = rp.PopRaw<u64>();

        const auto in_buffer_size{ctx.GetReadBufferSize()};
        if (in_buffer_size < sizeof(AudioInBuffer)) {
            LOG_ERROR(Service_Audio, "Input buffer is too small for an AudioInBuffer!");
        }

        const auto& in_buffer = ctx.ReadBuffer();
        AudioInBuffer buffer{};
        std::memcpy(&buffer, in_buffer.data(), sizeof(AudioInBuffer));

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

    void GetReleasedAudioInBuffer(Kernel::HLERequestContext& ctx) {
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

    void ContainsAudioInBuffer(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};

        const u64 tag{rp.Pop<u64>()};
        const auto buffer_queued{impl->ContainsAudioBuffer(tag)};

        LOG_DEBUG(Service_Audio, "called. Is buffer {:08X} registered? {}", tag, buffer_queued);

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push(buffer_queued);
    }

    void GetAudioInBufferCount(Kernel::HLERequestContext& ctx) {
        const auto buffer_count = impl->GetBufferCount();

        LOG_DEBUG(Service_Audio, "called. Buffer count={}", buffer_count);

        IPC::ResponseBuilder rb{ctx, 3};

        rb.Push(ResultSuccess);
        rb.Push(buffer_count);
    }

    void SetDeviceGain(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};

        const auto volume{rp.Pop<f32>()};
        LOG_DEBUG(Service_Audio, "called. Gain {}", volume);

        impl->SetVolume(volume);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void GetDeviceGain(Kernel::HLERequestContext& ctx) {
        auto volume{impl->GetVolume()};

        LOG_DEBUG(Service_Audio, "called. Gain {}", volume);

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push(volume);
    }

    void FlushAudioInBuffers(Kernel::HLERequestContext& ctx) {
        bool flushed{impl->FlushAudioInBuffers()};

        LOG_DEBUG(Service_Audio, "called. Were any buffers flushed? {}", flushed);

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push(flushed);
    }

    KernelHelpers::ServiceContext service_context;
    Kernel::KEvent* event;
    std::shared_ptr<AudioCore::AudioIn::In> impl;
};

AudInU::AudInU(Core::System& system_)
    : ServiceFramework{system_, "audin:u"}, service_context{system_, "AudInU"},
      impl{std::make_unique<AudioCore::AudioIn::Manager>(system_)} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, &AudInU::ListAudioIns, "ListAudioIns"},
        {1, &AudInU::OpenAudioIn, "OpenAudioIn"},
        {2, &AudInU::ListAudioIns, "ListAudioInsAuto"},
        {3, &AudInU::OpenAudioIn, "OpenAudioInAuto"},
        {4, &AudInU::ListAudioInsAutoFiltered, "ListAudioInsAutoFiltered"},
        {5, &AudInU::OpenAudioInProtocolSpecified, "OpenAudioInProtocolSpecified"},
    };
    // clang-format on

    RegisterHandlers(functions);
}

AudInU::~AudInU() = default;

void AudInU::ListAudioIns(Kernel::HLERequestContext& ctx) {
    using namespace AudioCore::AudioRenderer;

    LOG_DEBUG(Service_Audio, "called");

    const auto write_count =
        static_cast<u32>(ctx.GetWriteBufferSize() / sizeof(AudioDevice::AudioDeviceName));
    std::vector<AudioDevice::AudioDeviceName> device_names{};

    u32 out_count{0};
    if (write_count > 0) {
        out_count = impl->GetDeviceNames(device_names, write_count, false);
        ctx.WriteBuffer(device_names);
    }

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.Push(out_count);
}

void AudInU::ListAudioInsAutoFiltered(Kernel::HLERequestContext& ctx) {
    using namespace AudioCore::AudioRenderer;

    LOG_DEBUG(Service_Audio, "called");

    const auto write_count =
        static_cast<u32>(ctx.GetWriteBufferSize() / sizeof(AudioDevice::AudioDeviceName));
    std::vector<AudioDevice::AudioDeviceName> device_names{};

    u32 out_count{0};
    if (write_count > 0) {
        out_count = impl->GetDeviceNames(device_names, write_count, true);
        ctx.WriteBuffer(device_names);
    }

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.Push(out_count);
}

void AudInU::OpenAudioIn(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    auto in_params{rp.PopRaw<AudioInParameter>()};
    auto applet_resource_user_id{rp.PopRaw<u64>()};
    const auto device_name_data{ctx.ReadBuffer()};
    auto device_name = Common::StringFromBuffer(device_name_data);
    auto handle{ctx.GetCopyHandle(0)};

    std::scoped_lock l{impl->mutex};
    auto link{impl->LinkToManager()};
    if (link.IsError()) {
        LOG_ERROR(Service_Audio, "Failed to link Audio In to Audio Manager");
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

    LOG_DEBUG(Service_Audio, "Opening new AudioIn, sessionid={}, free sessions={}", new_session_id,
              impl->num_free_sessions);

    auto audio_in = std::make_shared<IAudioIn>(system, *impl, new_session_id, device_name,
                                               in_params, handle, applet_resource_user_id);
    impl->sessions[new_session_id] = audio_in->GetImpl();
    impl->applet_resource_user_ids[new_session_id] = applet_resource_user_id;

    auto& out_system = impl->sessions[new_session_id]->GetSystem();
    AudioInParameterInternal out_params{.sample_rate = out_system.GetSampleRate(),
                                        .channel_count = out_system.GetChannelCount(),
                                        .sample_format =
                                            static_cast<u32>(out_system.GetSampleFormat()),
                                        .state = static_cast<u32>(out_system.GetState())};

    IPC::ResponseBuilder rb{ctx, 6, 0, 1};

    std::string out_name{out_system.GetName()};
    ctx.WriteBuffer(out_name);

    rb.Push(ResultSuccess);
    rb.PushRaw<AudioInParameterInternal>(out_params);
    rb.PushIpcInterface<IAudioIn>(audio_in);
}

void AudInU::OpenAudioInProtocolSpecified(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    auto protocol_specified{rp.PopRaw<u64>()};
    auto in_params{rp.PopRaw<AudioInParameter>()};
    auto applet_resource_user_id{rp.PopRaw<u64>()};
    const auto device_name_data{ctx.ReadBuffer()};
    auto device_name = Common::StringFromBuffer(device_name_data);
    auto handle{ctx.GetCopyHandle(0)};

    std::scoped_lock l{impl->mutex};
    auto link{impl->LinkToManager()};
    if (link.IsError()) {
        LOG_ERROR(Service_Audio, "Failed to link Audio In to Audio Manager");
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

    LOG_DEBUG(Service_Audio, "Opening new AudioIn, sessionid={}, free sessions={}", new_session_id,
              impl->num_free_sessions);

    auto audio_in = std::make_shared<IAudioIn>(system, *impl, new_session_id, device_name,
                                               in_params, handle, applet_resource_user_id);
    impl->sessions[new_session_id] = audio_in->GetImpl();
    impl->applet_resource_user_ids[new_session_id] = applet_resource_user_id;

    auto& out_system = impl->sessions[new_session_id]->GetSystem();
    AudioInParameterInternal out_params{.sample_rate = out_system.GetSampleRate(),
                                        .channel_count = out_system.GetChannelCount(),
                                        .sample_format =
                                            static_cast<u32>(out_system.GetSampleFormat()),
                                        .state = static_cast<u32>(out_system.GetState())};

    IPC::ResponseBuilder rb{ctx, 6, 0, 1};

    std::string out_name{out_system.GetName()};
    if (protocol_specified == 0) {
        if (out_system.IsUac()) {
            out_name = "UacIn";
        } else {
            out_name = "DeviceIn";
        }
    }

    ctx.WriteBuffer(out_name);

    rb.Push(ResultSuccess);
    rb.PushRaw<AudioInParameterInternal>(out_params);
    rb.PushIpcInterface<IAudioIn>(audio_in);
}

} // namespace Service::Audio
