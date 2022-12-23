// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <span>
#include <catch2/catch.hpp>
#include "common/common_types.h"
#include "common/scratch_buffer.h"

namespace Common {

TEST_CASE("ScratchBuffer: Basic Test", "[common]") {
    ScratchBuffer<u8> buf;

    REQUIRE(buf.size() == 0U);
    REQUIRE(buf.capacity() == 0U);

    std::array<u8, 10> payload;
    payload.fill(66);

    buf.resize(payload.size());
    REQUIRE(buf.size() == payload.size());
    REQUIRE(buf.capacity() == payload.size());

    std::memcpy(buf.data(), payload.data(), payload.size());
    for (size_t i = 0; i < payload.size(); ++i) {
        REQUIRE(buf[i] == payload[i]);
    }
}

TEST_CASE("ScratchBuffer: resize_destructive Grow", "[common]") {
    std::array<u8, 10> payload;
    payload.fill(66);

    ScratchBuffer<u8> buf(payload.size());
    REQUIRE(buf.size() == payload.size());
    REQUIRE(buf.capacity() == payload.size());

    // Increasing the size should reallocate the buffer
    buf.resize_destructive(payload.size() * 2);
    REQUIRE(buf.size() == payload.size() * 2);
    REQUIRE(buf.capacity() == payload.size() * 2);

    // Since the buffer is not value initialized, reading its data will be garbage
}

TEST_CASE("ScratchBuffer: resize_destructive Shrink", "[common]") {
    std::array<u8, 10> payload;
    payload.fill(66);

    ScratchBuffer<u8> buf(payload.size());
    REQUIRE(buf.size() == payload.size());
    REQUIRE(buf.capacity() == payload.size());

    std::memcpy(buf.data(), payload.data(), payload.size());
    for (size_t i = 0; i < payload.size(); ++i) {
        REQUIRE(buf[i] == payload[i]);
    }

    // Decreasing the size should not cause a buffer reallocation
    // This can be tested by ensuring the buffer capacity and data has not changed,
    buf.resize_destructive(1U);
    REQUIRE(buf.size() == 1U);
    REQUIRE(buf.capacity() == payload.size());

    for (size_t i = 0; i < payload.size(); ++i) {
        REQUIRE(buf[i] == payload[i]);
    }
}

TEST_CASE("ScratchBuffer: resize Grow u8", "[common]") {
    std::array<u8, 10> payload;
    payload.fill(66);

    ScratchBuffer<u8> buf(payload.size());
    REQUIRE(buf.size() == payload.size());
    REQUIRE(buf.capacity() == payload.size());

    std::memcpy(buf.data(), payload.data(), payload.size());
    for (size_t i = 0; i < payload.size(); ++i) {
        REQUIRE(buf[i] == payload[i]);
    }

    // Increasing the size should reallocate the buffer
    buf.resize(payload.size() * 2);
    REQUIRE(buf.size() == payload.size() * 2);
    REQUIRE(buf.capacity() == payload.size() * 2);

    // resize() keeps the previous data intact
    for (size_t i = 0; i < payload.size(); ++i) {
        REQUIRE(buf[i] == payload[i]);
    }
}

TEST_CASE("ScratchBuffer: resize Grow u64", "[common]") {
    std::array<u64, 10> payload;
    payload.fill(6666);

    ScratchBuffer<u64> buf(payload.size());
    REQUIRE(buf.size() == payload.size());
    REQUIRE(buf.capacity() == payload.size());

    std::memcpy(buf.data(), payload.data(), payload.size() * sizeof(u64));
    for (size_t i = 0; i < payload.size(); ++i) {
        REQUIRE(buf[i] == payload[i]);
    }

    // Increasing the size should reallocate the buffer
    buf.resize(payload.size() * 2);
    REQUIRE(buf.size() == payload.size() * 2);
    REQUIRE(buf.capacity() == payload.size() * 2);

    // resize() keeps the previous data intact
    for (size_t i = 0; i < payload.size(); ++i) {
        REQUIRE(buf[i] == payload[i]);
    }
}

TEST_CASE("ScratchBuffer: resize Shrink", "[common]") {
    std::array<u8, 10> payload;
    payload.fill(66);

    ScratchBuffer<u8> buf(payload.size());
    REQUIRE(buf.size() == payload.size());
    REQUIRE(buf.capacity() == payload.size());

    std::memcpy(buf.data(), payload.data(), payload.size());
    for (size_t i = 0; i < payload.size(); ++i) {
        REQUIRE(buf[i] == payload[i]);
    }

    // Decreasing the size should not cause a buffer reallocation
    // This can be tested by ensuring the buffer capacity and data has not changed,
    buf.resize(1U);
    REQUIRE(buf.size() == 1U);
    REQUIRE(buf.capacity() == payload.size());

    for (size_t i = 0; i < payload.size(); ++i) {
        REQUIRE(buf[i] == payload[i]);
    }
}

TEST_CASE("ScratchBuffer: Span Size", "[common]") {
    std::array<u8, 10> payload;
    payload.fill(66);

    ScratchBuffer<u8> buf(payload.size());
    REQUIRE(buf.size() == payload.size());
    REQUIRE(buf.capacity() == payload.size());

    std::memcpy(buf.data(), payload.data(), payload.size());
    for (size_t i = 0; i < payload.size(); ++i) {
        REQUIRE(buf[i] == payload[i]);
    }

    buf.resize(3U);
    REQUIRE(buf.size() == 3U);
    REQUIRE(buf.capacity() == payload.size());

    const auto buf_span = std::span<u8>(buf);
    // The span size is the last requested size of the buffer, not its capacity
    REQUIRE(buf_span.size() == buf.size());

    for (size_t i = 0; i < buf_span.size(); ++i) {
        REQUIRE(buf_span[i] == buf[i]);
        REQUIRE(buf_span[i] == payload[i]);
    }
}

TEST_CASE("ScratchBuffer: Span Writes", "[common]") {
    std::array<u8, 10> payload;
    payload.fill(66);

    ScratchBuffer<u8> buf(payload.size());
    REQUIRE(buf.size() == payload.size());
    REQUIRE(buf.capacity() == payload.size());

    std::memcpy(buf.data(), payload.data(), payload.size());
    for (size_t i = 0; i < payload.size(); ++i) {
        REQUIRE(buf[i] == payload[i]);
    }

    buf.resize(3U);
    REQUIRE(buf.size() == 3U);
    REQUIRE(buf.capacity() == payload.size());

    const auto buf_span = std::span<u8>(buf);
    REQUIRE(buf_span.size() == buf.size());

    for (size_t i = 0; i < buf_span.size(); ++i) {
        const auto new_value = static_cast<u8>(i + 1U);
        // Writes to a span of the scratch buffer will propogate to the buffer itself
        buf_span[i] = new_value;
        REQUIRE(buf[i] == new_value);
    }
}

} // namespace Common
