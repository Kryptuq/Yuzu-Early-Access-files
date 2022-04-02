// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <array>
#include <cstring>
#include <vector>

#include "audio_core/audio_out.h"
#include "audio_core/codec.h"
#include "common/common_funcs.h"
#include "common/logging/log.h"
#include "common/swap.h"
#include "core/core.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/kernel/k_event.h"
#include "core/hle/service/audio/audout_u.h"
#include "core/hle/service/audio/errors.h"
#include "core/hle/service/kernel_helpers.h"
#include "core/memory.h"

namespace Service::Audio {

constexpr std::array<char, 10> DefaultDevice{{"DeviceOut"}};
constexpr int DefaultSampleRate{48000};

struct AudoutParams {
    s32_le sample_rate;
    u16_le channel_count;
    INSERT_PADDING_BYTES_NOINIT(2);
};
static_assert(sizeof(AudoutParams) == 0x8, "AudoutParams is an invalid size");

enum class AudioState : u32 {
    Started,
    Stopped,
};

class IAudioOut final : public ServiceFramework<IAudioOut> {
public:
    explicit IAudioOut(Core::System& system_, AudoutParams audio_params_,
                       AudioCore::AudioOut& audio_core_, std::string&& device_name_,
                       std::string&& unique_name)
        : ServiceFramework{system_, "IAudioOut", ServiceThreadType::CreateNew},
          audio_core{audio_core_}, device_name{std::move(device_name_)},
          audio_params{audio_params_}, main_memory{system.Memory()}, service_context{system_,
                                                                                     "IAudioOut"} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &IAudioOut::GetAudioOutState, "GetAudioOutState"},
            {1, &IAudioOut::StartAudioOut, "Start"},
            {2, &IAudioOut::StopAudioOut, "Stop"},
            {3, &IAudioOut::AppendAudioOutBufferImpl, "AppendAudioOutBuffer"},
            {4, &IAudioOut::RegisterBufferEvent, "RegisterBufferEvent"},
            {5, &IAudioOut::GetReleasedAudioOutBufferImpl, "GetReleasedAudioOutBuffers"},
            {6, &IAudioOut::ContainsAudioOutBuffer, "ContainsAudioOutBuffer"},
            {7, &IAudioOut::AppendAudioOutBufferImpl, "AppendAudioOutBufferAuto"},
            {8, &IAudioOut::GetReleasedAudioOutBufferImpl, "GetReleasedAudioOutBufferAuto"},
            {9, &IAudioOut::GetAudioOutBufferCount, "GetAudioOutBufferCount"},
            {10, &IAudioOut::GetAudioOutPlayedSampleCount, "GetAudioOutPlayedSampleCount"},
            {11, &IAudioOut::FlushAudioOutBuffers, "FlushAudioOutBuffers"},
            {12, &IAudioOut::SetAudioOutVolume, "SetAudioOutVolume"},
            {13, &IAudioOut::GetAudioOutVolume, "GetAudioOutVolume"},
        };
        // clang-format on
        RegisterHandlers(functions);

        // This is the event handle used to check if the audio buffer was released
        buffer_event = service_context.CreateEvent("IAudioOutBufferReleased");

        stream = audio_core.OpenStream(system.CoreTiming(), audio_params.sample_rate,
                                       audio_params.channel_count, std::move(unique_name), [this] {
                                           const auto guard = LockService();
                                           buffer_event->GetWritableEvent().Signal();
                                       });
    }

    ~IAudioOut() override {
        service_context.CloseEvent(buffer_event);
    }

