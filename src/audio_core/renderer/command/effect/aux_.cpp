// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "audio_core/renderer/adsp/command_list_processor.h"
#include "audio_core/renderer/command/effect/aux_.h"
#include "audio_core/renderer/effect/aux_.h"
#include "core/core.h"
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

    AuxInfo::AuxInfoDsp info{};
    auto info_ptr{&info};
    bool host_safe{(aux_info & Core::Memory::YUZU_PAGEMASK) <=
                   (Core::Memory::YUZU_PAGESIZE - sizeof(AuxInfo::AuxInfoDsp))};

    if (host_safe) [[likely]] {
        info_ptr = memory.GetPointer<AuxInfo::AuxInfoDsp>(aux_info);
    } else {
        memory.ReadBlockUnsafe(aux_info, info_ptr, sizeof(AuxInfo::AuxInfoDsp));
    }

    info_ptr->read_offset = 0;
    info_ptr->write_offset = 0;
    info_ptr->total_sample_count = 0;

    if (!host_safe) [[unlikely]] {
        memory.WriteBlockUnsafe(aux_info, info_ptr, sizeof(AuxInfo::AuxInfoDsp));
    }
}

/**
 * Write the given input mix buffer to the memory at send_buffer, and update send_info_ if
 * update_count is set, to notify the game that an update happened.
 *
 * @param memory       - Core memory for writing.
 * @param send_info_   - Meta information for where to write the mix buffer.
 * @param sample_count - Unused.
 * @param send_buffer  - Memory address to write the mix buffer to.
 * @param count_max    - Maximum number of samples in the receiving buffer.
 * @param input        - Input mix buffer to write.
 * @param write_count_ - Number of samples to write.
 * @param write_offset - Current offset to begin writing the receiving buffer at.
 * @param update_count - If non-zero, send_info_ will be updated.
 * @return Number of samples written.
 */
