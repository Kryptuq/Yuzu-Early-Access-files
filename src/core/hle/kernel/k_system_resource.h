// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/assert.h"
#include "common/common_types.h"
#include "core/hle/kernel/k_auto_object.h"
#include "core/hle/kernel/k_dynamic_resource_manager.h"
#include "core/hle/kernel/k_memory_manager.h"
#include "core/hle/kernel/k_page_table_manager.h"
#include "core/hle/kernel/k_resource_limit.h"
#include "core/hle/kernel/slab_helpers.h"

namespace Kernel {

// NOTE: Nintendo's implementation does not have the "is_secure_resource" field, and instead uses
// virtual IsSecureResource().

class KSystemResource : public KAutoObject {
    KERNEL_AUTOOBJECT_TRAITS(KSystemResource, KAutoObject);

public:
    explicit KSystemResource(KernelCore& kernel_) : KAutoObject(kernel_) {}

protected:
    void SetSecureResource() {
        m_is_secure_resource = true;
    }

public:
    virtual void Destroy() override {
        UNREACHABLE_MSG("KSystemResource::Destroy() was called");
    }

    bool IsSecureResource() const {
        return m_is_secure_resource;
    }

    void SetManagers(KMemoryBlockSlabManager& mb, KBlockInfoManager& bi, KPageTableManager& pt) {
        ASSERT(m_p_memory_block_slab_manager == nullptr);
        ASSERT(m_p_block_info_manager == nullptr);
        ASSERT(m_p_page_table_manager == nullptr);

        m_p_memory_block_slab_manager = std::addressof(mb);
        m_p_block_info_manager = std::addressof(bi);
        m_p_page_table_manager = std::addressof(pt);
    }

    const KMemoryBlockSlabManager& GetMemoryBlockSlabManager() const {
        return *m_p_memory_block_slab_manager;
    }
    const KBlockInfoManager& GetBlockInfoManager() const {
        return *m_p_block_info_manager;
    }
    const KPageTableManager& GetPageTableManager() const {
        return *m_p_page_table_manager;
    }

    KMemoryBlockSlabManager& GetMemoryBlockSlabManager() {
        return *m_p_memory_block_slab_manager;
    }
    KBlockInfoManager& GetBlockInfoManager() {
        return *m_p_block_info_manager;
    }
    KPageTableManager& GetPageTableManager() {
        return *m_p_page_table_manager;
    }

    KMemoryBlockSlabManager* GetMemoryBlockSlabManagerPointer() {
        return m_p_memory_block_slab_manager;
    }
    KBlockInfoManager* GetBlockInfoManagerPointer() {
        return m_p_block_info_manager;
    }
    KPageTableManager* GetPageTableManagerPointer() {
        return m_p_page_table_manager;
    }

private:
    KMemoryBlockSlabManager* m_p_memory_block_slab_manager{};
    KBlockInfoManager* m_p_block_info_manager{};
    KPageTableManager* m_p_page_table_manager{};
    bool m_is_secure_resource{false};
};

class KSecureSystemResource final
    : public KAutoObjectWithSlabHeap<KSecureSystemResource, KSystemResource> {
public:
    explicit KSecureSystemResource(KernelCore& kernel_)
        : KAutoObjectWithSlabHeap<KSecureSystemResource, KSystemResource>(kernel_) {
        // Mark ourselves as being a secure resource.
        this->SetSecureResource();
    }

    Result Initialize(size_t size, KResourceLimit* resource_limit, KMemoryManager::Pool pool);
    void Finalize();

    bool IsInitialized() const {
        return m_is_initialized;
    }
    static void PostDestroy([[maybe_unused]] uintptr_t arg) {}

    size_t CalculateRequiredSecureMemorySize() const {
        return CalculateRequiredSecureMemorySize(m_resource_size, m_resource_pool);
    }

    size_t GetSize() const {
        return m_resource_size;
    }
    size_t GetUsedSize() const {
        return m_dynamic_page_manager.GetUsed() * PageSize;
    }

    const KDynamicPageManager& GetDynamicPageManager() const {
        return m_dynamic_page_manager;
    }

public:
    static size_t CalculateRequiredSecureMemorySize(size_t size, KMemoryManager::Pool pool);

private:
    bool m_is_initialized{};
    KMemoryManager::Pool m_resource_pool{};
    KDynamicPageManager m_dynamic_page_manager;
    KMemoryBlockSlabManager m_memory_block_slab_manager;
    KBlockInfoManager m_block_info_manager;
    KPageTableManager m_page_table_manager;
    KMemoryBlockSlabHeap m_memory_block_heap;
    KBlockInfoSlabHeap m_block_info_heap;
    KPageTableSlabHeap m_page_table_heap;
    KResourceLimit* m_resource_limit{};
    VAddr m_resource_address{};
    size_t m_resource_size{};
};

} // namespace Kernel
