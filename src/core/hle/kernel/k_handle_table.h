// Copyright 2021 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>

#include "common/assert.h"
#include "common/bit_field.h"
#include "common/common_types.h"
#include "core/hle/kernel/k_auto_object.h"
#include "core/hle/kernel/k_spin_lock.h"
#include "core/hle/kernel/k_thread.h"
#include "core/hle/kernel/kernel.h"
#include "core/hle/kernel/svc_common.h"
#include "core/hle/kernel/svc_results.h"
#include "core/hle/result.h"

namespace Kernel {

class KernelCore;

class KHandleTable {
    YUZU_NON_COPYABLE(KHandleTable);
    YUZU_NON_MOVEABLE(KHandleTable);

public:
    static constexpr size_t MaxTableSize = 1024;

public:
    explicit KHandleTable(KernelCore& kernel_);
    ~KHandleTable();

    ResultCode Initialize(s32 size) {
        R_UNLESS(size <= static_cast<s32>(MaxTableSize), ResultOutOfMemory);

        // Initialize all fields.
        m_max_count = 0;
        m_table_size = static_cast<u16>((size <= 0) ? MaxTableSize : size);
        m_next_linear_id = MinLinearId;
        m_count = 0;
        m_free_head_index = -1;

        // Free all entries.
        for (s32 i = 0; i < static_cast<s32>(m_table_size); ++i) {
            m_objects[i] = nullptr;
            m_entry_infos[i].next_free_index = i - 1;
            m_free_head_index = i;
        }

        return ResultSuccess;
    }

    size_t GetTableSize() const {
        return m_table_size;
    }
    size_t GetCount() const {
        return m_count;
    }
    size_t GetMaxCount() const {
        return m_max_count;
    }

    ResultCode Finalize();
    bool Remove(Handle handle);

    template <typename T = KAutoObject>
    KScopedAutoObject<T> GetObjectWithoutPseudoHandle(Handle handle) const {
        // Lock and look up in table.
        KScopedDisableDispatch dd(kernel);
        KScopedSpinLock lk(m_lock);

        if constexpr (std::is_same_v<T, KAutoObject>) {
            return this->GetObjectImpl(handle);
        } else {
            if (auto* obj = this->GetObjectImpl(handle); obj != nullptr) {
                return obj->DynamicCast<T*>();
            } else {
                return nullptr;
            }
        }
    }

    template <typename T = KAutoObject>
    KScopedAutoObject<T> GetObject(Handle handle) const {
        // Handle pseudo-handles.
        if constexpr (std::derived_from<KProcess, T>) {
            if (handle == Svc::PseudoHandle::CurrentProcess) {
                auto* const cur_process = kernel.CurrentProcess();
                ASSERT(cur_process != nullptr);
                return cur_process;
            }
        } else if constexpr (std::derived_from<KThread, T>) {
            if (handle == Svc::PseudoHandle::CurrentThread) {
                auto* const cur_thread = GetCurrentThreadPointer(kernel);
                ASSERT(cur_thread != nullptr);
                return cur_thread;
            }
        }

        return this->template GetObjectWithoutPseudoHandle<T>(handle);
    }

    ResultCode Reserve(Handle* out_handle);
    void Unreserve(Handle handle);

    template <typename T>
    ResultCode Add(Handle* out_handle, T* obj) {
        static_assert(std::is_base_of_v<KAutoObject, T>);
        return this->Add(out_handle, obj, obj->GetTypeObj().GetClassToken());
    }

    template <typename T>
    void Register(Handle handle, T* obj) {
        static_assert(std::is_base_of_v<KAutoObject, T>);
        return this->Register(handle, obj, obj->GetTypeObj().GetClassToken());
    }

    template <typename T>
    bool GetMultipleObjects(T** out, const Handle* handles, size_t num_handles) const {
        // Try to convert and open all the handles.
        size_t num_opened;
        {
            // Lock the table.
            KScopedDisableDispatch dd(kernel);
            KScopedSpinLock lk(m_lock);
            for (num_opened = 0; num_opened < num_handles; num_opened++) {
                // Get the current handle.
                const auto cur_handle = handles[num_opened];

                // Get the object for the current handle.
                KAutoObject* cur_object = this->GetObjectImpl(cur_handle);
                if (cur_object == nullptr) {
                    break;
                }

                // Cast the current object to the desired type.
                T* cur_t = cur_object->DynamicCast<T*>();
                if (cur_t == nullptr) {
                    break;
                }

                // Open a reference to the current object.
                cur_t->Open();
                out[num_opened] = cur_t;
            }
        }

        // If we converted every object, succeed.
        if (num_opened == num_handles) {
            return true;
        }

        // If we didn't convert entry object, close the ones we opened.
        for (size_t i = 0; i < num_opened; i++) {
            out[i]->Close();
        }

        return false;
    }

private:
    ResultCode Add(Handle* out_handle, KAutoObject* obj, u16 type);
    void Register(Handle handle, KAutoObject* obj, u16 type);