static u32 WriteAuxBufferDsp(Core::Memory::Memory& memory, CpuAddr send_info_,
                             [[maybe_unused]] u32 sample_count, CpuAddr send_buffer, u32 count_max,
                             std::span<const s32> input, u32 write_count_, u32 write_offset,
                             u32 update_count) {
    if (write_count_ > count_max) {
        LOG_ERROR(Service_Audio,
                  "write_count must be smaller than count_max! write_count {}, count_max {}",
                  write_count_, count_max);
        return 0;
    }

    if (send_info_ == 0) {
        LOG_ERROR(Service_Audio, "send_info_ is 0!");
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

    AuxInfo::AuxInfoDsp send_info{};
    auto send_ptr = &send_info;
    bool host_safe = (send_info_ & Core::Memory::YUZU_PAGEMASK) <=
                     (Core::Memory::YUZU_PAGESIZE - sizeof(AuxInfo::AuxInfoDsp));

    if (host_safe) [[likely]] {
        send_ptr = memory.GetPointer<AuxInfo::AuxInfoDsp>(send_info_);
    } else {
        memory.ReadBlockUnsafe(send_info_, send_ptr, sizeof(AuxInfo::AuxInfoDsp));
    }

    u32 target_write_offset{send_ptr->write_offset + write_offset};
    if (target_write_offset > count_max) {
        return 0;
    }

    u32 write_count{write_count_};
    u32 read_pos{0};
    while (write_count > 0) {
        u32 to_write{std::min(count_max - target_write_offset, write_count)};
        const auto write_addr = send_buffer + target_write_offset * sizeof(s32);
        bool write_safe{(write_addr & Core::Memory::YUZU_PAGEMASK) <=
                        (Core::Memory::YUZU_PAGESIZE - (write_addr + to_write * sizeof(s32)))};
        if (write_safe) [[likely]] {
            auto ptr = memory.GetPointer(write_addr);
            std::memcpy(ptr, &input[read_pos], to_write * sizeof(s32));
        } else {
            memory.WriteBlockUnsafe(send_buffer + target_write_offset * sizeof(s32),
                                    &input[read_pos], to_write * sizeof(s32));
        }
        target_write_offset = (target_write_offset + to_write) % count_max;
        write_count -= to_write;
        read_pos += to_write;
    }

    if (update_count) {
        send_ptr->write_offset = (send_ptr->write_offset + update_count) % count_max;
    }

    if (!host_safe) [[unlikely]] {
        memory.WriteBlockUnsafe(send_info_, send_ptr, sizeof(AuxInfo::AuxInfoDsp));
    }

    return write_count_;
}

/**
 * Read the given memory at return_buffer into the output mix buffer, and update return_info_ if
 * update_count is set, to notify the game that an update happened.
 *
 * @param memory        - Core memory for reading.
 * @param return_info_  - Meta information for where to read the mix buffer.
 * @param return_buffer - Memory address to read the samples from.
 * @param count_max     - Maximum number of samples in the receiving buffer.
 * @param output        - Output mix buffer which will receive the samples.
 * @param count_        - Number of samples to read.
 * @param read_offset   - Current offset to begin reading the return_buffer at.
 * @param update_count  - If non-zero, send_info_ will be updated.
 * @return Number of samples read.
 */
static u32 ReadAuxBufferDsp(Core::Memory::Memory& memory, CpuAddr return_info_,
                            CpuAddr return_buffer, u32 count_max, std::span<s32> output,
                            u32 read_count_, u32 read_offset, u32 update_count) {
    if (count_max == 0) {
        return 0;
    }

    if (read_count_ > count_max) {
        LOG_ERROR(Service_Audio, "count must be smaller than count_max! count {}, count_max {}",
                  read_count_, count_max);
        return 0;
    }

    if (return_info_ == 0) {
        LOG_ERROR(Service_Audio, "return_info_ is 0!");
        return 0;
    }

    if (output.empty()) {
        LOG_ERROR(Service_Audio, "output buffer is empty!");
        return 0;
    }

    if (return_buffer == 0) {
        LOG_ERROR(Service_Audio, "return_buffer is 0!");
        return 0;
    }

    AuxInfo::AuxInfoDsp return_info{};
    auto return_ptr = &return_info;
    bool host_safe = (return_info_ & Core::Memory::YUZU_PAGEMASK) <=
                     (Core::Memory::YUZU_PAGESIZE - sizeof(AuxInfo::AuxInfoDsp));

    if (host_safe) [[likely]] {
        return_ptr = memory.GetPointer<AuxInfo::AuxInfoDsp>(return_info_);
    } else {
        memory.ReadBlockUnsafe(return_info_, return_ptr, sizeof(AuxInfo::AuxInfoDsp));
    }

    u32 target_read_offset{return_ptr->read_offset + read_offset};
    if (target_read_offset > count_max) {
        return 0;
    }

    u32 read_count{read_count_};
    u32 write_pos{0};
    while (read_count > 0) {
        u32 to_read{std::min(count_max - target_read_offset, read_count)};
        const auto read_addr = return_buffer + target_read_offset * sizeof(s32);
        bool read_safe{(read_addr & Core::Memory::YUZU_PAGEMASK) <=
                       (Core::Memory::YUZU_PAGESIZE - (read_addr + to_read * sizeof(s32)))};
        if (read_safe) [[likely]] {
            auto ptr = memory.GetPointer(read_addr);
            std::memcpy(&output[write_pos], ptr, to_read * sizeof(s32));
        } else {
            memory.ReadBlockUnsafe(return_buffer + target_read_offset * sizeof(s32),
                                   &output[write_pos], to_read * sizeof(s32));
        }
        target_read_offset = (target_read_offset + to_read) % count_max;
        read_count -= to_read;
        write_pos += to_read;
    }

    if (update_count) {
        return_ptr->read_offset = (return_ptr->read_offset + update_count) % count_max;
    }

    if (!host_safe) [[unlikely]] {
        memory.WriteBlockUnsafe(return_info_, return_ptr, sizeof(AuxInfo::AuxInfoDsp));
    }

    return read_count_;
}

void AuxCommand::Dump([[maybe_unused]] const ADSP::CommandListProcessor& processor,
                      std::string& string) {
    string += fmt::format("AuxCommand\n\tenabled {} input {:02X} output {:02X}\n", effect_enabled,
                          input, output);
}

void AuxCommand::Process(const ADSP::CommandListProcessor& processor) {
    auto input_buffer{
        processor.mix_buffers.subspan(input * processor.sample_count, processor.sample_count)};
    auto output_buffer{
        processor.mix_buffers.subspan(output * processor.sample_count, processor.sample_count)};

    if (effect_enabled) {
        WriteAuxBufferDsp(*processor.memory, send_buffer_info, processor.sample_count, send_buffer,
                          count_max, input_buffer, processor.sample_count, write_offset,
                          update_count);

        auto read{ReadAuxBufferDsp(*processor.memory, return_buffer_info, return_buffer, count_max,
                                   output_buffer, processor.sample_count, write_offset,
                                   update_count)};

        if (read != processor.sample_count) {
            std::memset(&output_buffer[read], 0, (processor.sample_count - read) * sizeof(s32));
        }
    } else {
        ResetAuxBufferDsp(*processor.memory, send_buffer_info);
        ResetAuxBufferDsp(*processor.memory, return_buffer_info);
        if (input != output) {
            std::memcpy(output_buffer.data(), input_buffer.data(), output_buffer.size_bytes());
        }
    }
}

bool AuxCommand::Verify(const ADSP::CommandListProcessor& processor) {
    return true;
}

} // namespace AudioCore::AudioRenderer
