// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/alignment.h"
#include "common/assert.h"
#include "common/common_types.h"
#include "core/hle/kernel/memory_types.h"
#include "core/hle/kernel/svc_types.h"

namespace Kernel {

enum class KMemoryState : u32 {
    None = 0,
    Mask = 0xFF,
    All = ~None,

    FlagCanReprotect = (1 << 8),
    FlagCanDebug = (1 << 9),
    FlagCanUseIpc = (1 << 10),
    FlagCanUseNonDeviceIpc = (1 << 11),
    FlagCanUseNonSecureIpc = (1 << 12),
    FlagMapped = (1 << 13),
    FlagCode = (1 << 14),
    FlagCanAlias = (1 << 15),
    FlagCanCodeAlias = (1 << 16),
    FlagCanTransfer = (1 << 17),
    FlagCanQueryPhysical = (1 << 18),
    FlagCanDeviceMap = (1 << 19),
    FlagCanAlignedDeviceMap = (1 << 20),
    FlagCanIpcUserBuffer = (1 << 21),
    FlagReferenceCounted = (1 << 22),
    FlagCanMapProcess = (1 << 23),
    FlagCanChangeAttribute = (1 << 24),
    FlagCanCodeMemory = (1 << 25),

    FlagsData = FlagCanReprotect | FlagCanUseIpc | FlagCanUseNonDeviceIpc | FlagCanUseNonSecureIpc |
                FlagMapped | FlagCanAlias | FlagCanTransfer | FlagCanQueryPhysical |
                FlagCanDeviceMap | FlagCanAlignedDeviceMap | FlagCanIpcUserBuffer |
                FlagReferenceCounted | FlagCanChangeAttribute,

    FlagsCode = FlagCanDebug | FlagCanUseIpc | FlagCanUseNonDeviceIpc | FlagCanUseNonSecureIpc |
                FlagMapped | FlagCode | FlagCanQueryPhysical | FlagCanDeviceMap |
                FlagCanAlignedDeviceMap | FlagReferenceCounted,

    FlagsMisc = FlagMapped | FlagReferenceCounted | FlagCanQueryPhysical | FlagCanDeviceMap,

    Free = static_cast<u32>(Svc::MemoryState::Free),
    Io = static_cast<u32>(Svc::MemoryState::Io) | FlagMapped,
    Static = static_cast<u32>(Svc::MemoryState::Static) | FlagMapped | FlagCanQueryPhysical,
    Code = static_cast<u32>(Svc::MemoryState::Code) | FlagsCode | FlagCanMapProcess,
    CodeData = static_cast<u32>(Svc::MemoryState::CodeData) | FlagsData | FlagCanMapProcess |
               FlagCanCodeMemory,
    Shared = static_cast<u32>(Svc::MemoryState::Shared) | FlagMapped | FlagReferenceCounted,
    Normal = static_cast<u32>(Svc::MemoryState::Normal) | FlagsData | FlagCanCodeMemory,

    AliasCode = static_cast<u32>(Svc::MemoryState::AliasCode) | FlagsCode | FlagCanMapProcess |
                FlagCanCodeAlias,
    AliasCodeData = static_cast<u32>(Svc::MemoryState::AliasCodeData) | FlagsData |
                    FlagCanMapProcess | FlagCanCodeAlias | FlagCanCodeMemory,

    Ipc = static_cast<u32>(Svc::MemoryState::Ipc) | FlagsMisc | FlagCanAlignedDeviceMap |
          FlagCanUseIpc | FlagCanUseNonSecureIpc | FlagCanUseNonDeviceIpc,

    Stack = static_cast<u32>(Svc::MemoryState::Stack) | FlagsMisc | FlagCanAlignedDeviceMap |
            FlagCanUseIpc | FlagCanUseNonSecureIpc | FlagCanUseNonDeviceIpc,

    ThreadLocal =
        static_cast<u32>(Svc::MemoryState::ThreadLocal) | FlagMapped | FlagReferenceCounted,

    Transfered = static_cast<u32>(Svc::MemoryState::Transferred) | FlagsMisc |
                 FlagCanAlignedDeviceMap | FlagCanChangeAttribute | FlagCanUseIpc |
                 FlagCanUseNonSecureIpc | FlagCanUseNonDeviceIpc,

