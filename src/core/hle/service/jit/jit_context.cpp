// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <array>
#include <map>
#include <span>
#include <boost/icl/interval_set.hpp>
#include <dynarmic/interface/A64/a64.h>
#include <dynarmic/interface/A64/config.h>

#include "common/alignment.h"
#include "common/common_funcs.h"
#include "common/div_ceil.h"
#include "common/elf.h"
#include "common/logging/log.h"
#include "core/hle/service/jit/jit_context.h"
#include "core/memory.h"

using namespace Common::ELF;

namespace Service::JIT {

constexpr std::array<u8, 8> SVC0_ARM64 = {
    0x01, 0x00, 0x00, 0xd4, // svc  #0
    0xc0, 0x03, 0x5f, 0xd6, // ret
};

constexpr std::array HELPER_FUNCTIONS{
    "_stop", "_resolve", "_panic", "memcpy", "memmove", "memset",
};

constexpr size_t STACK_ALIGN = 16;

class JITContextImpl;

using IntervalSet = boost::icl::interval_set<VAddr>::type;
using IntervalType = boost::icl::interval_set<VAddr>::interval_type;

class DynarmicCallbacks64 : public Dynarmic::A64::UserCallbacks {
public:
    explicit DynarmicCallbacks64(Core::Memory::Memory& memory_, std::vector<u8>& local_memory_,
                                 IntervalSet& mapped_ranges_, JITContextImpl& parent_)
        : memory{memory_}, local_memory{local_memory_},
          mapped_ranges{mapped_ranges_}, parent{parent_} {}

    u8 MemoryRead8(u64 vaddr) override {
        return ReadMemory<u8>(vaddr);
    }
    u16 MemoryRead16(u64 vaddr) override {
        return ReadMemory<u16>(vaddr);
    }
    u32 MemoryRead32(u64 vaddr) override {
        return ReadMemory<u32>(vaddr);
    }
    u64 MemoryRead64(u64 vaddr) override {
        return ReadMemory<u64>(vaddr);
    }
    u128 MemoryRead128(u64 vaddr) override {
        return ReadMemory<u128>(vaddr);
    }
    std::string MemoryReadCString(u64 vaddr) {
        std::string result;
        u8 next;

        while ((next = MemoryRead8(vaddr++)) != 0) {
            result += next;
        }

        return result;
    }

    void MemoryWrite8(u64 vaddr, u8 value) override {
        WriteMemory<u8>(vaddr, value);
    }
    void MemoryWrite16(u64 vaddr, u16 value) override {
        WriteMemory<u16>(vaddr, value);
    }
    void MemoryWrite32(u64 vaddr, u32 value) override {
        WriteMemory<u32>(vaddr, value);
    }
    void MemoryWrite64(u64 vaddr, u64 value) override {
        WriteMemory<u64>(vaddr, value);
    }
    void MemoryWrite128(u64 vaddr, u128 value) override {
        WriteMemory<u128>(vaddr, value);
    }

    bool MemoryWriteExclusive8(u64 vaddr, u8 value, u8) override {
        return WriteMemory<u8>(vaddr, value);
    }
    bool MemoryWriteExclusive16(u64 vaddr, u16 value, u16) override {
        return WriteMemory<u16>(vaddr, value);
    }
    bool MemoryWriteExclusive32(u64 vaddr, u32 value, u32) override {
        return WriteMemory<u32>(vaddr, value);
    }
    bool MemoryWriteExclusive64(u64 vaddr, u64 value, u64) override {
        return WriteMemory<u64>(vaddr, value);
    }
    bool MemoryWriteExclusive128(u64 vaddr, u128 value, u128) override {
        return WriteMemory<u128>(vaddr, value);
    }

    void CallSVC(u32 swi) override;
    void ExceptionRaised(u64 pc, Dynarmic::A64::Exception exception) override;
    void InterpreterFallback(u64 pc, size_t num_instructions) override;

