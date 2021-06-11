// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <atomic>

#include "common/assert.h"
#include "common/common_types.h"

namespace Kernel {

class KernelCore;

namespace impl {

class KSlabHeapImpl final : NonCopyable {
public:
    struct Node {
        Node* next{};
    };

    constexpr KSlabHeapImpl() = default;

    void Initialize(std::size_t size) {
        ASSERT(head == nullptr);
        obj_size = size;
    }

    constexpr std::size_t GetObjectSize() const {
        return obj_size;
    }

    Node* GetHead() const {
        return head;
    }

    void* Allocate() {
        Node* ret = head.load();

        do {
            if (ret == nullptr) {
                break;
            }
        } while (!head.compare_exchange_weak(ret, ret->next));

        return ret;
    }

    void Free(void* obj) {
        Node* node = static_cast<Node*>(obj);

        Node* cur_head = head.load();
        do {
            node->next = cur_head;
        } while (!head.compare_exchange_weak(cur_head, node));
    }

private:
    std::atomic<Node*> head{};
    std::size_t obj_size{};
};

} // namespace impl

class KSlabHeapBase : NonCopyable {
public:
    constexpr KSlabHeapBase() = default;

    constexpr bool Contains(uintptr_t addr) const {
        return start <= addr && addr < end;
    }

    constexpr std::size_t GetSlabHeapSize() const {
        return (end - start) / GetObjectSize();
    }

    constexpr std::size_t GetObjectSize() const {
        return impl.GetObjectSize();
    }

    constexpr uintptr_t GetSlabHeapAddress() const {
        return start;
    }

    std::size_t GetObjectIndexImpl(const void* obj) const {
        return (reinterpret_cast<uintptr_t>(obj) - start) / GetObjectSize();
    }

    std::size_t GetPeakIndex() const {
        return GetObjectIndexImpl(reinterpret_cast<const void*>(peak));
    }

    void* AllocateImpl() {
        return impl.Allocate();
    }

    void FreeImpl(void* obj) {
        // Don't allow freeing an object that wasn't allocated from this heap
        ASSERT(Contains(reinterpret_cast<uintptr_t>(obj)));

        impl.Free(obj);
    }

    void InitializeImpl(std::size_t obj_size, void* memory, std::size_t memory_size) {
        // Ensure we don't initialize a slab using null memory
        ASSERT(memory != nullptr);

        // Initialize the base allocator
        impl.Initialize(obj_size);

        // Set our tracking variables
        const std::size_t num_obj = (memory_size / obj_size);
        start = reinterpret_cast<uintptr_t>(memory);
        end = start + num_obj * obj_size;
        peak = start;

        // Free the objects
        u8* cur = reinterpret_cast<u8*>(end);

        for (std::size_t i{}; i < num_obj; i++) {
            cur -= obj_size;
            impl.Free(cur);
        }
    }

private:
    using Impl = impl::KSlabHeapImpl;

    Impl impl;
    uintptr_t peak{};
    uintptr_t start{};
    uintptr_t end{};
};

template <typename T>
class KSlabHeap final : public KSlabHeapBase {
public:
    enum class AllocationType {
        Host,
        Guest,
    };

    explicit constexpr KSlabHeap(AllocationType allocation_type_ = AllocationType::Host)
        : KSlabHeapBase(), allocation_type{allocation_type_} {}

    void Initialize(void* memory, std::size_t memory_size) {
        if (allocation_type == AllocationType::Guest) {
            InitializeImpl(sizeof(T), memory, memory_size);
        }
    }

    T* Allocate() {
        switch (allocation_type) {
        case AllocationType::Host:
            // Fallback for cases where we do not yet support allocating guest memory from the slab
            // heap, such as for kernel memory regions.
            return new T;

        case AllocationType::Guest:
            T* obj = static_cast<T*>(AllocateImpl());
            if (obj != nullptr) {
                new (obj) T();
            }
            return obj;
        }

        UNREACHABLE_MSG("Invalid AllocationType {}", allocation_type);
        return nullptr;
    }

    T* AllocateWithKernel(KernelCore& kernel) {
        switch (allocation_type) {
        case AllocationType::Host:
            // Fallback for cases where we do not yet support allocating guest memory from the slab
            // heap, such as for kernel memory regions.
            return new T(kernel);

        case AllocationType::Guest:
            T* obj = static_cast<T*>(AllocateImpl());
            if (obj != nullptr) {
                new (obj) T(kernel);
            }
            return obj;
        }

        UNREACHABLE_MSG("Invalid AllocationType {}", allocation_type);
        return nullptr;
    }

    void Free(T* obj) {
        switch (allocation_type) {
        case AllocationType::Host:
            // Fallback for cases where we do not yet support allocating guest memory from the slab
            // heap, such as for kernel memory regions.
            delete obj;
            return;

        case AllocationType::Guest:
            FreeImpl(obj);
            return;
        }

        UNREACHABLE_MSG("Invalid AllocationType {}", allocation_type);
    }

    constexpr std::size_t GetObjectIndex(const T* obj) const {
        return GetObjectIndexImpl(obj);
    }

private:
    const AllocationType allocation_type;
};

} // namespace Kernel