    SharedTransfered = static_cast<u32>(Svc::MemoryState::SharedTransferred) | FlagsMisc |
                       FlagCanAlignedDeviceMap | FlagCanUseNonSecureIpc | FlagCanUseNonDeviceIpc,

    SharedCode = static_cast<u32>(Svc::MemoryState::SharedCode) | FlagMapped |
                 FlagReferenceCounted | FlagCanUseNonSecureIpc | FlagCanUseNonDeviceIpc,

    Inaccessible = static_cast<u32>(Svc::MemoryState::Inaccessible),

    NonSecureIpc = static_cast<u32>(Svc::MemoryState::NonSecureIpc) | FlagsMisc |
                   FlagCanAlignedDeviceMap | FlagCanUseNonSecureIpc | FlagCanUseNonDeviceIpc,

    NonDeviceIpc =
        static_cast<u32>(Svc::MemoryState::NonDeviceIpc) | FlagsMisc | FlagCanUseNonDeviceIpc,

    Kernel = static_cast<u32>(Svc::MemoryState::Kernel) | FlagMapped,

    GeneratedCode = static_cast<u32>(Svc::MemoryState::GeneratedCode) | FlagMapped |
                    FlagReferenceCounted | FlagCanDebug,
    CodeOut = static_cast<u32>(Svc::MemoryState::CodeOut) | FlagMapped | FlagReferenceCounted,

    Coverage = static_cast<u32>(Svc::MemoryState::Coverage) | FlagMapped,
};
DECLARE_ENUM_FLAG_OPERATORS(KMemoryState);

static_assert(static_cast<u32>(KMemoryState::Free) == 0x00000000);
static_assert(static_cast<u32>(KMemoryState::Io) == 0x00002001);
static_assert(static_cast<u32>(KMemoryState::Static) == 0x00042002);
static_assert(static_cast<u32>(KMemoryState::Code) == 0x00DC7E03);
static_assert(static_cast<u32>(KMemoryState::CodeData) == 0x03FEBD04);
static_assert(static_cast<u32>(KMemoryState::Normal) == 0x037EBD05);
static_assert(static_cast<u32>(KMemoryState::Shared) == 0x00402006);
static_assert(static_cast<u32>(KMemoryState::AliasCode) == 0x00DD7E08);
static_assert(static_cast<u32>(KMemoryState::AliasCodeData) == 0x03FFBD09);
static_assert(static_cast<u32>(KMemoryState::Ipc) == 0x005C3C0A);
static_assert(static_cast<u32>(KMemoryState::Stack) == 0x005C3C0B);
static_assert(static_cast<u32>(KMemoryState::ThreadLocal) == 0x0040200C);
static_assert(static_cast<u32>(KMemoryState::Transfered) == 0x015C3C0D);
static_assert(static_cast<u32>(KMemoryState::SharedTransfered) == 0x005C380E);
static_assert(static_cast<u32>(KMemoryState::SharedCode) == 0x0040380F);
static_assert(static_cast<u32>(KMemoryState::Inaccessible) == 0x00000010);
static_assert(static_cast<u32>(KMemoryState::NonSecureIpc) == 0x005C3811);
static_assert(static_cast<u32>(KMemoryState::NonDeviceIpc) == 0x004C2812);
static_assert(static_cast<u32>(KMemoryState::Kernel) == 0x00002013);
static_assert(static_cast<u32>(KMemoryState::GeneratedCode) == 0x00402214);
static_assert(static_cast<u32>(KMemoryState::CodeOut) == 0x00402015);
static_assert(static_cast<u32>(KMemoryState::Coverage) == 0x00002016);

enum class KMemoryPermission : u8 {
    None = 0,
    All = static_cast<u8>(~None),

    Read = 1 << 0,
    Write = 1 << 1,
    Execute = 1 << 2,

    ReadAndWrite = Read | Write,
    ReadAndExecute = Read | Execute,

    UserMask = static_cast<u8>(Svc::MemoryPermission::Read | Svc::MemoryPermission::Write |
                               Svc::MemoryPermission::Execute),

    KernelShift = 3,

    KernelRead = Read << KernelShift,
    KernelWrite = Write << KernelShift,
    KernelExecute = Execute << KernelShift,

