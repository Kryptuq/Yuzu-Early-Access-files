// Copyright 2022 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <array>
#include <map>
#include <span>
#include <boost/icl/interval_set.hpp>
#include <dynarmic/interface/A64/a64.h>
#include <dynarmic/interface/A64/config.h>

#include "common/alignment.h"
#include "common/common_funcs.h"
#include "common/div_ceil.h"
#include "common/logging/log.h"
#include "core/hle/service/jit/jit_context.h"
#include "core/memory.h"

namespace Service::JIT {

constexpr std::array<u8, 4> STOP_ARM64 = {
    0x01, 0x00, 0x00, 0xd4, // svc  #0
};

constexpr std::array<u8, 8> RESOLVE_ARM64 = {
    0x21, 0x00, 0x00, 0xd4, // svc  #1
    0xc0, 0x03, 0x5f, 0xd6, // ret
};

constexpr std::array<u8, 4> PANIC_ARM64 = {
    0x41, 0x00, 0x00, 0xd4, // svc  #2
};

constexpr std::array<u8, 60> MEMMOVE_ARM64 = {
    0x1f, 0x00, 0x01, 0xeb, // cmp  x0, x1
    0x83, 0x01, 0x00, 0x54, // b.lo #+34
    0x42, 0x04, 0x00, 0xd1, // sub  x2, x2, 1
    0x22, 0x01, 0xf8, 0xb7, // tbnz x2, #63, #+36
    0x23, 0x68, 0x62, 0x38, // ldrb w3, [x1, x2]
    0x03, 0x68, 0x22, 0x38, // strb w3, [x0, x2]
    0xfc, 0xff, 0xff, 0x17, // b    #-16
    0x24, 0x68, 0x63, 0x38, // ldrb w4, [x1, x3]
    0x04, 0x68, 0x23, 0x38, // strb w4, [x0, x3]
    0x63, 0x04, 0x00, 0x91, // add  x3, x3, 1
    0x7f, 0x00, 0x02, 0xeb, // cmp  x3, x2
    0x8b, 0xff, 0xff, 0x54, // b.lt #-16
    0xc0, 0x03, 0x5f, 0xd6, // ret
    0x03, 0x00, 0x80, 0xd2, // mov  x3, 0
    0xfc, 0xff, 0xff, 0x17, // b    #-16
};

constexpr std::array<u8, 28> MEMSET_ARM64 = {
    0x03, 0x00, 0x80, 0xd2, // mov  x3, 0
    0x7f, 0x00, 0x02, 0xeb, // cmp  x3, x2
    0x4b, 0x00, 0x00, 0x54, // b.lt #+8
    0xc0, 0x03, 0x5f, 0xd6, // ret
    0x01, 0x68, 0x23, 0x38, // strb w1, [x0, x3]
    0x63, 0x04, 0x00, 0x91, // add  x3, x3, 1
    0xfb, 0xff, 0xff, 0x17, // b    #-20
};

struct HelperFunction {
    const char* name;
    const std::span<const u8> data;
};

constexpr std::array<HelperFunction, 6> HELPER_FUNCTIONS{{
    {"_stop", STOP_ARM64},
    {"_resolve", RESOLVE_ARM64},
    {"_panic", PANIC_ARM64},
    {"memcpy", MEMMOVE_ARM64},
    {"memmove", MEMMOVE_ARM64},
    {"memset", MEMSET_ARM64},
}};

struct Elf64_Dyn {
    u64 d_tag;
    u64 d_un;
};

struct Elf64_Rela {
    u64 r_offset;
    u64 r_info;
    s64 r_addend;
};

static constexpr u32 Elf64_RelaType(const Elf64_Rela* rela) {
    return static_cast<u32>(rela->r_info);
}

constexpr int DT_RELA = 7;               /* Address of Rela relocs */
constexpr int DT_RELASZ = 8;             /* Total size of Rela relocs */
constexpr int R_AARCH64_RELATIVE = 1027; /* Adjust by program base.  */

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
        } else {
            return false;
        }
    }

    bool FixupRelocations() {
        const VAddr mod_offset{callbacks->MemoryRead32(4)};
        if (callbacks->MemoryRead32(mod_offset) != Common::MakeMagic('M', 'O', 'D', '0')) {
            return false;
        }

        VAddr dynamic_offset{mod_offset + callbacks->MemoryRead32(mod_offset + 4)};
        VAddr rela_dyn = 0;
        size_t num_rela = 0;
        while (true) {
            const auto dyn{callbacks->ReadMemory<Elf64_Dyn>(dynamic_offset)};
            dynamic_offset += sizeof(Elf64_Dyn);

            if (!dyn.d_tag) {
                break;
            }
            if (dyn.d_tag == DT_RELA) {
                rela_dyn = dyn.d_un;
            }
            if (dyn.d_tag == DT_RELASZ) {
                num_rela = dyn.d_un / sizeof(Elf64_Rela);
            }
        }

        for (size_t i = 0; i < num_rela; i++) {
            const auto rela{callbacks->ReadMemory<Elf64_Rela>(rela_dyn + i * sizeof(Elf64_Rela))};
            if (Elf64_RelaType(&rela) != R_AARCH64_RELATIVE) {
                continue;
            }
            const VAddr contents{callbacks->MemoryRead64(rela.r_offset)};
            callbacks->MemoryWrite64(rela.r_offset, contents + rela.r_addend);
        }

        return true;
    }

    void InsertHelperFunctions() {
        for (const auto& [name, contents] : HELPER_FUNCTIONS) {
            helpers[name] = local_memory.size();
            local_memory.insert(local_memory.end(), contents.begin(), contents.end());
        }
    }

    void InsertStack() {
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
        const size_t num_bytes{Common::AlignUp(size, STACK_ALIGN)};
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
    switch (swi) {
    case 0:
        parent.jit->HaltExecution();
        break;

    case 1: {
        // X0 contains a char* for a symbol to resolve
        std::string name{MemoryReadCString(parent.jit->GetRegister(0))};
        const auto helper{parent.helpers[name]};

        if (helper != 0) {
            parent.jit->SetRegister(0, helper);
        } else {
            LOG_WARNING(Service_JIT, "plugin requested unknown function {}", name);
            parent.jit->SetRegister(0, parent.helpers["_panic"]);
        }
        break;
    }

    case 2:
    default:
        LOG_CRITICAL(Service_JIT, "plugin panicked!");
        parent.jit->HaltExecution();
        break;
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
