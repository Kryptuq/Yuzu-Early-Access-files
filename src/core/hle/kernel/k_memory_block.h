// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/alignment.h"
#include "common/assert.h"
#include "common/common_types.h"
#include "common/intrusive_red_black_tree.h"
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
    All = 0xFF,
    UserMask = All,

    Locked = static_cast<u8>(Svc::MemoryAttribute::Locked),
    IpcLocked = static_cast<u8>(Svc::MemoryAttribute::IpcLocked),
    DeviceShared = static_cast<u8>(Svc::MemoryAttribute::DeviceShared),
    Uncached = static_cast<u8>(Svc::MemoryAttribute::Uncached),

    SetMask = Uncached,
};
DECLARE_ENUM_FLAG_OPERATORS(KMemoryAttribute);

enum class KMemoryBlockDisableMergeAttribute : u8 {
    None = 0,
    Normal = (1u << 0),
    DeviceLeft = (1u << 1),
    IpcLeft = (1u << 2),
    Locked = (1u << 3),
    DeviceRight = (1u << 4),

    AllLeft = Normal | DeviceLeft | IpcLeft | Locked,
    AllRight = DeviceRight,
};
DECLARE_ENUM_FLAG_OPERATORS(KMemoryBlockDisableMergeAttribute);

struct KMemoryInfo {
    uintptr_t m_address;
    size_t m_size;
    KMemoryState m_state;
    u16 m_device_disable_merge_left_count;
    u16 m_device_disable_merge_right_count;
    u16 m_ipc_lock_count;
    u16 m_device_use_count;
    u16 m_ipc_disable_merge_count;
    KMemoryPermission m_permission;
    KMemoryAttribute m_attribute;
    KMemoryPermission m_original_permission;
    KMemoryBlockDisableMergeAttribute m_disable_merge_attribute;

    constexpr Svc::MemoryInfo GetSvcMemoryInfo() const {
        return {
            .addr = m_address,
            .size = m_size,
            .state = static_cast<Svc::MemoryState>(m_state & KMemoryState::Mask),
            .attr = static_cast<Svc::MemoryAttribute>(m_attribute & KMemoryAttribute::UserMask),
            .perm = static_cast<Svc::MemoryPermission>(m_permission & KMemoryPermission::UserMask),
            .ipc_refcount = m_ipc_lock_count,
            .device_refcount = m_device_use_count,
            .padding = {},
        };
    }

    constexpr uintptr_t GetAddress() const {
        return m_address;
    }

    constexpr size_t GetSize() const {
        return m_size;
    }

    constexpr size_t GetNumPages() const {
        return this->GetSize() / PageSize;
    }

    constexpr uintptr_t GetEndAddress() const {
        return this->GetAddress() + this->GetSize();
    }

    constexpr uintptr_t GetLastAddress() const {
        return this->GetEndAddress() - 1;
    }

    constexpr u16 GetIpcLockCount() const {
        return m_ipc_lock_count;
    }

    constexpr u16 GetIpcDisableMergeCount() const {
        return m_ipc_disable_merge_count;
    }

    constexpr KMemoryState GetState() const {
        return m_state;
    }

    constexpr KMemoryPermission GetPermission() const {
        return m_permission;
    }

    constexpr KMemoryPermission GetOriginalPermission() const {
        return m_original_permission;
    }

    constexpr KMemoryAttribute GetAttribute() const {
        return m_attribute;
    }

    constexpr KMemoryBlockDisableMergeAttribute GetDisableMergeAttribute() const {
        return m_disable_merge_attribute;
    }
};

class KMemoryBlock : public Common::IntrusiveRedBlackTreeBaseNode<KMemoryBlock> {
private:
    u16 m_device_disable_merge_left_count;
    u16 m_device_disable_merge_right_count;
    VAddr m_address;
    size_t m_num_pages;
    KMemoryState m_memory_state;
    u16 m_ipc_lock_count;
    u16 m_device_use_count;
    u16 m_ipc_disable_merge_count;
    KMemoryPermission m_permission;
    KMemoryPermission m_original_permission;
    KMemoryAttribute m_attribute;
    KMemoryBlockDisableMergeAttribute m_disable_merge_attribute;

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
    constexpr VAddr GetAddress() const {
        return m_address;
    }

