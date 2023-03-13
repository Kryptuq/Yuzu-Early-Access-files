// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <cstddef>
#include <numeric>
#include <thread>
#include <vector>
#include <catch2/catch_test_macros.hpp>
#include "common/ring_buffer.h"

namespace Common {

TEST_CASE("RingBuffer: Basic Tests", "[common]") {
    RingBuffer<char, 4> buf;

    // Pushing values into a ring buffer with space should succeed.
    for (std::size_t i = 0; i < 4; i++) {
        const char elem = static_cast<char>(i);
        const std::size_t count = buf.Push(&elem, 1);
        REQUIRE(count == 1U);
    }

    REQUIRE(buf.Size() == 4U);

    // Pushing values into a full ring buffer should fail.
    {
        const char elem = static_cast<char>(42);
        const std::size_t count = buf.Push(&elem, 1);
        REQUIRE(count == 0U);
    }

    REQUIRE(buf.Size() == 4U);

    // Popping multiple values from a ring buffer with values should succeed.
    {
        const std::vector<char> popped = buf.Pop(2);
        REQUIRE(popped.size() == 2U);
        REQUIRE(popped[0] == 0);
        REQUIRE(popped[1] == 1);
    }

    REQUIRE(buf.Size() == 2U);

    // Popping a single value from a ring buffer with values should succeed.
    {
        const std::vector<char> popped = buf.Pop(1);
        REQUIRE(popped.size() == 1U);
        REQUIRE(popped[0] == 2);
    }

    REQUIRE(buf.Size() == 1U);

    // Pushing more values than space available should partially succeed.
    {
        std::vector<char> to_push(6);
        std::iota(to_push.begin(), to_push.end(), 88);
        const std::size_t count = buf.Push(to_push);
        REQUIRE(count == 3U);
    }

    REQUIRE(buf.Size() == 4U);

    // Doing an unlimited pop should pop all values.
    {
        const std::vector<char> popped = buf.Pop();
        REQUIRE(popped.size() == 4U);
        REQUIRE(popped[0] == 3);
        REQUIRE(popped[1] == 88);
        REQUIRE(popped[2] == 89);
        REQUIRE(popped[3] == 90);
    }

    REQUIRE(buf.Size() == 0U);
}

TEST_CASE("RingBuffer: Threaded Test", "[common]") {
    RingBuffer<char, 8> buf;
    const char seed = 42;
    const std::size_t count = 1000000;
    std::size_t full = 0;
    std::size_t empty = 0;

    const auto next_value = [](std::array<char, 2>& value) {
        value[0] += 1;
        value[1] += 2;
    };

    std::thread producer{[&] {
        std::array<char, 2> value = {seed, seed};
        std::size_t i = 0;
        while (i < count) {
            if (const std::size_t c = buf.Push(&value[0], 2); c > 0) {
                REQUIRE(c == 2U);
                i++;
                next_value(value);
            } else {
                full++;
                std::this_thread::yield();
            }
        }
    }};

    std::thread consumer{[&] {
        std::array<char, 2> value = {seed, seed};
        std::size_t i = 0;
        while (i < count) {
            if (const std::vector<char> v = buf.Pop(2); v.size() > 0) {
                REQUIRE(v.size() == 2U);
                REQUIRE(v[0] == value[0]);
                REQUIRE(v[1] == value[1]);
                i++;
                next_value(value);
            } else {
                empty++;
                std::this_thread::yield();
            }
        }
    }};

    producer.join();
    consumer.join();

    REQUIRE(buf.Size() == 0U);
    printf("RingBuffer: Threaded Test: full: %zu, empty: %zu\n", full, empty);
}

} // namespace Common