    NotMapped = (1 << (2 * KernelShift)),

    KernelReadWrite = KernelRead | KernelWrite,
    KernelReadExecute = KernelRead | KernelExecute,

    UserRead = Read | KernelRead,
    UserWrite = Write | KernelWrite,
    UserExecute = Execute,

    UserReadWrite = UserRead | UserWrite,
    UserReadExecute = UserRead | UserExecute,

    IpcLockChangeMask = NotMapped | UserReadWrite
};
DECLARE_ENUM_FLAG_OPERATORS(KMemoryPermission);

constexpr KMemoryPermission ConvertToKMemoryPermission(Svc::MemoryPermission perm) {
    return static_cast<KMemoryPermission>(
        (static_cast<KMemoryPermission>(perm) & KMemoryPermission::UserMask) |
        KMemoryPermission::KernelRead |
        ((static_cast<KMemoryPermission>(perm) & KMemoryPermission::UserWrite)
         << KMemoryPermission::KernelShift) |
        (perm == Svc::MemoryPermission::None ? KMemoryPermission::NotMapped
                                             : KMemoryPermission::None));
}

enum class KMemoryAttribute : u8 {
    None = 0x00,
    Mask = 0x7F,
    All = Mask,
    DontCareMask = 0x80,

    Locked = static_cast<u8>(Svc::MemoryAttribute::Locked),
    IpcLocked = static_cast<u8>(Svc::MemoryAttribute::IpcLocked),
    DeviceShared = static_cast<u8>(Svc::MemoryAttribute::DeviceShared),
    Uncached = static_cast<u8>(Svc::MemoryAttribute::Uncached),

    SetMask = Uncached,

    IpcAndDeviceMapped = IpcLocked | DeviceShared,
    LockedAndIpcLocked = Locked | IpcLocked,
    DeviceSharedAndUncached = DeviceShared | Uncached
};
DECLARE_ENUM_FLAG_OPERATORS(KMemoryAttribute);

static_assert((static_cast<u8>(KMemoryAttribute::Mask) &
               static_cast<u8>(KMemoryAttribute::DontCareMask)) == 0);

struct KMemoryInfo {
    VAddr addr{};
    std::size_t size{};
    KMemoryState state{};
    KMemoryPermission perm{};
    KMemoryAttribute attribute{};
    KMemoryPermission original_perm{};
    u16 ipc_lock_count{};
    u16 device_use_count{};

    constexpr Svc::MemoryInfo GetSvcMemoryInfo() const {
        return {
            addr,
            size,
            static_cast<Svc::MemoryState>(state & KMemoryState::Mask),
            static_cast<Svc::MemoryAttribute>(attribute & KMemoryAttribute::Mask),
            static_cast<Svc::MemoryPermission>(perm & KMemoryPermission::UserMask),
            ipc_lock_count,
            device_use_count,
        };
    }

    constexpr VAddr GetAddress() const {
        return addr;
    }
    constexpr std::size_t GetSize() const {
        return size;
    }
    constexpr std::size_t GetNumPages() const {
        return GetSize() / PageSize;
    }
    constexpr VAddr GetEndAddress() const {
        return GetAddress() + GetSize();
    }
    constexpr VAddr GetLastAddress() const {
        return GetEndAddress() - 1;
    }
    constexpr KMemoryState GetState() const {
        return state;
    }
    constexpr KMemoryAttribute GetAttribute() const {
        return attribute;
    }
    constexpr KMemoryPermission GetPermission() const {
        return perm;
    }
};

class KMemoryBlock final {
    friend class KMemoryBlockManager;

private:
    VAddr addr{};
    std::size_t num_pages{};
    KMemoryState state{KMemoryState::None};
    u16 ipc_lock_count{};
    u16 device_use_count{};
    KMemoryPermission perm{KMemoryPermission::None};
    KMemoryPermission original_perm{KMemoryPermission::None};
    KMemoryAttribute attribute{KMemoryAttribute::None};

public:
    static constexpr int Compare(const KMemoryBlock& lhs, const KMemoryBlock& rhs) {
        if (lhs.GetAddress() < rhs.GetAddress()) {
            return -1;
        } else if (lhs.GetAddress() <= rhs.GetLastAddress()) {
            return 0;
        } else {
            return 1;
        }
    }

public:
    constexpr KMemoryBlock() = default;
    constexpr KMemoryBlock(VAddr addr_, std::size_t num_pages_, KMemoryState state_,
                           KMemoryPermission perm_, KMemoryAttribute attribute_)
        : addr{addr_}, num_pages(num_pages_), state{state_}, perm{perm_}, attribute{attribute_} {}

