// SPDX-FileCopyrightText: Copyright (c) 2020 Erik Rigtorp <erik@rigtorp.se>
// SPDX-License-Identifier: MIT
#pragma once
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4324)
#endif

#include <atomic>
#include <bit>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <new>
#include <stdexcept>
#include <stop_token>
#include <type_traits>
#include <utility>

namespace Common {
namespace mpsc {
#if defined(__cpp_lib_hardware_interference_size)
constexpr size_t hardware_interference_size = std::hardware_destructive_interference_size;
#else
constexpr size_t hardware_interference_size = 64;
#endif

template <typename T>
using AlignedAllocator = std::allocator<T>;

template <typename T>
struct Slot {
    ~Slot() noexcept {
        if (turn.test()) {
            destroy();
        }
    }

    template <typename... Args>
    void construct(Args&&... args) noexcept {
        static_assert(std::is_nothrow_constructible_v<T, Args&&...>,
                      "T must be nothrow constructible with Args&&...");
        std::construct_at(reinterpret_cast<T*>(&storage), std::forward<Args>(args)...);
    }

    void destroy() noexcept {
        static_assert(std::is_nothrow_destructible_v<T>, "T must be nothrow destructible");
        std::destroy_at(reinterpret_cast<T*>(&storage));
    }

    T&& move() noexcept {
        return reinterpret_cast<T&&>(storage);
    }

    // Align to avoid false sharing between adjacent slots
    alignas(hardware_interference_size) std::atomic_flag turn{};
    struct aligned_store {
        struct type {
            alignas(T) unsigned char data[sizeof(T)];
        };
    };
    typename aligned_store::type storage;
};

template <typename T, typename Allocator = AlignedAllocator<Slot<T>>>
class Queue {
public:
    explicit Queue(const size_t capacity, const Allocator& allocator = Allocator())
        : allocator_(allocator) {
        if (capacity < 1) {
            throw std::invalid_argument("capacity < 1");
        }
        // Ensure that the queue length is an integer power of 2
        // This is so that idx(i) can be a simple i & mask_ insted of i % capacity
        // https://github.com/rigtorp/MPMCQueue/pull/36
        if (!std::has_single_bit(capacity)) {
            throw std::invalid_argument("capacity must be an integer power of 2");
        }

        mask_ = capacity - 1;

        // Allocate one extra slot to prevent false sharing on the last slot
        slots_ = allocator_.allocate(mask_ + 2);
        // Allocators are not required to honor alignment for over-aligned types
        // (see http://eel.is/c++draft/allocator.requirements#10) so we verify
        // alignment here
        if (reinterpret_cast<uintptr_t>(slots_) % alignof(Slot<T>) != 0) {
            allocator_.deallocate(slots_, mask_ + 2);
            throw std::bad_alloc();
        }
        for (size_t i = 0; i < mask_ + 1; ++i) {
            std::construct_at(&slots_[i]);
        }
        static_assert(alignof(Slot<T>) == hardware_interference_size,
                      "Slot must be aligned to cache line boundary to prevent false sharing");
        static_assert(sizeof(Slot<T>) % hardware_interference_size == 0,
                      "Slot size must be a multiple of cache line size to prevent "
                      "false sharing between adjacent slots");
        static_assert(sizeof(Queue) % hardware_interference_size == 0,
                      "Queue size must be a multiple of cache line size to "
                      "prevent false sharing between adjacent queues");
    }

    ~Queue() noexcept {
        for (size_t i = 0; i < mask_ + 1; ++i) {
            slots_[i].~Slot();
        }
        allocator_.deallocate(slots_, mask_ + 2);
    }

    // non-copyable and non-movable
    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;

    void Push(const T& v) noexcept {
        static_assert(std::is_nothrow_copy_constructible_v<T>,
                      "T must be nothrow copy constructible");
        emplace(v);
    }

    template <typename P, typename = std::enable_if_t<std::is_nothrow_constructible_v<T, P&&>>>
    void Push(P&& v) noexcept {
        emplace(std::forward<P>(v));
    }

    void Pop(T& v, std::stop_token stop) noexcept {
        auto const tail = tail_.fetch_add(1);
        auto& slot = slots_[idx(tail)];
        if (false == slot.turn.test()) {
            std::unique_lock lock{cv_mutex};
            cv.wait(lock, stop, [&slot] { return slot.turn.test(); });
        }
        v = slot.move();
        slot.destroy();
        slot.turn.clear();
        slot.turn.notify_one();
    }

private:
    template <typename... Args>
    void emplace(Args&&... args) noexcept {
        static_assert(std::is_nothrow_constructible_v<T, Args&&...>,
                      "T must be nothrow constructible with Args&&...");
        auto const head = head_.fetch_add(1);
        auto& slot = slots_[idx(head)];
        slot.turn.wait(true);
        slot.construct(std::forward<Args>(args)...);
        slot.turn.test_and_set();
        cv.notify_one();
    }

    constexpr size_t idx(size_t i) const noexcept {
        return i & mask_;
    }

    std::conditional_t<true, std::condition_variable_any, std::condition_variable> cv;
    std::mutex cv_mutex;
    size_t mask_;
    Slot<T>* slots_;
    [[no_unique_address]] Allocator allocator_;

    // Align to avoid false sharing between head_ and tail_
    alignas(hardware_interference_size) std::atomic<size_t> head_{0};
    alignas(hardware_interference_size) std::atomic<size_t> tail_{0};

    static_assert(std::is_nothrow_copy_assignable_v<T> || std::is_nothrow_move_assignable_v<T>,
                  "T must be nothrow copy or move assignable");

    static_assert(std::is_nothrow_destructible_v<T>, "T must be nothrow destructible");
};
} // namespace mpsc

template <typename T, typename Allocator = mpsc::AlignedAllocator<mpsc::Slot<T>>>
using MPSCQueue = mpsc::Queue<T, Allocator>;

} // namespace Common

#ifdef _MSC_VER
#pragma warning(pop)
#endif