    constexpr size_t GetNumPages() const {
        return m_num_pages;
    }

    constexpr size_t GetSize() const {
        return this->GetNumPages() * PageSize;
    }

    constexpr VAddr GetEndAddress() const {
        return this->GetAddress() + this->GetSize();
    }

    constexpr VAddr GetLastAddress() const {
        return this->GetEndAddress() - 1;
    }

    constexpr u16 GetIpcLockCount() const {
        return m_ipc_lock_count;
    }

    constexpr u16 GetIpcDisableMergeCount() const {
        return m_ipc_disable_merge_count;
    }

    constexpr KMemoryPermission GetPermission() const {
        return m_permission;
    }

    constexpr KMemoryPermission GetOriginalPermission() const {
        return m_original_permission;
    }

    constexpr KMemoryAttribute GetAttribute() const {
        return m_attribute;
    }

    constexpr KMemoryInfo GetMemoryInfo() const {
        return {
            .m_address = this->GetAddress(),
            .m_size = this->GetSize(),
            .m_state = m_memory_state,
            .m_device_disable_merge_left_count = m_device_disable_merge_left_count,
            .m_device_disable_merge_right_count = m_device_disable_merge_right_count,
            .m_ipc_lock_count = m_ipc_lock_count,
            .m_device_use_count = m_device_use_count,
            .m_ipc_disable_merge_count = m_ipc_disable_merge_count,
            .m_permission = m_permission,
            .m_attribute = m_attribute,
            .m_original_permission = m_original_permission,
            .m_disable_merge_attribute = m_disable_merge_attribute,
        };
    }

public:
    explicit KMemoryBlock() = default;

    constexpr KMemoryBlock(VAddr addr, size_t np, KMemoryState ms, KMemoryPermission p,
                           KMemoryAttribute attr)
        : Common::IntrusiveRedBlackTreeBaseNode<KMemoryBlock>(),
          m_device_disable_merge_left_count(), m_device_disable_merge_right_count(),
          m_address(addr), m_num_pages(np), m_memory_state(ms), m_ipc_lock_count(0),
          m_device_use_count(0), m_ipc_disable_merge_count(), m_permission(p),
          m_original_permission(KMemoryPermission::None), m_attribute(attr),
          m_disable_merge_attribute() {}

    constexpr void Initialize(VAddr addr, size_t np, KMemoryState ms, KMemoryPermission p,
                              KMemoryAttribute attr) {
        m_device_disable_merge_left_count = 0;
        m_device_disable_merge_right_count = 0;
        m_address = addr;
        m_num_pages = np;
        m_memory_state = ms;
        m_ipc_lock_count = 0;
        m_device_use_count = 0;
        m_permission = p;
        m_original_permission = KMemoryPermission::None;
        m_attribute = attr;
        m_disable_merge_attribute = KMemoryBlockDisableMergeAttribute::None;
    }

    constexpr bool HasProperties(KMemoryState s, KMemoryPermission p, KMemoryAttribute a) const {
        constexpr auto AttributeIgnoreMask =
            KMemoryAttribute::IpcLocked | KMemoryAttribute::DeviceShared;
        return m_memory_state == s && m_permission == p &&
               (m_attribute | AttributeIgnoreMask) == (a | AttributeIgnoreMask);
    }

    constexpr bool HasSameProperties(const KMemoryBlock& rhs) const {
        return m_memory_state == rhs.m_memory_state && m_permission == rhs.m_permission &&
               m_original_permission == rhs.m_original_permission &&
               m_attribute == rhs.m_attribute && m_ipc_lock_count == rhs.m_ipc_lock_count &&
               m_device_use_count == rhs.m_device_use_count;
    }