    s32 AllocateEntry() {
        ASSERT(m_count < m_table_size);

        const auto index = m_free_head_index;

        m_free_head_index = m_entry_infos[index].GetNextFreeIndex();

        m_max_count = std::max(m_max_count, ++m_count);

        return index;
    }

    void FreeEntry(s32 index) {
        ASSERT(m_count > 0);

        m_objects[index] = nullptr;
        m_entry_infos[index].next_free_index = m_free_head_index;

        m_free_head_index = index;

        --m_count;
    }

    u16 AllocateLinearId() {
        const u16 id = m_next_linear_id++;
        if (m_next_linear_id > MaxLinearId) {
            m_next_linear_id = MinLinearId;
        }
        return id;
    }

    bool IsValidHandle(Handle handle) const {
        // Unpack the handle.
        const auto handle_pack = HandlePack(handle);
        const auto raw_value = handle_pack.raw;
        const auto index = handle_pack.index;
        const auto linear_id = handle_pack.linear_id;
        const auto reserved = handle_pack.reserved;
        ASSERT(reserved == 0);

        // Validate our indexing information.
        if (raw_value == 0) {
            return false;
        }
        if (linear_id == 0) {
            return false;
        }
        if (index >= m_table_size) {
            return false;
        }

        // Check that there's an object, and our serial id is correct.
        if (m_objects[index] == nullptr) {
            return false;
        }
        if (m_entry_infos[index].GetLinearId() != linear_id) {
            return false;
        }

        return true;
    }

    KAutoObject* GetObjectImpl(Handle handle) const {
        // Handles must not have reserved bits set.
        const auto handle_pack = HandlePack(handle);
        if (handle_pack.reserved != 0) {
            return nullptr;
        }

        if (this->IsValidHandle(handle)) {
            return m_objects[handle_pack.index];
        } else {
            return nullptr;
        }
    }

    KAutoObject* GetObjectByIndexImpl(Handle* out_handle, size_t index) const {

        // Index must be in bounds.
        if (index >= m_table_size) {
            return nullptr;
        }

        // Ensure entry has an object.
        if (KAutoObject* obj = m_objects[index]; obj != nullptr) {
            *out_handle = EncodeHandle(static_cast<u16>(index), m_entry_infos[index].GetLinearId());
            return obj;
        } else {
            return nullptr;
        }
    }

private:
    union HandlePack {
        HandlePack() = default;
        HandlePack(Handle handle) : raw{static_cast<u32>(handle)} {}

        u32 raw;
        BitField<0, 15, u32> index;
        BitField<15, 15, u32> linear_id;
        BitField<30, 2, u32> reserved;
    };

    static constexpr u16 MinLinearId = 1;
    static constexpr u16 MaxLinearId = 0x7FFF;

    static constexpr Handle EncodeHandle(u16 index, u16 linear_id) {
        HandlePack handle{};
        handle.index.Assign(index);
        handle.linear_id.Assign(linear_id);
        handle.reserved.Assign(0);
        return handle.raw;
    }

    union EntryInfo {
        struct {
            u16 linear_id;
            u16 type;
        } info;
        s32 next_free_index;

        constexpr u16 GetLinearId() const {
            return info.linear_id;
        }
        constexpr u16 GetType() const {
            return info.type;
        }
        constexpr s32 GetNextFreeIndex() const {
            return next_free_index;
        }
    };

private:
    std::array<EntryInfo, MaxTableSize> m_entry_infos{};
    std::array<KAutoObject*, MaxTableSize> m_objects{};
    s32 m_free_head_index{-1};
    u16 m_table_size{};
    u16 m_max_count{};
    u16 m_next_linear_id{MinLinearId};
    u16 m_count{};
    mutable KSpinLock m_lock;
    KernelCore& kernel;
};

} // namespace Kernel
