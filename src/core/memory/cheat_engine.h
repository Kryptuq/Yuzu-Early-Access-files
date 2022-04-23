// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <vector>
#include "common/common_types.h"
#include "core/memory/dmnt_cheat_types.h"
#include "core/memory/dmnt_cheat_vm.h"

namespace Core {
class System;
}

namespace Core::Timing {
class CoreTiming;
struct EventType;
} // namespace Core::Timing

namespace Core::Memory {

class StandardVmCallbacks : public DmntCheatVm::Callbacks {
public:
    StandardVmCallbacks(System& system_, const CheatProcessMetadata& metadata_);
    ~StandardVmCallbacks() override;

    void MemoryRead(VAddr address, void* data, u64 size) override;
    void MemoryWrite(VAddr address, const void* data, u64 size) override;
    u64 HidKeysDown() override;
    void DebugLog(u8 id, u64 value) override;
    void CommandLog(std::string_view data) override;

private:
    VAddr SanitizeAddress(VAddr address) const;

    const CheatProcessMetadata& metadata;
    System& system;
};

// Intermediary class that parses a text file or other disk format for storing cheats into a
// CheatList object, that can be used for execution.
class CheatParser {
public:
    virtual ~CheatParser();

    [[nodiscard]] virtual std::vector<CheatEntry> Parse(std::string_view data) const = 0;
};

// CheatParser implementation that parses text files
class TextCheatParser final : public CheatParser {
public:
    ~TextCheatParser() override;

    [[nodiscard]] std::vector<CheatEntry> Parse(std::string_view data) const override;
};

// Class that encapsulates a CheatList and manages its interaction with memory and CoreTiming
class CheatEngine final {
public:
    CheatEngine(System& system_, std::vector<CheatEntry> cheats_,
                const std::array<u8, 0x20>& build_id_);
    ~CheatEngine();

    void Initialize();
    void SetMainMemoryParameters(VAddr main_region_begin, u64 main_region_size);

    void Reload(std::vector<CheatEntry> reload_cheats);

private:
    void FrameCallback(std::uintptr_t user_data, std::chrono::nanoseconds ns_late);

    DmntCheatVm vm;
    CheatProcessMetadata metadata;

    std::vector<CheatEntry> cheats;
    std::atomic_bool is_pending_reload{false};

    std::shared_ptr<Core::Timing::EventType> event;
    Core::Timing::CoreTiming& core_timing;
    Core::System& system;
};

} // namespace Core::Memory
