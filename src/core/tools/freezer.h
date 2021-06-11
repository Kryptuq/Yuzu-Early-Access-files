// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>
#include "common/common_types.h"

namespace Core::Timing {
class CoreTiming;
struct EventType;
} // namespace Core::Timing

namespace Core::Memory {
class Memory;
}

namespace Tools {

/**
 * This class allows the user to prevent an application from writing new values to certain memory
 * locations. This has a variety of uses when attempting to reverse a game.
 *
 * One example could be a cheat to prevent Mario from taking damage in SMO. One could freeze the
 * memory address that the game uses to store Mario's health so when he takes damage (and the game
 * tries to write the new health value to memory), the value won't change.
 */
class Freezer {
public:
    struct Entry {
        VAddr address;
        u32 width;
        u64 value;
    };

    explicit Freezer(Core::Timing::CoreTiming& core_timing_, Core::Memory::Memory& memory_);
    ~Freezer();

    // Enables or disables the entire memory freezer.
    void SetActive(bool is_active);

    // Returns whether or not the freezer is active.
    bool IsActive() const;

    // Removes all entries from the freezer.
    void Clear();

    // Freezes a value to its current memory address. The value the memory is kept at will be the
    // value that is read during this function. Width can be 1, 2, 4, or 8 (in bytes).
    u64 Freeze(VAddr address, u32 width);

    // Unfreezes the memory value at address. If the address isn't frozen, this is a no-op.
    void Unfreeze(VAddr address);

    // Returns whether or not the address is frozen.
    bool IsFrozen(VAddr address) const;

    // Sets the value that address should be frozen to. This doesn't change the width set by using
    // Freeze(). If the value isn't frozen, this will not freeze it and is thus a no-op.
    void SetFrozenValue(VAddr address, u64 value);

    // Returns the entry corresponding to the address if the address is frozen, otherwise
    // std::nullopt.
    std::optional<Entry> GetEntry(VAddr address) const;

    // Returns all the entries in the freezer, an empty vector means nothing is frozen.
    std::vector<Entry> GetEntries() const;

private:
    using Entries = std::vector<Entry>;

    Entries::iterator FindEntry(VAddr address);
    Entries::const_iterator FindEntry(VAddr address) const;

    void FrameCallback(std::uintptr_t user_data, std::chrono::nanoseconds ns_late);
    void FillEntryReads();

    std::atomic_bool active{false};

    mutable std::mutex entries_mutex;
    Entries entries;

    std::shared_ptr<Core::Timing::EventType> event;
    Core::Timing::CoreTiming& core_timing;
    Core::Memory::Memory& memory;
};

} // namespace Tools
