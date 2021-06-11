/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <array>
#include <map>

#include "dynarmic/common/assert.h"
#include "dynarmic/common/common_types.h"
#include "dynarmic/interface/A64/a64.h"

using Vector = Dynarmic::A64::Vector;

class A64TestEnv : public Dynarmic::A64::UserCallbacks {
public:
    u64 ticks_left = 0;

    bool code_mem_modified_by_guest = false;
    u64 code_mem_start_address = 0;
    std::vector<u32> code_mem;

    std::map<u64, u8> modified_memory;
    std::vector<std::string> interrupts;

    bool IsInCodeMem(u64 vaddr) const {
        return vaddr >= code_mem_start_address && vaddr < code_mem_start_address + code_mem.size() * 4;
    }

    std::uint32_t MemoryReadCode(u64 vaddr) override {
        if (!IsInCodeMem(vaddr)) {
            return 0x14000000;  // B .
        }

        const size_t index = (vaddr - code_mem_start_address) / 4;
        return code_mem[index];
    }

    std::uint8_t MemoryRead8(u64 vaddr) override {
        if (IsInCodeMem(vaddr)) {
            return reinterpret_cast<u8*>(code_mem.data())[vaddr - code_mem_start_address];
        }
        if (auto iter = modified_memory.find(vaddr); iter != modified_memory.end()) {
            return iter->second;
        }
        return static_cast<u8>(vaddr);
    }
    std::uint16_t MemoryRead16(u64 vaddr) override {
        return u16(MemoryRead8(vaddr)) | u16(MemoryRead8(vaddr + 1)) << 8;
    }
    std::uint32_t MemoryRead32(u64 vaddr) override {
        return u32(MemoryRead16(vaddr)) | u32(MemoryRead16(vaddr + 2)) << 16;
    }
    std::uint64_t MemoryRead64(u64 vaddr) override {
        return u64(MemoryRead32(vaddr)) | u64(MemoryRead32(vaddr + 4)) << 32;
    }
    Vector MemoryRead128(u64 vaddr) override {
        return {MemoryRead64(vaddr), MemoryRead64(vaddr + 8)};
    }

    void MemoryWrite8(u64 vaddr, std::uint8_t value) override {
        if (IsInCodeMem(vaddr)) {
            code_mem_modified_by_guest = true;
        }
        modified_memory[vaddr] = value;
    }
    void MemoryWrite16(u64 vaddr, std::uint16_t value) override {
        MemoryWrite8(vaddr, static_cast<u8>(value));
        MemoryWrite8(vaddr + 1, static_cast<u8>(value >> 8));
    }
    void MemoryWrite32(u64 vaddr, std::uint32_t value) override {
        MemoryWrite16(vaddr, static_cast<u16>(value));
        MemoryWrite16(vaddr + 2, static_cast<u16>(value >> 16));
    }
    void MemoryWrite64(u64 vaddr, std::uint64_t value) override {
        MemoryWrite32(vaddr, static_cast<u32>(value));
        MemoryWrite32(vaddr + 4, static_cast<u32>(value >> 32));
    }
    void MemoryWrite128(u64 vaddr, Vector value) override {
        MemoryWrite64(vaddr, value[0]);
        MemoryWrite64(vaddr + 8, value[1]);
    }

    bool MemoryWriteExclusive8(u64 vaddr, std::uint8_t value, [[maybe_unused]] std::uint8_t expected) override {
        MemoryWrite8(vaddr, value);
        return true;
    }
    bool MemoryWriteExclusive16(u64 vaddr, std::uint16_t value, [[maybe_unused]] std::uint16_t expected) override {
        MemoryWrite16(vaddr, value);
        return true;
    }
    bool MemoryWriteExclusive32(u64 vaddr, std::uint32_t value, [[maybe_unused]] std::uint32_t expected) override {
        MemoryWrite32(vaddr, value);
        return true;
    }
    bool MemoryWriteExclusive64(u64 vaddr, std::uint64_t value, [[maybe_unused]] std::uint64_t expected) override {
        MemoryWrite64(vaddr, value);
        return true;
    }
    bool MemoryWriteExclusive128(u64 vaddr, Vector value, [[maybe_unused]] Vector expected) override {
        MemoryWrite128(vaddr, value);
        return true;
    }

    void InterpreterFallback(u64 pc, size_t num_instructions) override { ASSERT_MSG(false, "InterpreterFallback({:016x}, {})", pc, num_instructions); }

