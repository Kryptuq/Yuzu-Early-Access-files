// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <cstddef>
#include <vector>

#include "common/common_types.h"
#include "core/hle/kernel/physical_memory.h"

namespace Kernel {

/**
 * Represents executable data that may be loaded into a kernel process.
 *
 * A code set consists of three basic segments:
 *   - A code (AKA text) segment,
 *   - A read-only data segment (rodata)
 *   - A data segment
 *
 * The code segment is the portion of the object file that contains
 * executable instructions.
 *
 * The read-only data segment in the portion of the object file that
 * contains (as one would expect) read-only data, such as fixed constant
 * values and data structures.
 *
 * The data segment is similar to the read-only data segment -- it contains
 * variables and data structures that have predefined values, however,
 * entities within this segment can be modified.
 */
struct CodeSet final {
    /// A single segment within a code set.
    struct Segment final {
        /// The byte offset that this segment is located at.
        std::size_t offset = 0;

        /// The address to map this segment to.
        VAddr addr = 0;

        /// The size of this segment in bytes.
        u32 size = 0;
    };

    explicit CodeSet();
    ~CodeSet();

    CodeSet(const CodeSet&) = delete;
    CodeSet& operator=(const CodeSet&) = delete;

    CodeSet(CodeSet&&) = default;
    CodeSet& operator=(CodeSet&&) = default;

    Segment& CodeSegment() {
        return segments[0];
    }

    const Segment& CodeSegment() const {
        return segments[0];
    }

    Segment& RODataSegment() {
        return segments[1];
    }

    const Segment& RODataSegment() const {
        return segments[1];
    }

    Segment& DataSegment() {
        return segments[2];
    }

    const Segment& DataSegment() const {
        return segments[2];
    }

    /// The overall data that backs this code set.
    Kernel::PhysicalMemory memory;

    /// The segments that comprise this code set.
    std::array<Segment, 3> segments;

    /// The entry point address for this code set.
    VAddr entrypoint = 0;
};

} // namespace Kernel
