// Copyright 2021 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "core/hle/kernel/k_auto_object.h"
#include "core/hle/kernel/k_auto_object_container.h"
#include "core/hle/kernel/kernel.h"

namespace Kernel {

template <class Derived>
class KSlabAllocated {
public:
    constexpr KSlabAllocated() = default;

    size_t GetSlabIndex(KernelCore& kernel) const {
        return kernel.SlabHeap<Derived>().GetIndex(static_cast<const Derived*>(this));
    }

public:
    static void InitializeSlabHeap(KernelCore& kernel, void* memory, size_t memory_size) {
        kernel.SlabHeap<Derived>().Initialize(memory, memory_size);
    }

    static Derived* Allocate(KernelCore& kernel) {
        return kernel.SlabHeap<Derived>().Allocate();
    }

    static void Free(KernelCore& kernel, Derived* obj) {
        kernel.SlabHeap<Derived>().Free(obj);
    }

    static size_t GetObjectSize(KernelCore& kernel) {
        return kernel.SlabHeap<Derived>().GetObjectSize();
    }

    static size_t GetSlabHeapSize(KernelCore& kernel) {
        return kernel.SlabHeap<Derived>().GetSlabHeapSize();
    }

    static size_t GetPeakIndex(KernelCore& kernel) {
        return kernel.SlabHeap<Derived>().GetPeakIndex();
    }

    static uintptr_t GetSlabHeapAddress(KernelCore& kernel) {
        return kernel.SlabHeap<Derived>().GetSlabHeapAddress();
    }

    static size_t GetNumRemaining(KernelCore& kernel) {
        return kernel.SlabHeap<Derived>().GetNumRemaining();
    }
};

template <typename Derived, typename Base>
class KAutoObjectWithSlabHeapAndContainer : public Base {
    static_assert(std::is_base_of<KAutoObjectWithList, Base>::value);

private:
    static Derived* Allocate(KernelCore& kernel) {
        return kernel.SlabHeap<Derived>().AllocateWithKernel(kernel);
    }

    static void Free(KernelCore& kernel, Derived* obj) {
        kernel.SlabHeap<Derived>().Free(obj);
    }

public:
    KAutoObjectWithSlabHeapAndContainer(KernelCore& kernel_) : Base(kernel_), kernel(kernel_) {}
    virtual ~KAutoObjectWithSlabHeapAndContainer() {}

    virtual void Destroy() override {
        const bool is_initialized = this->IsInitialized();
        uintptr_t arg = 0;
        if (is_initialized) {
            kernel.ObjectListContainer().Unregister(this);
            arg = this->GetPostDestroyArgument();
            this->Finalize();
        }
        Free(kernel, static_cast<Derived*>(this));
        if (is_initialized) {
            Derived::PostDestroy(arg);
        }
    }

    virtual bool IsInitialized() const {
        return true;
    }
    virtual uintptr_t GetPostDestroyArgument() const {
        return 0;
    }

    size_t GetSlabIndex() const {
        return SlabHeap<Derived>(kernel).GetObjectIndex(static_cast<const Derived*>(this));
    }

public:
    static void InitializeSlabHeap(KernelCore& kernel, void* memory, size_t memory_size) {
        kernel.SlabHeap<Derived>().Initialize(memory, memory_size);
        kernel.ObjectListContainer().Initialize();
    }

    static Derived* Create(KernelCore& kernel) {
        Derived* obj = Allocate(kernel);
        if (obj != nullptr) {
            KAutoObject::Create(obj);
        }
        return obj;
    }

    static void Register(KernelCore& kernel, Derived* obj) {
        return kernel.ObjectListContainer().Register(obj);
    }

    static size_t GetObjectSize(KernelCore& kernel) {
        return kernel.SlabHeap<Derived>().GetObjectSize();
    }

    static size_t GetSlabHeapSize(KernelCore& kernel) {
        return kernel.SlabHeap<Derived>().GetSlabHeapSize();
    }

    static size_t GetPeakIndex(KernelCore& kernel) {
        return kernel.SlabHeap<Derived>().GetPeakIndex();
    }

    static uintptr_t GetSlabHeapAddress(KernelCore& kernel) {
        return kernel.SlabHeap<Derived>().GetSlabHeapAddress();
    }

    static size_t GetNumRemaining(KernelCore& kernel) {
        return kernel.SlabHeap<Derived>().GetNumRemaining();
    }

protected:
    KernelCore& kernel;
};

} // namespace Kernel
