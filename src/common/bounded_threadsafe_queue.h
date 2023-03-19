// SPDX-FileCopyrightText: Copyright (c) 2020 Erik Rigtorp <erik@rigtorp.se>
// SPDX-License-Identifier: MIT

#pragma once

#include <atomic>
#include <bit>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <new>
#include <type_traits>
#include <utility>

#include "common/polyfill_thread.h"

namespace Common {

#if defined(__cpp_lib_hardware_interference_size)
constexpr size_t hardware_interference_size = std::hardware_destructive_interference_size;
#else
constexpr size_t hardware_interference_size = 64;
#endif

template <typename T, size_t capacity = 0x400>
class MPSCQueue {
public:
    explicit MPSCQueue() : allocator{std::allocator<Slot<T>>()} {
        // Allocate one extra slot to prevent false sharing on the last slot
        slots = allocator.allocate(capacity + 1);
        // Allocators are not required to honor alignment for over-aligned types
        // (see http://eel.is/c++draft/allocator.requirements#10) so we verify
        // alignment here
        if (reinterpret_cast<uintptr_t>(slots) % alignof(Slot<T>) != 0) {
            allocator.deallocate(slots, capacity + 1);
            throw std::bad_alloc();
        }
        for (size_t i = 0; i < capacity; ++i) {
            std::construct_at(&slots[i]);
        }
        static_assert(std::has_single_bit(capacity), "capacity must be an integer power of 2");
        static_assert(alignof(Slot<T>) == hardware_interference_size,
                      "Slot must be aligned to cache line boundary to prevent false sharing");
        static_assert(sizeof(Slot<T>) % hardware_interference_size == 0,
                      "Slot size must be a multiple of cache line size to prevent "
                      "false sharing between adjacent slots");
        static_assert(sizeof(MPSCQueue) % hardware_interference_size == 0,
                      "Queue size must be a multiple of cache line size to "
                      "prevent false sharing between adjacent queues");
    }

    ~MPSCQueue() noexcept {
        for (size_t i = 0; i < capacity; ++i) {
            std::destroy_at(&slots[i]);
        }
        allocator.deallocate(slots, capacity + 1);
    }

    // The queue must be both non-copyable and non-movable
    MPSCQueue(const MPSCQueue&) = delete;
    MPSCQueue& operator=(const MPSCQueue&) = delete;

    MPSCQueue(MPSCQueue&&) = delete;
    MPSCQueue& operator=(MPSCQueue&&) = delete;

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
        auto& slot = slots[idx(tail)];
        if (!slot.turn.test()) {
            std::unique_lock lock{cv_mutex};
            Common::CondvarWait(cv, lock, stop, [&slot] { return slot.turn.test(); });
        }
        v = slot.move();
        slot.destroy();
        slot.turn.clear();
        slot.turn.notify_one();
    }

private:
    template <typename U = T>
    struct Slot {
        ~Slot() noexcept {
            if (turn.test()) {
                destroy();
            }
        }

        template <typename... Args>
        void construct(Args&&... args) noexcept {
            static_assert(std::is_nothrow_constructible_v<U, Args&&...>,
                          "T must be nothrow constructible with Args&&...");
            std::construct_at(reinterpret_cast<U*>(&storage), std::forward<Args>(args)...);
        }

        void destroy() noexcept {
            static_assert(std::is_nothrow_destructible_v<U>, "T must be nothrow destructible");
            std::destroy_at(reinterpret_cast<U*>(&storage));
        }

        U&& move() noexcept {
            return reinterpret_cast<U&&>(storage);
        }

        // Align to avoid false sharing between adjacent slots
        alignas(hardware_interference_size) std::atomic_flag turn{};
        struct aligned_store {
            struct type {
                alignas(U) unsigned char data[sizeof(U)];
            };
        };
        typename aligned_store::type storage;
    };

    template <typename... Args>
    void emplace(Args&&... args) noexcept {
        static_assert(std::is_nothrow_constructible_v<T, Args&&...>,
                      "T must be nothrow constructible with Args&&...");
        auto const head = head_.fetch_add(1);
        auto& slot = slots[idx(head)];
        slot.turn.wait(true);
        slot.construct(std::forward<Args>(args)...);
        slot.turn.test_and_set();
        cv.notify_one();
    }

    constexpr size_t idx(size_t i) const noexcept {
        return i & mask;
    }

    static constexpr size_t mask = capacity - 1;

    // Align to avoid false sharing between head_ and tail_
    alignas(hardware_interference_size) std::atomic<size_t> head_{0};
    alignas(hardware_interference_size) std::atomic<size_t> tail_{0};

    std::mutex cv_mutex;
    std::condition_variable_any cv;

    Slot<T>* slots;
    [[no_unique_address]] std::allocator<Slot<T>> allocator;

    static_assert(std::is_nothrow_copy_assignable_v<T> || std::is_nothrow_move_assignable_v<T>,
                  "T must be nothrow copy or move assignable");

    static_assert(std::is_nothrow_destructible_v<T>, "T must be nothrow destructible");
};

} // namespace Common