    constexpr bool CanMergeWith(const KMemoryBlock& rhs) const {
        return this->HasSameProperties(rhs) &&
               (m_disable_merge_attribute & KMemoryBlockDisableMergeAttribute::AllRight) ==
                   KMemoryBlockDisableMergeAttribute::None &&
               (rhs.m_disable_merge_attribute & KMemoryBlockDisableMergeAttribute::AllLeft) ==
                   KMemoryBlockDisableMergeAttribute::None;
    }

    constexpr bool Contains(VAddr addr) const {
        return this->GetAddress() <= addr && addr <= this->GetEndAddress();
    }

    constexpr void Add(const KMemoryBlock& added_block) {
        ASSERT(added_block.GetNumPages() > 0);
        ASSERT(this->GetAddress() + added_block.GetSize() - 1 <
               this->GetEndAddress() + added_block.GetSize() - 1);

        m_num_pages += added_block.GetNumPages();
        m_disable_merge_attribute = static_cast<KMemoryBlockDisableMergeAttribute>(
            m_disable_merge_attribute | added_block.m_disable_merge_attribute);
        m_device_disable_merge_right_count = added_block.m_device_disable_merge_right_count;
    }

    constexpr void Update(KMemoryState s, KMemoryPermission p, KMemoryAttribute a,
                          bool set_disable_merge_attr, u8 set_mask, u8 clear_mask) {
        ASSERT(m_original_permission == KMemoryPermission::None);
        ASSERT((m_attribute & KMemoryAttribute::IpcLocked) == KMemoryAttribute::None);

        m_memory_state = s;
        m_permission = p;
        m_attribute = static_cast<KMemoryAttribute>(
            a | (m_attribute & (KMemoryAttribute::IpcLocked | KMemoryAttribute::DeviceShared)));

        if (set_disable_merge_attr && set_mask != 0) {
            m_disable_merge_attribute = m_disable_merge_attribute |
                                        static_cast<KMemoryBlockDisableMergeAttribute>(set_mask);
        }
        if (clear_mask != 0) {
            m_disable_merge_attribute = m_disable_merge_attribute &
                                        static_cast<KMemoryBlockDisableMergeAttribute>(~clear_mask);
        }
    }

    constexpr void Split(KMemoryBlock* block, VAddr addr) {
        ASSERT(this->GetAddress() < addr);
        ASSERT(this->Contains(addr));
        ASSERT(Common::IsAligned(addr, PageSize));

        block->m_address = m_address;
        block->m_num_pages = (addr - this->GetAddress()) / PageSize;
        block->m_memory_state = m_memory_state;
        block->m_ipc_lock_count = m_ipc_lock_count;
        block->m_device_use_count = m_device_use_count;
        block->m_permission = m_permission;
        block->m_original_permission = m_original_permission;
        block->m_attribute = m_attribute;
        block->m_disable_merge_attribute = static_cast<KMemoryBlockDisableMergeAttribute>(
            m_disable_merge_attribute & KMemoryBlockDisableMergeAttribute::AllLeft);
        block->m_ipc_disable_merge_count = m_ipc_disable_merge_count;
        block->m_device_disable_merge_left_count = m_device_disable_merge_left_count;
        block->m_device_disable_merge_right_count = 0;

        m_address = addr;
        m_num_pages -= block->m_num_pages;

        m_ipc_disable_merge_count = 0;
        m_device_disable_merge_left_count = 0;
        m_disable_merge_attribute = static_cast<KMemoryBlockDisableMergeAttribute>(
            m_disable_merge_attribute & KMemoryBlockDisableMergeAttribute::AllRight);
    }

    constexpr void UpdateDeviceDisableMergeStateForShareLeft(
        [[maybe_unused]] KMemoryPermission new_perm, bool left, [[maybe_unused]] bool right) {
        if (left) {
            m_disable_merge_attribute = static_cast<KMemoryBlockDisableMergeAttribute>(
                m_disable_merge_attribute | KMemoryBlockDisableMergeAttribute::DeviceLeft);
            const u16 new_device_disable_merge_left_count = ++m_device_disable_merge_left_count;
            ASSERT(new_device_disable_merge_left_count > 0);
        }
    }

