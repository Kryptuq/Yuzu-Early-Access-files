// Copyright 2010 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include <atomic>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <optional>

namespace Common {

/// a more foolproof multiple reader, multiple writer queue
template <typename T>
class MPMCQueue {
#define ABORT()                                                                                    \
    do {                                                                                           \
        std::cerr << __FILE__ " ERR " << __LINE__ << std::endl;                                    \
        abort();                                                                                   \
    } while (0)
public:
    ~MPMCQueue() {
        Clear();
        if (waiting || head || tail) {
            // Remove all the ABORT() after 1 month merged without problems
            ABORT();
        }
    }

    template <typename Arg>
    void Push(Arg&& t) {
        Node* const node = new Node(std::forward<Arg>(t));
        if (!node || node == PLACEHOLDER) {
            ABORT();
        }
        while (true) {
            if (Node* const previous = tail.load(ACQUIRE)) {
                if (Node* exchange = nullptr;
                    !previous->next.compare_exchange_weak(exchange, node, ACQ_REL)) {
                    continue;
                }
                if (tail.exchange(node, ACQ_REL) != previous) {
                    ABORT();
                }
            } else {
                if (Node* exchange = nullptr;
                    !tail.compare_exchange_weak(exchange, node, ACQ_REL)) {
                    continue;
                }
                for (Node* exchange = nullptr;
                     !head.compare_exchange_weak(exchange, node, ACQ_REL);)
                    ;
            }
            break;
        }
        if (waiting.load(ACQUIRE)) {
            std::lock_guard lock{mutex};
            condition.notify_one();
        }
    }

    bool Pop(T& t) {
        return PopImpl<false>(t);
    }

    T PopWait() {
        T t;
        if (!PopImpl<true>(t)) {
            ABORT();
        }
        return t;
    }

    void Wait() {
        if (head.load(ACQUIRE)) {
            return;
        }
        static_cast<void>(waiting.fetch_add(1, ACQ_REL));
        std::unique_lock lock{mutex};
        while (true) {
            if (head.load(ACQUIRE)) {
                break;
            }
            condition.wait(lock);
        }
        if (!waiting.fetch_sub(1, ACQ_REL)) {
            ABORT();
        }
    }

    void Clear() {
        while (true) {
            Node* const last = tail.load(ACQUIRE);
            if (!last) {
                return;
            }
            if (Node* exchange = nullptr;
                !last->next.compare_exchange_weak(exchange, PLACEHOLDER, ACQ_REL)) {
                continue;
            }
            if (tail.exchange(nullptr, ACQ_REL) != last) {
                ABORT();
            }
            Node* node = head.exchange(nullptr, ACQ_REL);
            while (node && node != PLACEHOLDER) {
                Node* next = node->next.load(ACQUIRE);
                delete node;
                node = next;
            }
            return;
        }
    }

private:
    template <bool WAIT>
    bool PopImpl(T& t) {
        std::optional<std::unique_lock<std::mutex>> lock{std::nullopt};
        while (true) {
            Node* const node = head.load(ACQUIRE);
            if (!node) {
                if constexpr (!WAIT) {
                    return false;
                }
                if (!lock) {
                    static_cast<void>(waiting.fetch_add(1, ACQ_REL));
                    lock = std::unique_lock{mutex};
                    continue;
                }
                condition.wait(*lock);
                continue;
            }
            Node* const next = node->next.load(ACQUIRE);
            if (next) {
                if (next == PLACEHOLDER) {
                    continue;
                }
                if (Node* exchange = node; !head.compare_exchange_weak(exchange, next, ACQ_REL)) {
                    continue;
                }
            } else {
                if (Node* exchange = nullptr;
                    !node->next.compare_exchange_weak(exchange, PLACEHOLDER, ACQ_REL)) {
                    continue;
                }
                if (tail.exchange(nullptr, ACQ_REL) != node) {
                    ABORT();
                }
                if (head.exchange(nullptr, ACQ_REL) != node) {
                    ABORT();
                }
            }
            t = std::move(node->value);
            delete node;
            if (lock) {
                if (!waiting.fetch_sub(1, ACQ_REL)) {
                    ABORT();
                }
            }
            return true;
        }
    }

    struct Node {
        template <typename Arg>
        explicit Node(Arg&& t) : value{std::forward<Arg>(t)} {}

        Node(const Node&) = delete;
        Node& operator=(const Node&) = delete;

        Node(Node&&) = delete;
        Node& operator=(Node&&) = delete;

        const T value;
        std::atomic<Node*> next{nullptr};
    };

    // We only need to avoid SEQ_CST on X86
    // We can add RELAXED later if we port to ARM and it's too slow
    static constexpr auto ACQUIRE = std::memory_order_acquire;
    static constexpr auto ACQ_REL = std::memory_order_acq_rel;
    static inline const auto PLACEHOLDER = reinterpret_cast<Node*>(1);

    std::atomic<Node*> head{nullptr};
    std::atomic<Node*> tail{nullptr};

    std::atomic_size_t waiting{0};
    std::condition_variable condition{};
    std::mutex mutex{};
#undef ABORT
};

/// a simple lockless thread-safe,
/// single reader, single writer queue
template <typename T>
class /*[[deprecated("Transition to MPMCQueue")]]*/ SPSCQueue {
public:
    template <typename Arg>
    void Push(Arg&& t) {
        queue.Push(std::forward<Arg>(t));
    }

    bool Pop(T& t) {
        return queue.Pop(t);
    }

    void Wait() {
        queue.Wait();
    }

    T PopWait() {
        return queue.PopWait();
    }

    void Clear() {
        queue.Clear();
    }

private:
    MPMCQueue<T> queue{};
};

/// a simple thread-safe,
/// single reader, multiple writer queue
template <typename T>
class /*[[deprecated("Transition to MPMCQueue")]]*/ MPSCQueue {
public:
    template <typename Arg>
    void Push(Arg&& t) {
        queue.Push(std::forward<Arg>(t));
    }

    bool Pop(T& t) {
        return queue.Pop(t);
    }

    T PopWait() {
        return queue.PopWait();
    }

private:
    MPMCQueue<T> queue{};
};
} // namespace Common
