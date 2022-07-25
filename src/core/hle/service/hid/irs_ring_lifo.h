// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>

#include "common/common_types.h"

namespace Service::IRS {

template <typename State, std::size_t max_buffer_size>
struct Lifo {
    s64 sampling_number{};
    s64 buffer_count{};
    std::array<State, max_buffer_size> entries{};

    const State& ReadCurrentEntry() const {
        return entries[GetBufferTail()];
    }

    const State& ReadPreviousEntry() const {
        return entries[GetPreviousEntryIndex()];
    }

    s64 GetBufferTail() const {
        return sampling_number % max_buffer_size;
    }

    std::size_t GetPreviousEntryIndex() const {
        return static_cast<size_t>((GetBufferTail() + max_buffer_size - 1) % max_buffer_size);
    }

    std::size_t GetNextEntryIndex() const {
        return static_cast<size_t>((GetBufferTail() + 1) % max_buffer_size);
    }

    void WriteNextEntry(const State& new_state) {
        if (buffer_count < static_cast<s64>(max_buffer_size)) {
            buffer_count++;
        }
        sampling_number++;
        entries[GetBufferTail()] = new_state;
    }
};

} // namespace Service::IRS