    void CallSVC(std::uint32_t swi) override { ASSERT_MSG(false, "CallSVC({})", swi); }

    void ExceptionRaised(u64 pc, Dynarmic::A64::Exception /*exception*/) override { ASSERT_MSG(false, "ExceptionRaised({:016x})", pc); }

    void AddTicks(std::uint64_t ticks) override {
        if (ticks > ticks_left) {
            ticks_left = 0;
            return;
        }
        ticks_left -= ticks;
    }
    std::uint64_t GetTicksRemaining() override {
        return ticks_left;
    }
    std::uint64_t GetCNTPCT() override {
        return 0x10000000000 - ticks_left;
    }
};

class A64FastmemTestEnv final : public Dynarmic::A64::UserCallbacks {
public:
    u64 ticks_left = 0;
    char* backing_memory = nullptr;

    explicit A64FastmemTestEnv(char* addr)
            : backing_memory(addr) {}

    template<typename T>
    T read(u64 vaddr) {
        T value;
        memcpy(&value, backing_memory + vaddr, sizeof(T));
        return value;
    }
    template<typename T>
    void write(u64 vaddr, const T& value) {
        memcpy(backing_memory + vaddr, &value, sizeof(T));
    }

    std::uint32_t MemoryReadCode(u64 vaddr) override {
        return read<std::uint32_t>(vaddr);
    }

    std::uint8_t MemoryRead8(u64 vaddr) override {
        return read<std::uint8_t>(vaddr);
    }
    std::uint16_t MemoryRead16(u64 vaddr) override {
        return read<std::uint16_t>(vaddr);
    }
    std::uint32_t MemoryRead32(u64 vaddr) override {
        return read<std::uint32_t>(vaddr);
    }
    std::uint64_t MemoryRead64(u64 vaddr) override {
        return read<std::uint64_t>(vaddr);
    }
    Vector MemoryRead128(u64 vaddr) override {
        return read<Vector>(vaddr);
    }

    void MemoryWrite8(u64 vaddr, std::uint8_t value) override {
        write(vaddr, value);
    }
    void MemoryWrite16(u64 vaddr, std::uint16_t value) override {
        write(vaddr, value);
    }
    void MemoryWrite32(u64 vaddr, std::uint32_t value) override {
        write(vaddr, value);
    }
    void MemoryWrite64(u64 vaddr, std::uint64_t value) override {
        write(vaddr, value);
    }
    void MemoryWrite128(u64 vaddr, Vector value) override {
        write(vaddr, value);
    }

    bool MemoryWriteExclusive8(u64 vaddr, std::uint8_t value, [[maybe_unused]] std::uint8_t expected) override {
        MemoryWrite8(vaddr, value);
        return true;
    }
    bool MemoryWriteExclusive16(u64 vaddr, std::uint16_t value, [[maybe_unused]] std::uint16_t expected) override {
        MemoryWrite16(vaddr, value);
        return true;
    }
    bool MemoryWriteExclusive32(u64 vaddr, std::uint32_t value, [[maybe_unused]] std::uint32_t expected) override {
        MemoryWrite32(vaddr, value);
        return true;
    }
    bool MemoryWriteExclusive64(u64 vaddr, std::uint64_t value, [[maybe_unused]] std::uint64_t expected) override {
        MemoryWrite64(vaddr, value);
        return true;
    }
    bool MemoryWriteExclusive128(u64 vaddr, Vector value, [[maybe_unused]] Vector expected) override {
        MemoryWrite128(vaddr, value);
        return true;
    }

    void InterpreterFallback(u64 pc, size_t num_instructions) override { ASSERT_MSG(false, "InterpreterFallback({:016x}, {})", pc, num_instructions); }

    void CallSVC(std::uint32_t swi) override { ASSERT_MSG(false, "CallSVC({})", swi); }

    void ExceptionRaised(u64 pc, Dynarmic::A64::Exception) override { ASSERT_MSG(false, "ExceptionRaised({:016x})", pc); }

    void AddTicks(std::uint64_t ticks) override {
        if (ticks > ticks_left) {
            ticks_left = 0;
            return;
        }
        ticks_left -= ticks;
    }
    std::uint64_t GetTicksRemaining() override {
        return ticks_left;
    }
    std::uint64_t GetCNTPCT() override {
        return 0x10000000000 - ticks_left;
    }
};
