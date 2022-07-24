// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "audio_core/sink/sink.h"
#include "audio_core/sink/sink_stream.h"

namespace AudioCore::Sink {
/**
 * A no-op sink for when no audio out is wanted.
 */
class NullSink final : public Sink {
public:
    explicit NullSink(std::string_view) {}
    ~NullSink() override = default;

    SinkStream* AcquireSinkStream([[maybe_unused]] Core::System& system,
                                  [[maybe_unused]] u32 system_channels,
                                  [[maybe_unused]] const std::string& name,
                                  [[maybe_unused]] StreamType type) override {
        return &null_sink_stream;
    }

    void CloseStream([[maybe_unused]] const SinkStream* stream) override {}
    void CloseStreams() override {}
    void PauseStreams() override {}
    void UnpauseStreams() override {}
    f32 GetDeviceVolume() const override {
        return 1.0f;
    }
    void SetDeviceVolume(f32 volume) override {}
    void SetSystemVolume(f32 volume) override {}

private:
    struct NullSinkStreamImpl final : SinkStream {
        void Finalize() override {}
        void Start(bool resume = false) override {}
        void Stop() override {}
        void AppendBuffer([[maybe_unused]] ::AudioCore::Sink::SinkBuffer& buffer,
                          [[maybe_unused]] std::vector<s16>& samples) override {}
        std::vector<s16> ReleaseBuffer([[maybe_unused]] u64 num_samples) override {
            return {};
        }
        bool IsBufferConsumed([[maybe_unused]] const u64 tag) {
            return true;
        }
        void ClearQueue() override {}
    } null_sink_stream;
};

} // namespace AudioCore::Sink