    constexpr VAddr GetAddress() const {
        return addr;
    }

    constexpr std::size_t GetNumPages() const {
        return num_pages;
    }

    constexpr std::size_t GetSize() const {
        return GetNumPages() * PageSize;
    }

    constexpr VAddr GetEndAddress() const {
        return GetAddress() + GetSize();
    }

    constexpr VAddr GetLastAddress() const {
        return GetEndAddress() - 1;
    }

    constexpr KMemoryInfo GetMemoryInfo() const {
        return {
            GetAddress(), GetSize(),     state,          perm,
            attribute,    original_perm, ipc_lock_count, device_use_count,
        };
    }

    void ShareToDevice(KMemoryPermission /*new_perm*/) {
        ASSERT((attribute & KMemoryAttribute::DeviceShared) == KMemoryAttribute::DeviceShared ||
               device_use_count == 0);
        attribute |= KMemoryAttribute::DeviceShared;
        const u16 new_use_count{++device_use_count};
        ASSERT(new_use_count > 0);
    }

    void UnshareToDevice(KMemoryPermission /*new_perm*/) {
        ASSERT((attribute & KMemoryAttribute::DeviceShared) == KMemoryAttribute::DeviceShared);
        const u16 prev_use_count{device_use_count--};
        ASSERT(prev_use_count > 0);
        if (prev_use_count == 1) {
            attribute &= ~KMemoryAttribute::DeviceShared;
        }
    }

private:
    constexpr bool HasProperties(KMemoryState s, KMemoryPermission p, KMemoryAttribute a) const {
        constexpr KMemoryAttribute AttributeIgnoreMask{KMemoryAttribute::DontCareMask |
                                                       KMemoryAttribute::IpcLocked |
                                                       KMemoryAttribute::DeviceShared};
        return state == s && perm == p &&
               (attribute | AttributeIgnoreMask) == (a | AttributeIgnoreMask);
    }

    constexpr bool HasSameProperties(const KMemoryBlock& rhs) const {
        return state == rhs.state && perm == rhs.perm && original_perm == rhs.original_perm &&
               attribute == rhs.attribute && ipc_lock_count == rhs.ipc_lock_count &&
               device_use_count == rhs.device_use_count;
    }

    constexpr bool Contains(VAddr start) const {
        return GetAddress() <= start && start <= GetEndAddress();
    }

    constexpr void Add(std::size_t count) {
        ASSERT(count > 0);
        ASSERT(GetAddress() + count * PageSize - 1 < GetEndAddress() + count * PageSize - 1);

        num_pages += count;
    }

    constexpr void Update(KMemoryState new_state, KMemoryPermission new_perm,
                          KMemoryAttribute new_attribute) {
        ASSERT(original_perm == KMemoryPermission::None);
        ASSERT((attribute & KMemoryAttribute::IpcLocked) == KMemoryAttribute::None);

        state = new_state;
        perm = new_perm;

        attribute = static_cast<KMemoryAttribute>(
            new_attribute |
            (attribute & (KMemoryAttribute::IpcLocked | KMemoryAttribute::DeviceShared)));
    }

    constexpr KMemoryBlock Split(VAddr split_addr) {
        ASSERT(GetAddress() < split_addr);
        ASSERT(Contains(split_addr));
        ASSERT(Common::IsAligned(split_addr, PageSize));

        KMemoryBlock block;
        block.addr = addr;
        block.num_pages = (split_addr - GetAddress()) / PageSize;
        block.state = state;
        block.ipc_lock_count = ipc_lock_count;
        block.device_use_count = device_use_count;
        block.perm = perm;
        block.original_perm = original_perm;
        block.attribute = attribute;

        addr = split_addr;
        num_pages -= block.num_pages;

        return block;
    }
};
static_assert(std::is_trivially_destructible<KMemoryBlock>::value);

} // namespace Kernel