private:
    struct AudioBuffer {
        u64_le next;
        u64_le buffer;
        u64_le buffer_capacity;
        u64_le buffer_size;
        u64_le offset;
    };
    static_assert(sizeof(AudioBuffer) == 0x28, "AudioBuffer is an invalid size");

    void GetAudioOutState(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_Audio, "called");

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push(static_cast<u32>(stream->IsPlaying() ? AudioState::Started : AudioState::Stopped));
    }

    void StartAudioOut(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_Audio, "called");

        if (stream->IsPlaying()) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ERR_OPERATION_FAILED);
            return;
        }

        audio_core.StartStream(stream);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void StopAudioOut(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_Audio, "called");

        if (stream->IsPlaying()) {
            audio_core.StopStream(stream);
        }

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void RegisterBufferEvent(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_Audio, "called");

        IPC::ResponseBuilder rb{ctx, 2, 1};
        rb.Push(ResultSuccess);
        rb.PushCopyObjects(buffer_event->GetReadableEvent());
    }

    void AppendAudioOutBufferImpl(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_Audio, "(STUBBED) called {}", ctx.Description());
        IPC::RequestParser rp{ctx};

        const auto& input_buffer{ctx.ReadBuffer()};
        ASSERT_MSG(input_buffer.size() == sizeof(AudioBuffer),
                   "AudioBuffer input is an invalid size!");
        AudioBuffer audio_buffer{};
        std::memcpy(&audio_buffer, input_buffer.data(), sizeof(AudioBuffer));
        const u64 tag{rp.Pop<u64>()};

        std::vector<s16> samples(audio_buffer.buffer_size / sizeof(s16));
        main_memory.ReadBlock(audio_buffer.buffer, samples.data(), audio_buffer.buffer_size);

        if (!audio_core.QueueBuffer(stream, tag, std::move(samples))) {
            IPC::ResponseBuilder rb{ctx, 2};
            rb.Push(ERR_BUFFER_COUNT_EXCEEDED);
            return;
        }

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void GetReleasedAudioOutBufferImpl(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_Audio, "called {}", ctx.Description());

        const u64 max_count{ctx.GetWriteBufferSize() / sizeof(u64)};
        const auto released_buffers{audio_core.GetTagsAndReleaseBuffers(stream, max_count)};

        std::vector<u64> tags{released_buffers};
        tags.resize(max_count);
        ctx.WriteBuffer(tags);

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push<u32>(static_cast<u32>(released_buffers.size()));
    }

    void ContainsAudioOutBuffer(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_Audio, "called");

        IPC::RequestParser rp{ctx};
        const u64 tag{rp.Pop<u64>()};
        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push(stream->ContainsBuffer(tag));
    }

    void GetAudioOutBufferCount(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_Audio, "called");

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push(static_cast<u32>(stream->GetQueueSize()));
    }

    void GetAudioOutPlayedSampleCount(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_Audio, "called");

        IPC::ResponseBuilder rb{ctx, 4};
        rb.Push(ResultSuccess);
        rb.Push(stream->GetPlayedSampleCount());
    }

    void FlushAudioOutBuffers(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_Audio, "called");

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push(stream->Flush());
    }

    void SetAudioOutVolume(Kernel::HLERequestContext& ctx) {
        IPC::RequestParser rp{ctx};
        const float volume = rp.Pop<float>();
        LOG_DEBUG(Service_Audio, "called, volume={}", volume);

        stream->SetVolume(volume);

        IPC::ResponseBuilder rb{ctx, 2};
        rb.Push(ResultSuccess);
    }

    void GetAudioOutVolume(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Service_Audio, "called");

        IPC::ResponseBuilder rb{ctx, 3};
        rb.Push(ResultSuccess);
        rb.Push(stream->GetVolume());
    }

    AudioCore::AudioOut& audio_core;
    AudioCore::StreamPtr stream;
    std::string device_name;

    [[maybe_unused]] AudoutParams audio_params{};

    Core::Memory::Memory& main_memory;

    KernelHelpers::ServiceContext service_context;

    /// This is the event handle used to check if the audio buffer was released
    Kernel::KEvent* buffer_event;
};

AudOutU::AudOutU(Core::System& system_) : ServiceFramework{system_, "audout:u"} {
    // clang-format off
    static const FunctionInfo functions[] = {
        {0, &AudOutU::ListAudioOutsImpl, "ListAudioOuts"},
        {1, &AudOutU::OpenAudioOutImpl, "OpenAudioOut"},
        {2, &AudOutU::ListAudioOutsImpl, "ListAudioOutsAuto"},
        {3, &AudOutU::OpenAudioOutImpl, "OpenAudioOutAuto"},
    };
    // clang-format on

    RegisterHandlers(functions);
    audio_core = std::make_unique<AudioCore::AudioOut>();
}

AudOutU::~AudOutU() = default;

void AudOutU::ListAudioOutsImpl(Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service_Audio, "called");

    ctx.WriteBuffer(DefaultDevice);

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.Push<u32>(1); // Amount of audio devices
}

void AudOutU::OpenAudioOutImpl(Kernel::HLERequestContext& ctx) {
    LOG_DEBUG(Service_Audio, "called");

    const auto device_name_data{ctx.ReadBuffer()};
    std::string device_name;
    if (device_name_data[0] != '\0') {
        device_name.assign(device_name_data.begin(), device_name_data.end());
    } else {
        device_name.assign(DefaultDevice.begin(), DefaultDevice.end());
    }
    ctx.WriteBuffer(device_name);

    IPC::RequestParser rp{ctx};
    auto params{rp.PopRaw<AudoutParams>()};
    if (params.channel_count <= 2) {
        // Mono does not exist for audout
        params.channel_count = 2;
    } else {
        params.channel_count = 6;
    }
    if (!params.sample_rate) {
        params.sample_rate = DefaultSampleRate;
    }

    std::string unique_name{fmt::format("{}-{}", device_name, audio_out_interfaces.size())};
    auto audio_out_interface = std::make_shared<IAudioOut>(
        system, params, *audio_core, std::move(device_name), std::move(unique_name));

    IPC::ResponseBuilder rb{ctx, 6, 0, 1};
    rb.Push(ResultSuccess);
    rb.Push<u32>(DefaultSampleRate);
    rb.Push<u32>(params.channel_count);
    rb.Push<u32>(static_cast<u32>(AudioCore::Codec::PcmFormat::Int16));
    rb.Push<u32>(static_cast<u32>(AudioState::Stopped));
    rb.PushIpcInterface<IAudioOut>(audio_out_interface);

    audio_out_interfaces.push_back(std::move(audio_out_interface));
}

} // namespace Service::Audio
