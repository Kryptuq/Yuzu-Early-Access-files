// SPDX-FileCopyrightText: Copyright 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <new>
#include <version>

#include "common/polyfill_thread.h"

namespace Common {

namespace detail {
constexpr size_t DefaultCapacity = 0x1000;
} // namespace detail

template <typename T, size_t Capacity = detail::DefaultCapacity>
class SPSCQueue {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of two.");

public:
    bool TryPush(T&& t) {
        const size_t write_index = m_write_index.load();

        // Check if we have free slots to write to.
        if ((write_index - m_read_index.load()) == Capacity) {
            return false;
        }

        // Determine the position to write to.
        const size_t pos = write_index % Capacity;

        // Push into the queue.
        m_data[pos] = std::move(t);

        // Increment the write index.
        ++m_write_index;

        // Notify the consumer that we have pushed into the queue.
        std::scoped_lock lock{cv_mutex};
        cv.notify_one();

        return true;
    }

    template <typename... Args>
    bool TryPush(Args&&... args) {
        const size_t write_index = m_write_index.load();

        // Check if we have free slots to write to.
        if ((write_index - m_read_index.load()) == Capacity) {
            return false;
        }

        // Determine the position to write to.
        const size_t pos = write_index % Capacity;

        // Emplace into the queue.
        std::construct_at(std::addressof(m_data[pos]), std::forward<Args>(args)...);

        // Increment the write index.
        ++m_write_index;

        // Notify the consumer that we have pushed into the queue.
        std::scoped_lock lock{cv_mutex};
        cv.notify_one();

        return true;
    }

    void Push(T&& t) {
        const size_t write_index = m_write_index.load();

        // Wait until we have free slots to write to.
        while ((write_index - m_read_index.load()) == Capacity) {
            std::this_thread::yield();
        }

        // Determine the position to write to.
        const size_t pos = write_index % Capacity;

        // Push into the queue.
        m_data[pos] = std::move(t);

        // Increment the write index.
        ++m_write_index;

        // Notify the consumer that we have pushed into the queue.
        std::scoped_lock lock{cv_mutex};
        cv.notify_one();
    }

    template <typename... Args>
    void Push(Args&&... args) {
        const size_t write_index = m_write_index.load();

        // Wait until we have free slots to write to.
        while ((write_index - m_read_index.load()) == Capacity) {
            std::this_thread::yield();
        }

        // Determine the position to write to.
        const size_t pos = write_index % Capacity;

        // Emplace into the queue.
        std::construct_at(std::addressof(m_data[pos]), std::forward<Args>(args)...);

        // Increment the write index.
        ++m_write_index;

        // Notify the consumer that we have pushed into the queue.
        std::scoped_lock lock{cv_mutex};
        cv.notify_one();
    }

    bool TryPop(T& t) {
        return Pop(t);
    }

    void PopWait(T& t, std::stop_token stop_token) {
        Wait(stop_token);
        Pop(t);
    }

    T PopWait(std::stop_token stop_token) {
        Wait(stop_token);
        T t;
        Pop(t);
        return t;
    }

    void Clear() {
        while (!Empty()) {
            Pop();
        }
    }

    bool Empty() const {
        return m_read_index.load() == m_write_index.load();
    }

    size_t Size() const {
        return m_write_index.load() - m_read_index.load();
    }

private:
    void Pop() {
        const size_t read_index = m_read_index.load();

        // Check if the queue is empty.
        if (read_index == m_write_index.load()) {
            return;
        }

        // Determine the position to read from.
        const size_t pos = read_index % Capacity;

        // Pop the data off the queue, deleting it.
        std::destroy_at(std::addressof(m_data[pos]));

        // Increment the read index.
        ++m_read_index;
    }

    bool Pop(T& t) {
        const size_t read_index = m_read_index.load();

        // Check if the queue is empty.
        if (read_index == m_write_index.load()) {
            return false;
        }

        // Determine the position to read from.
        const size_t pos = read_index % Capacity;

        // Pop the data off the queue, moving it.
        t = std::move(m_data[pos]);

        // Increment the read index.
        ++m_read_index;

        return true;
    }

