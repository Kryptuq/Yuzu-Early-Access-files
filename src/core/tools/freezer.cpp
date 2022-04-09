// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
#include "common/logging/log.h"
#include "core/core.h"
#include "core/core_timing.h"
#include "core/memory.h"
#include "core/tools/freezer.h"

namespace Tools {
namespace {

constexpr auto memory_freezer_ns = std::chrono::nanoseconds{1000000000 / 60};

u64 MemoryReadWidth(Core::Memory::Memory& memory, u32 width, VAddr addr) {
    switch (width) {
    case 1:
        return memory.Read8(addr);
    case 2:
        return memory.Read16(addr);
    case 4:
        return memory.Read32(addr);
    case 8:
        return memory.Read64(addr);
    default:
        UNREACHABLE();
        return 0;
    }
}

void MemoryWriteWidth(Core::Memory::Memory& memory, u32 width, VAddr addr, u64 value) {
    switch (width) {
    case 1:
        memory.Write8(addr, static_cast<u8>(value));
        break;
    case 2:
        memory.Write16(addr, static_cast<u16>(value));
        break;
    case 4:
        memory.Write32(addr, static_cast<u32>(value));
        break;
    case 8:
        memory.Write64(addr, value);
        break;
    default:
        UNREACHABLE();
    }
}

} // Anonymous namespace

Freezer::Freezer(Core::Timing::CoreTiming& core_timing_, Core::Memory::Memory& memory_)
    : core_timing{core_timing_}, memory{memory_} {
    event = Core::Timing::CreateEvent(
        "MemoryFreezer::FrameCallback",
        [this](std::uintptr_t user_data, std::chrono::nanoseconds ns_late) {
            FrameCallback(user_data, ns_late);
        });
    core_timing.ScheduleEvent(memory_freezer_ns, event);
}

Freezer::~Freezer() {
    core_timing.UnscheduleEvent(event, 0);
}

void Freezer::SetActive(bool is_active) {
    if (!active.exchange(is_active)) {
        FillEntryReads();
        core_timing.ScheduleEvent(memory_freezer_ns, event);
        LOG_DEBUG(Common_Memory, "Memory freezer activated!");
    } else {
        LOG_DEBUG(Common_Memory, "Memory freezer deactivated!");
    }
}

bool Freezer::IsActive() const {
    return active.load(std::memory_order_relaxed);
}

void Freezer::Clear() {
    std::scoped_lock lock{entries_mutex};

    LOG_DEBUG(Common_Memory, "Clearing all frozen memory values.");

    entries.clear();
}

u64 Freezer::Freeze(VAddr address, u32 width) {
    std::scoped_lock lock{entries_mutex};

    const auto current_value = MemoryReadWidth(memory, width, address);
    entries.push_back({address, width, current_value});

    LOG_DEBUG(Common_Memory,
              "Freezing memory for address={:016X}, width={:02X}, current_value={:016X}", address,
              width, current_value);

    return current_value;
}

void Freezer::Unfreeze(VAddr address) {
    std::scoped_lock lock{entries_mutex};

    LOG_DEBUG(Common_Memory, "Unfreezing memory for address={:016X}", address);

    std::erase_if(entries, [address](const Entry& entry) { return entry.address == address; });
}

bool Freezer::IsFrozen(VAddr address) const {
    std::scoped_lock lock{entries_mutex};

    return FindEntry(address) != entries.cend();
}

void Freezer::SetFrozenValue(VAddr address, u64 value) {
    std::scoped_lock lock{entries_mutex};

    const auto iter = FindEntry(address);

    if (iter == entries.cend()) {
        LOG_ERROR(Common_Memory,
                  "Tried to set freeze value for address={:016X} that is not frozen!", address);
        return;
    }

    LOG_DEBUG(Common_Memory,
              "Manually overridden freeze value for address={:016X}, width={:02X} to value={:016X}",
              iter->address, iter->width, value);
    iter->value = value;
}

std::optional<Freezer::Entry> Freezer::GetEntry(VAddr address) const {
    std::scoped_lock lock{entries_mutex};

    const auto iter = FindEntry(address);

    if (iter == entries.cend()) {
        return std::nullopt;
    }

    return *iter;
}

std::vector<Freezer::Entry> Freezer::GetEntries() const {
    std::scoped_lock lock{entries_mutex};

    return entries;
}

Freezer::Entries::iterator Freezer::FindEntry(VAddr address) {
    return std::find_if(entries.begin(), entries.end(),
                        [address](const Entry& entry) { return entry.address == address; });
}

Freezer::Entries::const_iterator Freezer::FindEntry(VAddr address) const {
    return std::find_if(entries.begin(), entries.end(),
                        [address](const Entry& entry) { return entry.address == address; });
}

void Freezer::FrameCallback(std::uintptr_t, std::chrono::nanoseconds ns_late) {
    if (!IsActive()) {
        LOG_DEBUG(Common_Memory, "Memory freezer has been deactivated, ending callback events.");
        return;
    }

    std::scoped_lock lock{entries_mutex};

    for (const auto& entry : entries) {
        LOG_DEBUG(Common_Memory,
                  "Enforcing memory freeze at address={:016X}, value={:016X}, width={:02X}",
                  entry.address, entry.value, entry.width);
        MemoryWriteWidth(memory, entry.width, entry.address, entry.value);
    }

    core_timing.ScheduleEvent(memory_freezer_ns - ns_late, event);
}

void Freezer::FillEntryReads() {
    std::scoped_lock lock{entries_mutex};

    LOG_DEBUG(Common_Memory, "Updating memory freeze entries to current values.");

    for (auto& entry : entries) {
        entry.value = MemoryReadWidth(memory, entry.width, entry.address);
    }
}

} // namespace Tools