    constexpr void UpdateDeviceDisableMergeStateForShareRight(
        [[maybe_unused]] KMemoryPermission new_perm, [[maybe_unused]] bool left, bool right) {
        if (right) {
            m_disable_merge_attribute = static_cast<KMemoryBlockDisableMergeAttribute>(
                m_disable_merge_attribute | KMemoryBlockDisableMergeAttribute::DeviceRight);
            const u16 new_device_disable_merge_right_count = ++m_device_disable_merge_right_count;
            ASSERT(new_device_disable_merge_right_count > 0);
        }
    }

    constexpr void UpdateDeviceDisableMergeStateForShare(KMemoryPermission new_perm, bool left,
                                                         bool right) {
        this->UpdateDeviceDisableMergeStateForShareLeft(new_perm, left, right);
        this->UpdateDeviceDisableMergeStateForShareRight(new_perm, left, right);
    }

    constexpr void ShareToDevice([[maybe_unused]] KMemoryPermission new_perm, bool left,
                                 bool right) {
        // We must either be shared or have a zero lock count.
        ASSERT((m_attribute & KMemoryAttribute::DeviceShared) == KMemoryAttribute::DeviceShared ||
               m_device_use_count == 0);

        // Share.
        const u16 new_count = ++m_device_use_count;
        ASSERT(new_count > 0);

        m_attribute = static_cast<KMemoryAttribute>(m_attribute | KMemoryAttribute::DeviceShared);

        this->UpdateDeviceDisableMergeStateForShare(new_perm, left, right);
    }

    constexpr void UpdateDeviceDisableMergeStateForUnshareLeft(
        [[maybe_unused]] KMemoryPermission new_perm, bool left, [[maybe_unused]] bool right) {

        if (left) {
            if (!m_device_disable_merge_left_count) {
                return;
            }
            --m_device_disable_merge_left_count;
        }

        m_device_disable_merge_left_count =
            std::min(m_device_disable_merge_left_count, m_device_use_count);

        if (m_device_disable_merge_left_count == 0) {
            m_disable_merge_attribute = static_cast<KMemoryBlockDisableMergeAttribute>(
                m_disable_merge_attribute & ~KMemoryBlockDisableMergeAttribute::DeviceLeft);
        }
    }

    constexpr void UpdateDeviceDisableMergeStateForUnshareRight(
        [[maybe_unused]] KMemoryPermission new_perm, [[maybe_unused]] bool left, bool right) {
        if (right) {
            const u16 old_device_disable_merge_right_count = m_device_disable_merge_right_count--;
            ASSERT(old_device_disable_merge_right_count > 0);
            if (old_device_disable_merge_right_count == 1) {
                m_disable_merge_attribute = static_cast<KMemoryBlockDisableMergeAttribute>(
                    m_disable_merge_attribute & ~KMemoryBlockDisableMergeAttribute::DeviceRight);
            }
        }
    }

    constexpr void UpdateDeviceDisableMergeStateForUnshare(KMemoryPermission new_perm, bool left,
                                                           bool right) {
        this->UpdateDeviceDisableMergeStateForUnshareLeft(new_perm, left, right);
        this->UpdateDeviceDisableMergeStateForUnshareRight(new_perm, left, right);
    }

    constexpr void UnshareToDevice([[maybe_unused]] KMemoryPermission new_perm, bool left,
                                   bool right) {
        // We must be shared.
        ASSERT((m_attribute & KMemoryAttribute::DeviceShared) == KMemoryAttribute::DeviceShared);

        // Unhare.
        const u16 old_count = m_device_use_count--;
        ASSERT(old_count > 0);

        if (old_count == 1) {
            m_attribute =
                static_cast<KMemoryAttribute>(m_attribute & ~KMemoryAttribute::DeviceShared);
        }

        this->UpdateDeviceDisableMergeStateForUnshare(new_perm, left, right);
    }

