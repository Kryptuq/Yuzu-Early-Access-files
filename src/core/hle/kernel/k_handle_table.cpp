// Copyright 2021 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/kernel/k_handle_table.h"

namespace Kernel {

KHandleTable::KHandleTable(KernelCore& kernel_) : kernel{kernel_} {}
KHandleTable::~KHandleTable() = default;

ResultCode KHandleTable::Finalize() {
    // Get the table and clear our record of it.
    u16 saved_table_size = 0;
    {
        KScopedSpinLock lk(m_lock);

        std::swap(m_table_size, saved_table_size);
    }

    // Close and free all entries.
    for (size_t i = 0; i < saved_table_size; i++) {
        if (KAutoObject* obj = m_objects[i]; obj != nullptr) {
            obj->Close();
        }
    }

    return ResultSuccess;
}

bool KHandleTable::Remove(Handle handle) {
    // Don't allow removal of a pseudo-handle.
    if (Svc::IsPseudoHandle(handle)) {
        return false;
    }

    // Handles must not have reserved bits set.
    const auto handle_pack = HandlePack(handle);
    if (handle_pack.reserved != 0) {
        return false;
    }

    // Find the object and free the entry.
    KAutoObject* obj = nullptr;
    {
        KScopedSpinLock lk(m_lock);

        if (this->IsValidHandle(handle)) {
            const auto index = handle_pack.index;

            obj = m_objects[index];
            this->FreeEntry(index);
        } else {
            return false;
        }
    }

    // Close the object.
    obj->Close();
    return true;
}

ResultCode KHandleTable::Add(Handle* out_handle, KAutoObject* obj, u16 type) {
    KScopedSpinLock lk(m_lock);

    // Never exceed our capacity.
    R_UNLESS(m_count < m_table_size, ResultOutOfHandles);

    // Allocate entry, set output handle.
    {
        const auto linear_id = this->AllocateLinearId();
        const auto index = this->AllocateEntry();

        m_entry_infos[index].info = {.linear_id = linear_id, .type = type};
        m_objects[index] = obj;

        obj->Open();

        *out_handle = EncodeHandle(static_cast<u16>(index), linear_id);
    }

    return ResultSuccess;
}

ResultCode KHandleTable::Reserve(Handle* out_handle) {
    KScopedSpinLock lk(m_lock);

    // Never exceed our capacity.
    R_UNLESS(m_count < m_table_size, ResultOutOfHandles);

    *out_handle = EncodeHandle(static_cast<u16>(this->AllocateEntry()), this->AllocateLinearId());
    return ResultSuccess;
}

void KHandleTable::Unreserve(Handle handle) {
    KScopedSpinLock lk(m_lock);

    // Unpack the handle.
    const auto handle_pack = HandlePack(handle);
    const auto index = handle_pack.index;
    const auto linear_id = handle_pack.linear_id;
    const auto reserved = handle_pack.reserved;
    ASSERT(reserved == 0);
    ASSERT(linear_id != 0);

    if (index < m_table_size) {
        // NOTE: This code does not check the linear id.
        ASSERT(m_objects[index] == nullptr);
        this->FreeEntry(index);
    }
}

void KHandleTable::Register(Handle handle, KAutoObject* obj, u16 type) {
    KScopedSpinLock lk(m_lock);

    // Unpack the handle.
    const auto handle_pack = HandlePack(handle);
    const auto index = handle_pack.index;
    const auto linear_id = handle_pack.linear_id;
    const auto reserved = handle_pack.reserved;
    ASSERT(reserved == 0);
    ASSERT(linear_id != 0);

    if (index < m_table_size) {
        // Set the entry.
        ASSERT(m_objects[index] == nullptr);

        m_entry_infos[index].info = {.linear_id = static_cast<u16>(linear_id), .type = type};
        m_objects[index] = obj;

        obj->Open();
    }
}

} // namespace Kernel
