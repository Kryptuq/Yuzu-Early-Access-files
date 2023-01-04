// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <chrono>
#include <cstring>
#include <memory>
#include <vector>

#include <opus.h>
#include <opus_multistream.h>

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/service/audio/hwopus.h"

namespace Service::Audio {
namespace {
struct OpusDeleter {
    void operator()(OpusMSDecoder* ptr) const {
        opus_multistream_decoder_destroy(ptr);
    }
};

using OpusDecoderPtr = std::unique_ptr<OpusMSDecoder, OpusDeleter>;

struct OpusPacketHeader {
    // Packet size in bytes.
    u32_be size;
    // Indicates the final range of the codec's entropy coder.
    u32_be final_range;
};
static_assert(sizeof(OpusPacketHeader) == 0x8, "OpusHeader is an invalid size");

class OpusDecoderState {
public:
    /// Describes extra behavior that may be asked of the decoding context.
    enum class ExtraBehavior {
        /// No extra behavior.
        None,

        /// Resets the decoder context back to a freshly initialized state.
        ResetContext,
    };

    enum class PerfTime {
        Disabled,
        Enabled,
    };

    explicit OpusDecoderState(OpusDecoderPtr decoder_, u32 sample_rate_, u32 channel_count_)
        : decoder{std::move(decoder_)}, sample_rate{sample_rate_}, channel_count{channel_count_} {}

    // Decodes interleaved Opus packets. Optionally allows reporting time taken to
    // perform the decoding, as well as any relevant extra behavior.
    void DecodeInterleaved(Kernel::HLERequestContext& ctx, PerfTime perf_time,
                           ExtraBehavior extra_behavior) {
        if (perf_time == PerfTime::Disabled) {
            DecodeInterleavedHelper(ctx, nullptr, extra_behavior);
        } else {
            u64 performance = 0;
            DecodeInterleavedHelper(ctx, &performance, extra_behavior);
        }
    }

private:
    void DecodeInterleavedHelper(Kernel::HLERequestContext& ctx, u64* performance,
                                 ExtraBehavior extra_behavior) {
        u32 consumed = 0;
        u32 sample_count = 0;
        std::vector<opus_int16> samples(ctx.GetWriteBufferNumElements<opus_int16>());

        if (extra_behavior == ExtraBehavior::ResetContext) {
            ResetDecoderContext();
        }

        if (!DecodeOpusData(consumed, sample_count, ctx.ReadBuffer(), samples, performance)) {
            LOG_ERROR(Audio, "Failed to decode opus data");
            IPC::ResponseBuilder rb{ctx, 2};
            // TODO(ogniK): Use correct error code
            rb.Push(ResultUnknown);
            return;
        }

        const u32 param_size = performance != nullptr ? 6 : 4;
        IPC::ResponseBuilder rb{ctx, param_size};
        rb.Push(ResultSuccess);
        rb.Push<u32>(consumed);
        rb.Push<u32>(sample_count);
        if (performance) {
            rb.Push<u64>(*performance);
        }
        ctx.WriteBuffer(samples);
    }