    constexpr void UnshareToDeviceRight([[maybe_unused]] KMemoryPermission new_perm, bool left,
                                        bool right) {

        // We must be shared.
        ASSERT((m_attribute & KMemoryAttribute::DeviceShared) == KMemoryAttribute::DeviceShared);

        // Unhare.
        const u16 old_count = m_device_use_count--;
        ASSERT(old_count > 0);

        if (old_count == 1) {
            m_attribute =
                static_cast<KMemoryAttribute>(m_attribute & ~KMemoryAttribute::DeviceShared);
        }

        this->UpdateDeviceDisableMergeStateForUnshareRight(new_perm, left, right);
    }

    constexpr void LockForIpc(KMemoryPermission new_perm, bool left, [[maybe_unused]] bool right) {
        // We must either be locked or have a zero lock count.
        ASSERT((m_attribute & KMemoryAttribute::IpcLocked) == KMemoryAttribute::IpcLocked ||
               m_ipc_lock_count == 0);

        // Lock.
        const u16 new_lock_count = ++m_ipc_lock_count;
        ASSERT(new_lock_count > 0);

        // If this is our first lock, update our permissions.
        if (new_lock_count == 1) {
            ASSERT(m_original_permission == KMemoryPermission::None);
            ASSERT((m_permission | new_perm | KMemoryPermission::NotMapped) ==
                   (m_permission | KMemoryPermission::NotMapped));
            ASSERT((m_permission & KMemoryPermission::UserExecute) !=
                       KMemoryPermission::UserExecute ||
                   (new_perm == KMemoryPermission::UserRead));
            m_original_permission = m_permission;
            m_permission = static_cast<KMemoryPermission>(
                (new_perm & KMemoryPermission::IpcLockChangeMask) |
                (m_original_permission & ~KMemoryPermission::IpcLockChangeMask));
        }
        m_attribute = static_cast<KMemoryAttribute>(m_attribute | KMemoryAttribute::IpcLocked);

        if (left) {
            m_disable_merge_attribute = static_cast<KMemoryBlockDisableMergeAttribute>(
                m_disable_merge_attribute | KMemoryBlockDisableMergeAttribute::IpcLeft);
            const u16 new_ipc_disable_merge_count = ++m_ipc_disable_merge_count;
            ASSERT(new_ipc_disable_merge_count > 0);
        }
    }

    constexpr void UnlockForIpc([[maybe_unused]] KMemoryPermission new_perm, bool left,
                                [[maybe_unused]] bool right) {
        // We must be locked.
        ASSERT((m_attribute & KMemoryAttribute::IpcLocked) == KMemoryAttribute::IpcLocked);

        // Unlock.
        const u16 old_lock_count = m_ipc_lock_count--;
        ASSERT(old_lock_count > 0);

        // If this is our last unlock, update our permissions.
        if (old_lock_count == 1) {
            ASSERT(m_original_permission != KMemoryPermission::None);
            m_permission = m_original_permission;
            m_original_permission = KMemoryPermission::None;
            m_attribute = static_cast<KMemoryAttribute>(m_attribute & ~KMemoryAttribute::IpcLocked);
        }

        if (left) {
            const u16 old_ipc_disable_merge_count = m_ipc_disable_merge_count--;
            ASSERT(old_ipc_disable_merge_count > 0);
            if (old_ipc_disable_merge_count == 1) {
                m_disable_merge_attribute = static_cast<KMemoryBlockDisableMergeAttribute>(
                    m_disable_merge_attribute & ~KMemoryBlockDisableMergeAttribute::IpcLeft);
            }
        }
    }

    constexpr KMemoryBlockDisableMergeAttribute GetDisableMergeAttribute() const {
        return m_disable_merge_attribute;
    }
};
static_assert(std::is_trivially_destructible<KMemoryBlock>::value);

} // namespace Kernel
