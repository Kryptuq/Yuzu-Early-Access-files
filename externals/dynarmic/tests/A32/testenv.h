/* This file is part of the dynarmic project.
 * Copyright (c) 2018 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <array>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include <mcl/assert.hpp>
#include <mcl/stdint.hpp>

#include "dynarmic/interface/A32/a32.h"

template<typename InstructionType_, u32 infinite_loop_u32>
class A32TestEnv final : public Dynarmic::A32::UserCallbacks {
public:
    using InstructionType = InstructionType_;
    using RegisterArray = std::array<u32, 16>;
    using ExtRegsArray = std::array<u32, 64>;

#ifdef _MSC_VER
#    pragma warning(push)
#    pragma warning(disable : 4309)  // C4309: 'static_cast': truncation of constant value
#endif
    static constexpr InstructionType infinite_loop = static_cast<InstructionType>(infinite_loop_u32);
#ifdef _MSC_VER
#    pragma warning(pop)
#endif

    u64 ticks_left = 0;
    bool code_mem_modified_by_guest = false;
    std::vector<InstructionType> code_mem;
    std::map<u32, u8> modified_memory;
    std::vector<std::string> interrupts;

    void PadCodeMem() {
        do {
            code_mem.push_back(infinite_loop);
        } while (code_mem.size() % 2 != 0);
    }

    bool IsInCodeMem(u32 vaddr) const {
        return vaddr < sizeof(InstructionType) * code_mem.size();
    }

    std::uint32_t MemoryReadCode(u32 vaddr) override {
        if (IsInCodeMem(vaddr)) {
            u32 value;
            std::memcpy(&value, &code_mem[vaddr / sizeof(InstructionType)], sizeof(u32));
            return value;
        }
        return infinite_loop_u32;  // B .
    }

    std::uint8_t MemoryRead8(u32 vaddr) override {
        if (IsInCodeMem(vaddr)) {
            return reinterpret_cast<u8*>(code_mem.data())[vaddr];
        }
        if (auto iter = modified_memory.find(vaddr); iter != modified_memory.end()) {
            return iter->second;
        }
        return static_cast<u8>(vaddr);
    }
    std::uint16_t MemoryRead16(u32 vaddr) override {
        return u16(MemoryRead8(vaddr)) | u16(MemoryRead8(vaddr + 1)) << 8;
    }
    std::uint32_t MemoryRead32(u32 vaddr) override {
        return u32(MemoryRead16(vaddr)) | u32(MemoryRead16(vaddr + 2)) << 16;
    }
    std::uint64_t MemoryRead64(u32 vaddr) override {
        return u64(MemoryRead32(vaddr)) | u64(MemoryRead32(vaddr + 4)) << 32;
    }

    void MemoryWrite8(u32 vaddr, std::uint8_t value) override {
        if (vaddr < code_mem.size() * sizeof(u32)) {
            code_mem_modified_by_guest = true;
        }
        modified_memory[vaddr] = value;
    }
    void MemoryWrite16(u32 vaddr, std::uint16_t value) override {
        MemoryWrite8(vaddr, static_cast<u8>(value));
        MemoryWrite8(vaddr + 1, static_cast<u8>(value >> 8));
    }
    void MemoryWrite32(u32 vaddr, std::uint32_t value) override {
        MemoryWrite16(vaddr, static_cast<u16>(value));
        MemoryWrite16(vaddr + 2, static_cast<u16>(value >> 16));
    }
    void MemoryWrite64(u32 vaddr, std::uint64_t value) override {
        MemoryWrite32(vaddr, static_cast<u32>(value));
        MemoryWrite32(vaddr + 4, static_cast<u32>(value >> 32));
    }

    void InterpreterFallback(u32 pc, size_t num_instructions) override { ASSERT_MSG(false, "InterpreterFallback({:08x}, {}) code = {:08x}", pc, num_instructions, MemoryReadCode(pc)); }

    void CallSVC(std::uint32_t swi) override { ASSERT_MSG(false, "CallSVC({})", swi); }

    void ExceptionRaised(u32 pc, Dynarmic::A32::Exception /*exception*/) override { ASSERT_MSG(false, "ExceptionRaised({:08x}) code = {:08x}", pc, MemoryReadCode(pc)); }

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
};

using ArmTestEnv = A32TestEnv<u32, 0xEAFFFFFE>;
using ThumbTestEnv = A32TestEnv<u16, 0xE7FEE7FE>;

class A32FastmemTestEnv final : public Dynarmic::A32::UserCallbacks {
public:
    u64 ticks_left = 0;
    char* backing_memory = nullptr;

    explicit A32FastmemTestEnv(char* addr)
            : backing_memory(addr) {}

    template<typename T>
    T read(std::uint32_t vaddr) {
        T value;
        memcpy(&value, backing_memory + vaddr, sizeof(T));
        return value;
    }
    template<typename T>
    void write(std::uint32_t vaddr, const T& value) {
        memcpy(backing_memory + vaddr, &value, sizeof(T));
    }

    std::uint32_t MemoryReadCode(std::uint32_t vaddr) override {
        return read<std::uint32_t>(vaddr);
    }

    std::uint8_t MemoryRead8(std::uint32_t vaddr) override {
        return read<std::uint8_t>(vaddr);
    }
    std::uint16_t MemoryRead16(std::uint32_t vaddr) override {
        return read<std::uint16_t>(vaddr);
    }
    std::uint32_t MemoryRead32(std::uint32_t vaddr) override {
        return read<std::uint32_t>(vaddr);
    }
    std::uint64_t MemoryRead64(std::uint32_t vaddr) override {
        return read<std::uint64_t>(vaddr);
    }

    void MemoryWrite8(std::uint32_t vaddr, std::uint8_t value) override {
        write(vaddr, value);
    }
    void MemoryWrite16(std::uint32_t vaddr, std::uint16_t value) override {
        write(vaddr, value);
    }
    void MemoryWrite32(std::uint32_t vaddr, std::uint32_t value) override {
        write(vaddr, value);
    }
    void MemoryWrite64(std::uint32_t vaddr, std::uint64_t value) override {
        write(vaddr, value);
    }

    bool MemoryWriteExclusive8(std::uint32_t vaddr, std::uint8_t value, [[maybe_unused]] std::uint8_t expected) override {
        MemoryWrite8(vaddr, value);
        return true;
    }
    bool MemoryWriteExclusive16(std::uint32_t vaddr, std::uint16_t value, [[maybe_unused]] std::uint16_t expected) override {
        MemoryWrite16(vaddr, value);
        return true;
    }
    bool MemoryWriteExclusive32(std::uint32_t vaddr, std::uint32_t value, [[maybe_unused]] std::uint32_t expected) override {
        MemoryWrite32(vaddr, value);
        return true;
    }
    bool MemoryWriteExclusive64(std::uint32_t vaddr, std::uint64_t value, [[maybe_unused]] std::uint64_t expected) override {
        MemoryWrite64(vaddr, value);
        return true;
    }

    void InterpreterFallback(std::uint32_t pc, size_t num_instructions) override { ASSERT_MSG(false, "InterpreterFallback({:016x}, {})", pc, num_instructions); }

    void CallSVC(std::uint32_t swi) override { ASSERT_MSG(false, "CallSVC({})", swi); }

    void ExceptionRaised(std::uint32_t pc, Dynarmic::A32::Exception) override { ASSERT_MSG(false, "ExceptionRaised({:016x})", pc); }

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
};