    bool DecodeOpusData(u32& consumed, u32& sample_count, std::span<const u8> input,
                        std::vector<opus_int16>& output, u64* out_performance_time) const {
        const auto start_time = std::chrono::steady_clock::now();
        const std::size_t raw_output_sz = output.size() * sizeof(opus_int16);
        if (sizeof(OpusPacketHeader) > input.size()) {
            LOG_ERROR(Audio, "Input is smaller than the header size, header_sz={}, input_sz={}",
                      sizeof(OpusPacketHeader), input.size());
            return false;
        }

        OpusPacketHeader hdr{};
        std::memcpy(&hdr, input.data(), sizeof(OpusPacketHeader));
        if (sizeof(OpusPacketHeader) + static_cast<u32>(hdr.size) > input.size()) {
            LOG_ERROR(Audio, "Input does not fit in the opus header size. data_sz={}, input_sz={}",
                      sizeof(OpusPacketHeader) + static_cast<u32>(hdr.size), input.size());
            return false;
        }

        const auto frame = input.data() + sizeof(OpusPacketHeader);
        const auto decoded_sample_count = opus_packet_get_nb_samples(
            frame, static_cast<opus_int32>(input.size() - sizeof(OpusPacketHeader)),
            static_cast<opus_int32>(sample_rate));
        if (decoded_sample_count * channel_count * sizeof(u16) > raw_output_sz) {
            LOG_ERROR(
                Audio,
                "Decoded data does not fit into the output data, decoded_sz={}, raw_output_sz={}",
                decoded_sample_count * channel_count * sizeof(u16), raw_output_sz);
            return false;
        }

        const int frame_size = (static_cast<int>(raw_output_sz / sizeof(s16) / channel_count));
        const auto out_sample_count =
            opus_multistream_decode(decoder.get(), frame, hdr.size, output.data(), frame_size, 0);
        if (out_sample_count < 0) {
            LOG_ERROR(Audio,
                      "Incorrect sample count received from opus_decode, "
                      "output_sample_count={}, frame_size={}, data_sz_from_hdr={}",
                      out_sample_count, frame_size, static_cast<u32>(hdr.size));
            return false;
        }

        const auto end_time = std::chrono::steady_clock::now() - start_time;
        sample_count = out_sample_count;
        consumed = static_cast<u32>(sizeof(OpusPacketHeader) + hdr.size);
        if (out_performance_time != nullptr) {
            *out_performance_time =
                std::chrono::duration_cast<std::chrono::milliseconds>(end_time).count();
        }

        return true;
    }

    void ResetDecoderContext() {
        ASSERT(decoder != nullptr);

        opus_multistream_decoder_ctl(decoder.get(), OPUS_RESET_STATE);
    }

    OpusDecoderPtr decoder;
    u32 sample_rate;
    u32 channel_count;
};

class IHardwareOpusDecoderManager final : public ServiceFramework<IHardwareOpusDecoderManager> {
public:
    explicit IHardwareOpusDecoderManager(Core::System& system_, OpusDecoderState decoder_state_)
        : ServiceFramework{system_, "IHardwareOpusDecoderManager"}, decoder_state{
                                                                        std::move(decoder_state_)} {
        // clang-format off
        static const FunctionInfo functions[] = {
            {0, &IHardwareOpusDecoderManager::DecodeInterleavedOld, "DecodeInterleavedOld"},
            {1, nullptr, "SetContext"},
            {2, nullptr, "DecodeInterleavedForMultiStreamOld"},
            {3, nullptr, "SetContextForMultiStream"},
            {4, &IHardwareOpusDecoderManager::DecodeInterleavedWithPerfOld, "DecodeInterleavedWithPerfOld"},
            {5, nullptr, "DecodeInterleavedForMultiStreamWithPerfOld"},
            {6, &IHardwareOpusDecoderManager::DecodeInterleaved, "DecodeInterleavedWithPerfAndResetOld"},
            {7, nullptr, "DecodeInterleavedForMultiStreamWithPerfAndResetOld"},
            {8, &IHardwareOpusDecoderManager::DecodeInterleaved, "DecodeInterleaved"},
            {9, nullptr, "DecodeInterleavedForMultiStream"},
        };
        // clang-format on

        RegisterHandlers(functions);
    }

private:
    void DecodeInterleavedOld(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Audio, "called");

        decoder_state.DecodeInterleaved(ctx, OpusDecoderState::PerfTime::Disabled,
                                        OpusDecoderState::ExtraBehavior::None);
    }

    void DecodeInterleavedWithPerfOld(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Audio, "called");

        decoder_state.DecodeInterleaved(ctx, OpusDecoderState::PerfTime::Enabled,
                                        OpusDecoderState::ExtraBehavior::None);
    }