    void AddTicks(u64 ticks) override {}
    u64 GetTicksRemaining() override {
        return std::numeric_limits<u32>::max();
    }
    u64 GetCNTPCT() override {
        return 0;
    }

    template <class T>
    T ReadMemory(u64 vaddr) {
        T ret{};
        if (boost::icl::contains(mapped_ranges, vaddr)) {
            memory.ReadBlock(vaddr, &ret, sizeof(T));
        } else if (vaddr + sizeof(T) > local_memory.size()) {
            LOG_CRITICAL(Service_JIT, "plugin: unmapped read @ 0x{:016x}", vaddr);
        } else {
            std::memcpy(&ret, local_memory.data() + vaddr, sizeof(T));
        }
        return ret;
    }

    template <class T>
    bool WriteMemory(u64 vaddr, const T value) {
        if (boost::icl::contains(mapped_ranges, vaddr)) {
            memory.WriteBlock(vaddr, &value, sizeof(T));
        } else if (vaddr + sizeof(T) > local_memory.size()) {
            LOG_CRITICAL(Service_JIT, "plugin: unmapped write @ 0x{:016x}", vaddr);
        } else {
            std::memcpy(local_memory.data() + vaddr, &value, sizeof(T));
        }
        return true;
    }

private:
    Core::Memory::Memory& memory;
    std::vector<u8>& local_memory;
    IntervalSet& mapped_ranges;
    JITContextImpl& parent;
};

class JITContextImpl {
public:
    explicit JITContextImpl(Core::Memory::Memory& memory_) : memory{memory_} {
        callbacks =
            std::make_unique<DynarmicCallbacks64>(memory, local_memory, mapped_ranges, *this);
        user_config.callbacks = callbacks.get();
        jit = std::make_unique<Dynarmic::A64::Jit>(user_config);
    }

    bool LoadNRO(std::span<const u8> data) {
        local_memory.clear();
        local_memory.insert(local_memory.end(), data.begin(), data.end());

        if (FixupRelocations()) {
            InsertHelperFunctions();
            InsertStack();
            return true;
        }

        return false;
    }

    bool FixupRelocations() {
        // The loaded NRO file has ELF relocations that must be processed before it can run.
        // Normally this would be processed by RTLD, but in HLE context, we don't have
        // the linker available, so we have to do it ourselves.

        const VAddr mod_offset{callbacks->MemoryRead32(4)};
        if (callbacks->MemoryRead32(mod_offset) != Common::MakeMagic('M', 'O', 'D', '0')) {
            return false;
        }

        // For more info about dynamic entries, see the ELF ABI specification:
        // https://refspecs.linuxbase.org/elf/gabi4+/ch5.dynamic.html
        // https://refspecs.linuxbase.org/elf/gabi4+/ch4.reloc.html
        VAddr dynamic_offset{mod_offset + callbacks->MemoryRead32(mod_offset + 4)};
        VAddr rela_dyn = 0;
        size_t num_rela = 0;
        while (true) {
            const auto dyn{callbacks->ReadMemory<Elf64_Dyn>(dynamic_offset)};
            dynamic_offset += sizeof(Elf64_Dyn);

            if (!dyn.d_tag) {
                break;
            }
            if (dyn.d_tag == ElfDtRela) {
                rela_dyn = dyn.d_un.d_ptr;
            }
            if (dyn.d_tag == ElfDtRelasz) {
                num_rela = dyn.d_un.d_val / sizeof(Elf64_Rela);
            }
        }

        for (size_t i = 0; i < num_rela; i++) {
            const auto rela{callbacks->ReadMemory<Elf64_Rela>(rela_dyn + i * sizeof(Elf64_Rela))};
            if (Elf64RelType(rela.r_info) != ElfAArch64Relative) {
                continue;
            }
            const VAddr contents{callbacks->MemoryRead64(rela.r_offset)};
            callbacks->MemoryWrite64(rela.r_offset, contents + rela.r_addend);
        }

        return true;
    }

