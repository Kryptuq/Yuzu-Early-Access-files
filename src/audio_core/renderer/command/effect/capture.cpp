// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/renderer/adsp/command_list_processor.h"
#include "audio_core/renderer/command/effect/capture.h"
#include "audio_core/renderer/effect/effect_aux_info.h"
#include "core/memory.h"

namespace AudioCore::AudioRenderer {
/**
 * Reset an AuxBuffer.
 *
 * @param memory   - Core memory for writing.
 * @param aux_info - Memory address pointing to the AuxInfo to reset.
 */
static void ResetAuxBufferDsp(Core::Memory::Memory& memory, const CpuAddr aux_info) {
    if (aux_info == 0) {
        LOG_ERROR(Service_Audio, "Aux info is 0!");
        return;
    }

    memory.Write32(VAddr(aux_info + offsetof(AuxInfo::AuxInfoDsp, read_offset)), 0);
    memory.Write32(VAddr(aux_info + offsetof(AuxInfo::AuxInfoDsp, write_offset)), 0);
    memory.Write32(VAddr(aux_info + offsetof(AuxInfo::AuxInfoDsp, total_sample_count)), 0);
}

/**
 * Write the given input mix buffer to the memory at send_buffer, and update send_info_ if
 * update_count is set, to notify the game that an update happened.
 *
 * @param memory       - Core memory for writing.
 * @param send_info_   - Header information for where to write the mix buffer.
 * @param send_buffer  - Memory address to write the mix buffer to.
 * @param count_max    - Maximum number of samples in the receiving buffer.
 * @param input        - Input mix buffer to write.
 * @param write_count_ - Number of samples to write.
 * @param write_offset - Current offset to begin writing the receiving buffer at.
 * @param update_count - If non-zero, send_info_ will be updated.
 * @return Number of samples written.
 */
static u32 WriteAuxBufferDsp(Core::Memory::Memory& memory, const CpuAddr send_info_,
                             const CpuAddr send_buffer, u32 count_max, std::span<const s32> input,
                             const u32 write_count_, const u32 write_offset,
                             const u32 update_count) {
    if (write_count_ > count_max) {
        LOG_ERROR(Service_Audio,
                  "write_count must be smaller than count_max! write_count {}, count_max {}",
                  write_count_, count_max);
        return 0;
    }

    if (send_info_ == 0) {
        LOG_ERROR(Service_Audio, "send_info is 0!");
        return 0;
    }

    if (input.empty()) {
        LOG_ERROR(Service_Audio, "input buffer is empty!");
        return 0;
    }

    if (send_buffer == 0) {
        LOG_ERROR(Service_Audio, "send_buffer is 0!");
        return 0;
    }

    if (count_max == 0) {
        return 0;
    }

    AuxInfo::AuxBufferInfo send_info{};
    memory.ReadBlockUnsafe(send_info_, &send_info, sizeof(AuxInfo::AuxBufferInfo));

    u32 target_write_offset{send_info.dsp_info.write_offset + write_offset};
    if (target_write_offset > count_max || write_count_ == 0) {
        return 0;
    }

    u32 write_count{write_count_};
    u32 write_pos{0};
    while (write_count > 0) {
        u32 to_write{std::min(count_max - target_write_offset, write_count)};

        if (to_write > 0) {
            memory.WriteBlockUnsafe(send_buffer + target_write_offset * sizeof(s32),
                                    &input[write_pos], to_write * sizeof(s32));
        }

        target_write_offset = (target_write_offset + to_write) % count_max;
        write_count -= to_write;
        write_pos += to_write;
    }

    if (update_count) {
        const auto count_diff{send_info.dsp_info.total_sample_count -
                              send_info.cpu_info.total_sample_count};
        if (count_diff >= count_max) {
            auto dsp_lost_count{send_info.dsp_info.lost_sample_count + update_count};
            if (dsp_lost_count - send_info.cpu_info.lost_sample_count <
                send_info.dsp_info.lost_sample_count - send_info.cpu_info.lost_sample_count) {
                dsp_lost_count = send_info.cpu_info.lost_sample_count - 1;
            }
            send_info.dsp_info.lost_sample_count = dsp_lost_count;
        }

        send_info.dsp_info.write_offset =
            (send_info.dsp_info.write_offset + update_count + count_max) % count_max;

        auto new_sample_count{send_info.dsp_info.total_sample_count + update_count};
        if (new_sample_count - send_info.cpu_info.total_sample_count < count_diff) {
            new_sample_count = send_info.cpu_info.total_sample_count - 1;
        }
        send_info.dsp_info.total_sample_count = new_sample_count;
    }

    memory.WriteBlockUnsafe(send_info_, &send_info, sizeof(AuxInfo::AuxBufferInfo));

    return write_count_;
}

void CaptureCommand::Dump([[maybe_unused]] const ADSP::CommandListProcessor& processor,
                          std::string& string) {
    string += fmt::format("CaptureCommand\n\tenabled {} input {:02X} output {:02X}", effect_enabled,
                          input, output);
}

void CaptureCommand::Process(const ADSP::CommandListProcessor& processor) {
    if (effect_enabled) {
        auto input_buffer{
            processor.mix_buffers.subspan(input * processor.sample_count, processor.sample_count)};
        WriteAuxBufferDsp(*processor.memory, send_buffer_info, send_buffer, count_max, input_buffer,
                          processor.sample_count, write_offset, update_count);
    } else {
        ResetAuxBufferDsp(*processor.memory, send_buffer_info);
    }
}

bool CaptureCommand::Verify(const ADSP::CommandListProcessor& processor) {
    return true;
}

} // namespace AudioCore::AudioRenderer