    void DecodeInterleaved(Kernel::HLERequestContext& ctx) {
        LOG_DEBUG(Audio, "called");

        IPC::RequestParser rp{ctx};
        const auto extra_behavior = rp.Pop<bool>() ? OpusDecoderState::ExtraBehavior::ResetContext
                                                   : OpusDecoderState::ExtraBehavior::None;

        decoder_state.DecodeInterleaved(ctx, OpusDecoderState::PerfTime::Enabled, extra_behavior);
    }

    OpusDecoderState decoder_state;
};

std::size_t WorkerBufferSize(u32 channel_count) {
    ASSERT_MSG(channel_count == 1 || channel_count == 2, "Invalid channel count");
    constexpr int num_streams = 1;
    const int num_stereo_streams = channel_count == 2 ? 1 : 0;
    return opus_multistream_decoder_get_size(num_streams, num_stereo_streams);
}

// Creates the mapping table that maps the input channels to the particular
// output channels. In the stereo case, we map the left and right input channels
// to the left and right output channels respectively.
//
// However, in the monophonic case, we only map the one available channel
// to the sole output channel. We specify 255 for the would-be right channel
// as this is a special value defined by Opus to indicate to the decoder to
// ignore that channel.
std::array<u8, 2> CreateMappingTable(u32 channel_count) {
    if (channel_count == 2) {
        return {{0, 1}};
    }

    return {{0, 255}};
}
} // Anonymous namespace

void HwOpus::GetWorkBufferSize(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto sample_rate = rp.Pop<u32>();
    const auto channel_count = rp.Pop<u32>();

    LOG_DEBUG(Audio, "called with sample_rate={}, channel_count={}", sample_rate, channel_count);

    ASSERT_MSG(sample_rate == 48000 || sample_rate == 24000 || sample_rate == 16000 ||
                   sample_rate == 12000 || sample_rate == 8000,
               "Invalid sample rate");
    ASSERT_MSG(channel_count == 1 || channel_count == 2, "Invalid channel count");

    const u32 worker_buffer_sz = static_cast<u32>(WorkerBufferSize(channel_count));
    LOG_DEBUG(Audio, "worker_buffer_sz={}", worker_buffer_sz);

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.Push<u32>(worker_buffer_sz);
}

void HwOpus::GetWorkBufferSizeEx(Kernel::HLERequestContext& ctx) {
    GetWorkBufferSize(ctx);
}

void HwOpus::GetWorkBufferSizeForMultiStreamEx(Kernel::HLERequestContext& ctx) {
    OpusMultiStreamParametersEx param;
    std::memcpy(&param, ctx.ReadBuffer().data(), ctx.GetReadBufferSize());

    const auto sample_rate = param.sample_rate;
    const auto channel_count = param.channel_count;
    const auto number_streams = param.number_streams;
    const auto number_stereo_streams = param.number_stereo_streams;

    LOG_DEBUG(
        Audio,
        "called with sample_rate={}, channel_count={}, number_streams={}, number_stereo_streams={}",
        sample_rate, channel_count, number_streams, number_stereo_streams);

    ASSERT_MSG(sample_rate == 48000 || sample_rate == 24000 || sample_rate == 16000 ||
                   sample_rate == 12000 || sample_rate == 8000,
               "Invalid sample rate");

    const u32 worker_buffer_sz =
        static_cast<u32>(opus_multistream_decoder_get_size(number_streams, number_stereo_streams));

    IPC::ResponseBuilder rb{ctx, 3};
    rb.Push(ResultSuccess);
    rb.Push<u32>(worker_buffer_sz);
}