    void InsertHelperFunctions() {
        for (const auto& name : HELPER_FUNCTIONS) {
            helpers[name] = local_memory.size();
            local_memory.insert(local_memory.end(), SVC0_ARM64.begin(), SVC0_ARM64.end());
        }
    }

    void InsertStack() {
        // Allocate enough space to avoid any reasonable risk of
        // overflowing the stack during plugin execution
        const u64 pad_amount{Common::AlignUp(local_memory.size(), STACK_ALIGN) -
                             local_memory.size()};
        local_memory.insert(local_memory.end(), 0x10000 + pad_amount, 0);
        top_of_stack = local_memory.size();
        heap_pointer = top_of_stack;
    }

    void MapProcessMemory(VAddr dest_address, std::size_t size) {
        mapped_ranges.add(IntervalType{dest_address, dest_address + size});
    }

    void PushArgument(const void* data, size_t size) {
        const size_t num_words = Common::DivCeil(size, sizeof(u64));
        const size_t current_pos = argument_stack.size();
        argument_stack.insert(argument_stack.end(), num_words, 0);
        std::memcpy(argument_stack.data() + current_pos, data, size);
    }

    void SetupArguments() {
        // The first 8 integer registers are used for the first 8 integer
        // arguments. Floating-point arguments are not handled at this time.
        //
        // If a function takes more than 8 arguments, then stack space is reserved
        // for the remaining arguments, and the remaining arguments are inserted in
        // ascending memory order, each argument aligned to an 8-byte boundary. The
        // stack pointer must remain aligned to 16 bytes.
        //
        // For more info, see the AArch64 ABI PCS:
        // https://github.com/ARM-software/abi-aa/blob/main/aapcs64/aapcs64.rst

        for (size_t i = 0; i < 8 && i < argument_stack.size(); i++) {
            jit->SetRegister(i, argument_stack[i]);
        }

        if (argument_stack.size() > 8) {
            const VAddr new_sp = Common::AlignDown(
                top_of_stack - (argument_stack.size() - 8) * sizeof(u64), STACK_ALIGN);
            for (size_t i = 8; i < argument_stack.size(); i++) {
                callbacks->MemoryWrite64(new_sp + (i - 8) * sizeof(u64), argument_stack[i]);
            }
            jit->SetSP(new_sp);
        }

        // Reset the call state for the next invocation
        argument_stack.clear();
        heap_pointer = top_of_stack;
    }

    u64 CallFunction(VAddr func) {
        jit->SetRegister(30, helpers["_stop"]);
        jit->SetSP(top_of_stack);
        SetupArguments();

        jit->SetPC(func);
        jit->Run();
        return jit->GetRegister(0);
    }

    VAddr GetHelper(const std::string& name) {
        return helpers[name];
    }

    VAddr AddHeap(const void* data, size_t size) {
        // Require all heap data types to have the same alignment as the
        // stack pointer, for compatibility
        const size_t num_bytes{Common::AlignUp(size, STACK_ALIGN)};

        // Make additional memory space if required
        if (heap_pointer + num_bytes > local_memory.size()) {
            local_memory.insert(local_memory.end(),
                                (heap_pointer + num_bytes) - local_memory.size(), 0);
        }

        const VAddr location{heap_pointer};
        std::memcpy(local_memory.data() + location, data, size);
        heap_pointer += num_bytes;
        return location;
    }

    void GetHeap(VAddr location, void* data, size_t size) {
        std::memcpy(data, local_memory.data() + location, size);
    }