    void Wait(std::stop_token stop_token) {
        std::unique_lock lock{cv_mutex};
        Common::CondvarWait(cv, lock, stop_token, [this] { return !Empty(); });
    }

#ifdef __cpp_lib_hardware_interference_size
    alignas(std::hardware_destructive_interference_size) std::atomic_size_t m_read_index{0};
    alignas(std::hardware_destructive_interference_size) std::atomic_size_t m_write_index{0};
#else
    alignas(64) std::atomic_size_t m_read_index{0};
    alignas(64) std::atomic_size_t m_write_index{0};
#endif

    std::array<T, Capacity> m_data;

    std::condition_variable_any cv;
    std::mutex cv_mutex;
};

template <typename T, size_t Capacity = detail::DefaultCapacity>
class MPSCQueue {
public:
    void TryPush(T&& t) {
        std::scoped_lock lock{write_mutex};
        spsc_queue.TryPush(std::move(t));
    }

    template <typename... Args>
    void TryPush(Args&&... args) {
        std::scoped_lock lock{write_mutex};
        spsc_queue.TryPush(std::forward<Args>(args)...);
    }

    void Push(T&& t) {
        std::scoped_lock lock{write_mutex};
        spsc_queue.Push(std::move(t));
    }

    template <typename... Args>
    void Push(Args&&... args) {
        std::scoped_lock lock{write_mutex};
        spsc_queue.Push(std::forward<Args>(args)...);
    }

    bool TryPop(T& t) {
        return spsc_queue.TryPop(t);
    }

    void PopWait(T& t, std::stop_token stop_token) {
        spsc_queue.PopWait(t, stop_token);
    }

    T PopWait(std::stop_token stop_token) {
        return spsc_queue.PopWait(stop_token);
    }

    void Clear() {
        spsc_queue.Clear();
    }

    bool Empty() {
        return spsc_queue.Empty();
    }

    size_t Size() {
        return spsc_queue.Size();
    }

private:
    SPSCQueue<T, Capacity> spsc_queue;
    std::mutex write_mutex;
};

template <typename T, size_t Capacity = detail::DefaultCapacity>
class MPMCQueue {
public:
    void TryPush(T&& t) {
        std::scoped_lock lock{write_mutex};
        spsc_queue.TryPush(std::move(t));
    }

    template <typename... Args>
    void TryPush(Args&&... args) {
        std::scoped_lock lock{write_mutex};
        spsc_queue.TryPush(std::forward<Args>(args)...);
    }

    void Push(T&& t) {
        std::scoped_lock lock{write_mutex};
        spsc_queue.Push(std::move(t));
    }

    template <typename... Args>
    void Push(Args&&... args) {
        std::scoped_lock lock{write_mutex};
        spsc_queue.Push(std::forward<Args>(args)...);
    }

    bool TryPop(T& t) {
        std::scoped_lock lock{read_mutex};
        return spsc_queue.TryPop(t);
    }

    void PopWait(T& t, std::stop_token stop_token) {
        std::scoped_lock lock{read_mutex};
        spsc_queue.PopWait(t, stop_token);
    }

    T PopWait(std::stop_token stop_token) {
        std::scoped_lock lock{read_mutex};
        return spsc_queue.PopWait(stop_token);
    }

    void Clear() {
        std::scoped_lock lock{read_mutex};
        spsc_queue.Clear();
    }

    bool Empty() {
        std::scoped_lock lock{read_mutex};
        return spsc_queue.Empty();
    }

    size_t Size() {
        std::scoped_lock lock{read_mutex};
        return spsc_queue.Size();
    }

private:
    SPSCQueue<T, Capacity> spsc_queue;
    std::mutex write_mutex;
    std::mutex read_mutex;
};

} // namespace Common