void HwOpus::OpenHardwareOpusDecoder(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto sample_rate = rp.Pop<u32>();
    const auto channel_count = rp.Pop<u32>();
    const auto buffer_sz = rp.Pop<u32>();

    LOG_DEBUG(Audio, "called sample_rate={}, channel_count={}, buffer_size={}", sample_rate,
              channel_count, buffer_sz);

    ASSERT_MSG(sample_rate == 48000 || sample_rate == 24000 || sample_rate == 16000 ||
                   sample_rate == 12000 || sample_rate == 8000,
               "Invalid sample rate");
    ASSERT_MSG(channel_count == 1 || channel_count == 2, "Invalid channel count");

    const std::size_t worker_sz = WorkerBufferSize(channel_count);
    ASSERT_MSG(buffer_sz >= worker_sz, "Worker buffer too large");

    const int num_stereo_streams = channel_count == 2 ? 1 : 0;
    const auto mapping_table = CreateMappingTable(channel_count);

    int error = 0;
    OpusDecoderPtr decoder{
        opus_multistream_decoder_create(sample_rate, static_cast<int>(channel_count), 1,
                                        num_stereo_streams, mapping_table.data(), &error)};
    if (error != OPUS_OK || decoder == nullptr) {
        LOG_ERROR(Audio, "Failed to create Opus decoder (error={}).", error);
        IPC::ResponseBuilder rb{ctx, 2};
        // TODO(ogniK): Use correct error code
        rb.Push(ResultUnknown);
        return;
    }

    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(ResultSuccess);
    rb.PushIpcInterface<IHardwareOpusDecoderManager>(
        system, OpusDecoderState{std::move(decoder), sample_rate, channel_count});
}

void HwOpus::OpenHardwareOpusDecoderEx(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp{ctx};
    const auto sample_rate = rp.Pop<u32>();
    const auto channel_count = rp.Pop<u32>();

    LOG_DEBUG(Audio, "called sample_rate={}, channel_count={}", sample_rate, channel_count);

    ASSERT_MSG(sample_rate == 48000 || sample_rate == 24000 || sample_rate == 16000 ||
                   sample_rate == 12000 || sample_rate == 8000,
               "Invalid sample rate");
    ASSERT_MSG(channel_count == 1 || channel_count == 2, "Invalid channel count");

    const int num_stereo_streams = channel_count == 2 ? 1 : 0;
    const auto mapping_table = CreateMappingTable(channel_count);

    int error = 0;
    OpusDecoderPtr decoder{
        opus_multistream_decoder_create(sample_rate, static_cast<int>(channel_count), 1,
                                        num_stereo_streams, mapping_table.data(), &error)};
    if (error != OPUS_OK || decoder == nullptr) {
        LOG_ERROR(Audio, "Failed to create Opus decoder (error={}).", error);
        IPC::ResponseBuilder rb{ctx, 2};
        // TODO(ogniK): Use correct error code
        rb.Push(ResultUnknown);
        return;
    }

    IPC::ResponseBuilder rb{ctx, 2, 0, 1};
    rb.Push(ResultSuccess);
    rb.PushIpcInterface<IHardwareOpusDecoderManager>(
        system, OpusDecoderState{std::move(decoder), sample_rate, channel_count});
}

HwOpus::HwOpus(Core::System& system_) : ServiceFramework{system_, "hwopus"} {
    static const FunctionInfo functions[] = {
        {0, &HwOpus::OpenHardwareOpusDecoder, "OpenHardwareOpusDecoder"},
        {1, &HwOpus::GetWorkBufferSize, "GetWorkBufferSize"},
        {2, nullptr, "OpenOpusDecoderForMultiStream"},
        {3, nullptr, "GetWorkBufferSizeForMultiStream"},
        {4, &HwOpus::OpenHardwareOpusDecoderEx, "OpenHardwareOpusDecoderEx"},
        {5, &HwOpus::GetWorkBufferSizeEx, "GetWorkBufferSizeEx"},
        {6, nullptr, "OpenHardwareOpusDecoderForMultiStreamEx"},
        {7, &HwOpus::GetWorkBufferSizeForMultiStreamEx, "GetWorkBufferSizeForMultiStreamEx"},
    };
    RegisterHandlers(functions);
}

HwOpus::~HwOpus() = default;

} // namespace Service::Audio
