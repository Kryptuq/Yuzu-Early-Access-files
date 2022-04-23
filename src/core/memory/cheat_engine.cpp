// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <locale>
#include "common/hex_util.h"
#include "common/microprofile.h"
#include "common/swap.h"
#include "core/core.h"
#include "core/core_timing.h"
#include "core/hle/kernel/k_page_table.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/service/hid/controllers/npad.h"
#include "core/hle/service/hid/hid.h"
#include "core/hle/service/sm/sm.h"
#include "core/memory.h"
#include "core/memory/cheat_engine.h"

namespace Core::Memory {
namespace {
constexpr auto CHEAT_ENGINE_NS = std::chrono::nanoseconds{1000000000 / 12};

std::string_view ExtractName(std::string_view data, std::size_t start_index, char match) {
    auto end_index = start_index;
    while (data[end_index] != match) {
        ++end_index;
        if (end_index > data.size() ||
            (end_index - start_index - 1) > sizeof(CheatDefinition::readable_name)) {
            return {};
        }
    }

    return data.substr(start_index, end_index - start_index);
}
} // Anonymous namespace

StandardVmCallbacks::StandardVmCallbacks(System& system_, const CheatProcessMetadata& metadata_)
    : metadata{metadata_}, system{system_} {}

StandardVmCallbacks::~StandardVmCallbacks() = default;

void StandardVmCallbacks::MemoryRead(VAddr address, void* data, u64 size) {
    system.Memory().ReadBlock(SanitizeAddress(address), data, size);
}

void StandardVmCallbacks::MemoryWrite(VAddr address, const void* data, u64 size) {
    system.Memory().WriteBlock(SanitizeAddress(address), data, size);
}

u64 StandardVmCallbacks::HidKeysDown() {
    const auto applet_resource =
        system.ServiceManager().GetService<Service::HID::Hid>("hid")->GetAppletResource();
    if (applet_resource == nullptr) {
        LOG_WARNING(CheatEngine,
                    "Attempted to read input state, but applet resource is not initialized!");
        return 0;
    }

    const auto press_state =
        applet_resource
            ->GetController<Service::HID::Controller_NPad>(Service::HID::HidController::NPad)
            .GetAndResetPressState();
    return static_cast<u64>(press_state & HID::NpadButton::All);
}

void StandardVmCallbacks::DebugLog(u8 id, u64 value) {
    LOG_INFO(CheatEngine, "Cheat triggered DebugLog: ID '{:01X}' Value '{:016X}'", id, value);
}

void StandardVmCallbacks::CommandLog(std::string_view data) {
    LOG_DEBUG(CheatEngine, "[DmntCheatVm]: {}",
              data.back() == '\n' ? data.substr(0, data.size() - 1) : data);
}

VAddr StandardVmCallbacks::SanitizeAddress(VAddr in) const {
    if ((in < metadata.main_nso_extents.base ||
         in >= metadata.main_nso_extents.base + metadata.main_nso_extents.size) &&
        (in < metadata.heap_extents.base ||
         in >= metadata.heap_extents.base + metadata.heap_extents.size)) {
        LOG_ERROR(CheatEngine,
                  "Cheat attempting to access memory at invalid address={:016X}, if this "
                  "persists, "
                  "the cheat may be incorrect. However, this may be normal early in execution if "
                  "the game has not properly set up yet.",
                  in);
        return 0; ///< Invalid addresses will hard crash
    }

    return in;
}

CheatParser::~CheatParser() = default;

TextCheatParser::~TextCheatParser() = default;

std::vector<CheatEntry> TextCheatParser::Parse(std::string_view data) const {
    std::vector<CheatEntry> out(1);
    std::optional<u64> current_entry;

    for (std::size_t i = 0; i < data.size(); ++i) {
        if (::isspace(data[i])) {
            continue;
        }

        if (data[i] == '{') {
            current_entry = 0;

            if (out[*current_entry].definition.num_opcodes > 0) {
                return {};
            }

            const auto name = ExtractName(data, i + 1, '}');
            if (name.empty()) {
                return {};
            }

            std::memcpy(out[*current_entry].definition.readable_name.data(), name.data(),
                        std::min<std::size_t>(out[*current_entry].definition.readable_name.size(),
                                              name.size()));
            out[*current_entry]
                .definition.readable_name[out[*current_entry].definition.readable_name.size() - 1] =
                '\0';

            i += name.length() + 1;
        } else if (data[i] == '[') {
            current_entry = out.size();
            out.emplace_back();

            const auto name = ExtractName(data, i + 1, ']');
            if (name.empty()) {
                return {};
            }

            std::memcpy(out[*current_entry].definition.readable_name.data(), name.data(),
                        std::min<std::size_t>(out[*current_entry].definition.readable_name.size(),
                                              name.size()));
            out[*current_entry]
                .definition.readable_name[out[*current_entry].definition.readable_name.size() - 1] =
                '\0';

            i += name.length() + 1;
        } else if (::isxdigit(data[i])) {
            if (!current_entry || out[*current_entry].definition.num_opcodes >=
                                      out[*current_entry].definition.opcodes.size()) {
                return {};
            }

            const auto hex = std::string(data.substr(i, 8));
            if (!std::all_of(hex.begin(), hex.end(), ::isxdigit)) {
                return {};
            }

            const auto value = static_cast<u32>(std::stoul(hex, nullptr, 0x10));
            out[*current_entry].definition.opcodes[out[*current_entry].definition.num_opcodes++] =
                value;

            i += 8;
        } else {
            return {};
        }
    }

    out[0].enabled = out[0].definition.num_opcodes > 0;
    out[0].cheat_id = 0;

    for (u32 i = 1; i < out.size(); ++i) {
        out[i].enabled = out[i].definition.num_opcodes > 0;
        out[i].cheat_id = i;
    }

    return out;
}

CheatEngine::CheatEngine(System& system_, std::vector<CheatEntry> cheats_,
                         const std::array<u8, 0x20>& build_id_)
    : vm{std::make_unique<StandardVmCallbacks>(system_, metadata)},
      cheats(std::move(cheats_)), core_timing{system_.CoreTiming()}, system{system_} {
    metadata.main_nso_build_id = build_id_;
}

CheatEngine::~CheatEngine() {
    core_timing.UnscheduleEvent(event, 0);
}

void CheatEngine::Initialize() {
    event = Core::Timing::CreateEvent(
        "CheatEngine::FrameCallback::" + Common::HexToString(metadata.main_nso_build_id),
        [this](std::uintptr_t user_data, std::chrono::nanoseconds ns_late) {
            FrameCallback(user_data, ns_late);
        });
    core_timing.ScheduleEvent(CHEAT_ENGINE_NS, event);

    metadata.process_id = system.CurrentProcess()->GetProcessID();
    metadata.title_id = system.GetCurrentProcessProgramID();

    const auto& page_table = system.CurrentProcess()->PageTable();
    metadata.heap_extents = {
        .base = page_table.GetHeapRegionStart(),
        .size = page_table.GetHeapRegionSize(),
    };

    metadata.address_space_extents = {
        .base = page_table.GetAddressSpaceStart(),
        .size = page_table.GetAddressSpaceSize(),
    };

    metadata.alias_extents = {
        .base = page_table.GetAliasCodeRegionStart(),
        .size = page_table.GetAliasCodeRegionSize(),
    };

    is_pending_reload.exchange(true);
}

void CheatEngine::SetMainMemoryParameters(VAddr main_region_begin, u64 main_region_size) {
    metadata.main_nso_extents = {
        .base = main_region_begin,
        .size = main_region_size,
    };
}

void CheatEngine::Reload(std::vector<CheatEntry> reload_cheats) {
    cheats = std::move(reload_cheats);
    is_pending_reload.exchange(true);
}

MICROPROFILE_DEFINE(Cheat_Engine, "Add-Ons", "Cheat Engine", MP_RGB(70, 200, 70));

void CheatEngine::FrameCallback(std::uintptr_t, std::chrono::nanoseconds ns_late) {
    if (is_pending_reload.exchange(false)) {
        vm.LoadProgram(cheats);
    }

    if (vm.GetProgramSize() == 0) {
        return;
    }

    MICROPROFILE_SCOPE(Cheat_Engine);

    vm.Execute(metadata);

    core_timing.ScheduleEvent(CHEAT_ENGINE_NS - ns_late, event);
}

} // namespace Core::Memory