    std::unique_ptr<DynarmicCallbacks64> callbacks;
    std::vector<u8> local_memory;
    std::vector<u64> argument_stack;
    IntervalSet mapped_ranges;
    Dynarmic::A64::UserConfig user_config;
    std::unique_ptr<Dynarmic::A64::Jit> jit;
    std::map<std::string, VAddr, std::less<>> helpers;
    Core::Memory::Memory& memory;
    VAddr top_of_stack;
    VAddr heap_pointer;
};

void DynarmicCallbacks64::CallSVC(u32 swi) {
    // Service calls are used to implement helper functionality.
    //
    // The most important of these is the _stop helper, which transfers control
    // from the plugin back to HLE context to return a value. However, a few more
    // are also implemented to reduce the need for direct ARM implementations of
    // basic functionality, like memory operations.
    //
    // When we receive a helper request, the swi number will be zero, and the call
    // will have originated from an address we know is a helper function. Otherwise,
    // the plugin may be trying to issue a service call, which we shouldn't handle.

    if (swi != 0) {
        LOG_CRITICAL(Service_JIT, "plugin issued unknown service call {}", swi);
        parent.jit->HaltExecution();
        return;
    }

    u64 pc{parent.jit->GetPC() - 4};
    auto& helpers{parent.helpers};

    if (pc == helpers["memcpy"] || pc == helpers["memmove"]) {
        const VAddr dest{parent.jit->GetRegister(0)};
        const VAddr src{parent.jit->GetRegister(1)};
        const size_t n{parent.jit->GetRegister(2)};

        if (dest < src) {
            for (size_t i = 0; i < n; i++) {
                MemoryWrite8(dest + i, MemoryRead8(src + i));
            }
        } else {
            for (size_t i = n; i > 0; i--) {
                MemoryWrite8(dest + i - 1, MemoryRead8(src + i - 1));
            }
        }
    } else if (pc == helpers["memset"]) {
        const VAddr dest{parent.jit->GetRegister(0)};
        const u64 c{parent.jit->GetRegister(1)};
        const size_t n{parent.jit->GetRegister(2)};

        for (size_t i = 0; i < n; i++) {
            MemoryWrite8(dest + i, static_cast<u8>(c));
        }
    } else if (pc == helpers["_resolve"]) {
        // X0 contains a char* for a symbol to resolve
        const auto name{MemoryReadCString(parent.jit->GetRegister(0))};
        const auto helper{helpers[name]};

        if (helper != 0) {
            parent.jit->SetRegister(0, helper);
        } else {
            LOG_WARNING(Service_JIT, "plugin requested unknown function {}", name);
            parent.jit->SetRegister(0, helpers["_panic"]);
        }
    } else if (pc == helpers["_stop"]) {
        parent.jit->HaltExecution();
    } else if (pc == helpers["_panic"]) {
        LOG_CRITICAL(Service_JIT, "plugin panicked!");
        parent.jit->HaltExecution();
    } else {
        LOG_CRITICAL(Service_JIT, "plugin issued syscall at unknown address 0x{:x}", pc);
        parent.jit->HaltExecution();
    }
}

void DynarmicCallbacks64::ExceptionRaised(u64 pc, Dynarmic::A64::Exception exception) {
    LOG_CRITICAL(Service_JIT, "Illegal operation PC @ {:08x}", pc);
    parent.jit->HaltExecution();
}

void DynarmicCallbacks64::InterpreterFallback(u64 pc, size_t num_instructions) {
    LOG_CRITICAL(Service_JIT, "Unimplemented instruction PC @ {:08x}", pc);
    parent.jit->HaltExecution();
}

JITContext::JITContext(Core::Memory::Memory& memory)
    : impl{std::make_unique<JITContextImpl>(memory)} {}

JITContext::~JITContext() {}

bool JITContext::LoadNRO(std::span<const u8> data) {
    return impl->LoadNRO(data);
}

void JITContext::MapProcessMemory(VAddr dest_address, std::size_t size) {
    impl->MapProcessMemory(dest_address, size);
}

u64 JITContext::CallFunction(VAddr func) {
    return impl->CallFunction(func);
}

void JITContext::PushArgument(const void* data, size_t size) {
    impl->PushArgument(data, size);
}

VAddr JITContext::GetHelper(const std::string& name) {
    return impl->GetHelper(name);
}

VAddr JITContext::AddHeap(const void* data, size_t size) {
    return impl->AddHeap(data, size);
}

void JITContext::GetHeap(VAddr location, void* data, size_t size) {
    impl->GetHeap(location, data, size);
}

} // namespace Service::JIT
