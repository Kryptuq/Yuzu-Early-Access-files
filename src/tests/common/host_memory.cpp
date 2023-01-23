// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <catch2/catch_test_macros.hpp>

#include "common/host_memory.h"
#include "common/literals.h"

using Common::HostMemory;
using namespace Common::Literals;

static constexpr size_t VIRTUAL_SIZE = 1ULL << 39;
static constexpr size_t BACKING_SIZE = 4_GiB;

TEST_CASE("HostMemory: Initialize and deinitialize", "[common]") {
    { HostMemory mem(BACKING_SIZE, VIRTUAL_SIZE); }
    { HostMemory mem(BACKING_SIZE, VIRTUAL_SIZE); }
}

TEST_CASE("HostMemory: Simple map", "[common]") {
    HostMemory mem(BACKING_SIZE, VIRTUAL_SIZE);
    mem.Map(0x5000, 0x8000, 0x1000);

    volatile u8* const data = mem.VirtualBasePointer() + 0x5000;
    data[0] = 50;
    REQUIRE(data[0] == 50);
}

TEST_CASE("HostMemory: Simple mirror map", "[common]") {
    HostMemory mem(BACKING_SIZE, VIRTUAL_SIZE);
    mem.Map(0x5000, 0x3000, 0x2000);
    mem.Map(0x8000, 0x4000, 0x1000);

    volatile u8* const mirror_a = mem.VirtualBasePointer() + 0x5000;
    volatile u8* const mirror_b = mem.VirtualBasePointer() + 0x8000;
    mirror_b[0] = 76;
    REQUIRE(mirror_a[0x1000] == 76);
}

TEST_CASE("HostMemory: Simple unmap", "[common]") {
    HostMemory mem(BACKING_SIZE, VIRTUAL_SIZE);
    mem.Map(0x5000, 0x3000, 0x2000);

    volatile u8* const data = mem.VirtualBasePointer() + 0x5000;
    data[75] = 50;
    REQUIRE(data[75] == 50);

    mem.Unmap(0x5000, 0x2000);
}

TEST_CASE("HostMemory: Simple unmap and remap", "[common]") {
    HostMemory mem(BACKING_SIZE, VIRTUAL_SIZE);
    mem.Map(0x5000, 0x3000, 0x2000);

    volatile u8* const data = mem.VirtualBasePointer() + 0x5000;
    data[0] = 50;
    REQUIRE(data[0] == 50);

    mem.Unmap(0x5000, 0x2000);

    mem.Map(0x5000, 0x3000, 0x2000);
    REQUIRE(data[0] == 50);

    mem.Map(0x7000, 0x2000, 0x5000);
    REQUIRE(data[0x3000] == 50);
}

TEST_CASE("HostMemory: Nieche allocation", "[common]") {
    HostMemory mem(BACKING_SIZE, VIRTUAL_SIZE);
    mem.Map(0x0000, 0, 0x20000);
    mem.Unmap(0x0000, 0x4000);
    mem.Map(0x1000, 0, 0x2000);
    mem.Map(0x3000, 0, 0x1000);
    mem.Map(0, 0, 0x1000);
}

TEST_CASE("HostMemory: Full unmap", "[common]") {
    HostMemory mem(BACKING_SIZE, VIRTUAL_SIZE);
    mem.Map(0x8000, 0, 0x4000);
    mem.Unmap(0x8000, 0x4000);
    mem.Map(0x6000, 0, 0x16000);
}

TEST_CASE("HostMemory: Right out of bounds unmap", "[common]") {
    HostMemory mem(BACKING_SIZE, VIRTUAL_SIZE);
    mem.Map(0x0000, 0, 0x4000);
    mem.Unmap(0x2000, 0x4000);
    mem.Map(0x2000, 0x80000, 0x4000);
}

TEST_CASE("HostMemory: Left out of bounds unmap", "[common]") {
    HostMemory mem(BACKING_SIZE, VIRTUAL_SIZE);
    mem.Map(0x8000, 0, 0x4000);
    mem.Unmap(0x6000, 0x4000);
    mem.Map(0x8000, 0, 0x2000);
}

TEST_CASE("HostMemory: Multiple placeholder unmap", "[common]") {
    HostMemory mem(BACKING_SIZE, VIRTUAL_SIZE);
    mem.Map(0x0000, 0, 0x4000);
    mem.Map(0x4000, 0, 0x1b000);
    mem.Unmap(0x3000, 0x1c000);
    mem.Map(0x3000, 0, 0x20000);
}

TEST_CASE("HostMemory: Unmap between placeholders", "[common]") {
    HostMemory mem(BACKING_SIZE, VIRTUAL_SIZE);
    mem.Map(0x0000, 0, 0x4000);
    mem.Map(0x4000, 0, 0x4000);
    mem.Unmap(0x2000, 0x4000);
    mem.Map(0x2000, 0, 0x4000);
}

TEST_CASE("HostMemory: Unmap to origin", "[common]") {
    HostMemory mem(BACKING_SIZE, VIRTUAL_SIZE);
    mem.Map(0x4000, 0, 0x4000);
    mem.Map(0x8000, 0, 0x4000);
    mem.Unmap(0x4000, 0x4000);
    mem.Map(0, 0, 0x4000);
    mem.Map(0x4000, 0, 0x4000);
}

TEST_CASE("HostMemory: Unmap to right", "[common]") {
    HostMemory mem(BACKING_SIZE, VIRTUAL_SIZE);
    mem.Map(0x4000, 0, 0x4000);
    mem.Map(0x8000, 0, 0x4000);
    mem.Unmap(0x8000, 0x4000);
    mem.Map(0x8000, 0, 0x4000);
}

TEST_CASE("HostMemory: Partial right unmap check bindings", "[common]") {
    HostMemory mem(BACKING_SIZE, VIRTUAL_SIZE);
    mem.Map(0x4000, 0x10000, 0x4000);

    volatile u8* const ptr = mem.VirtualBasePointer() + 0x4000;
    ptr[0x1000] = 17;

    mem.Unmap(0x6000, 0x2000);

    REQUIRE(ptr[0x1000] == 17);
}

TEST_CASE("HostMemory: Partial left unmap check bindings", "[common]") {
    HostMemory mem(BACKING_SIZE, VIRTUAL_SIZE);
    mem.Map(0x4000, 0x10000, 0x4000);

    volatile u8* const ptr = mem.VirtualBasePointer() + 0x4000;
    ptr[0x3000] = 19;
    ptr[0x3fff] = 12;

    mem.Unmap(0x4000, 0x2000);

    REQUIRE(ptr[0x3000] == 19);
    REQUIRE(ptr[0x3fff] == 12);
}

TEST_CASE("HostMemory: Partial middle unmap check bindings", "[common]") {
    HostMemory mem(BACKING_SIZE, VIRTUAL_SIZE);
    mem.Map(0x4000, 0x10000, 0x4000);

    volatile u8* const ptr = mem.VirtualBasePointer() + 0x4000;
    ptr[0x0000] = 19;
    ptr[0x3fff] = 12;

    mem.Unmap(0x1000, 0x2000);

    REQUIRE(ptr[0x0000] == 19);
    REQUIRE(ptr[0x3fff] == 12);
}

TEST_CASE("HostMemory: Partial sparse middle unmap and check bindings", "[common]") {
    HostMemory mem(BACKING_SIZE, VIRTUAL_SIZE);
    mem.Map(0x4000, 0x10000, 0x2000);
    mem.Map(0x6000, 0x20000, 0x2000);

    volatile u8* const ptr = mem.VirtualBasePointer() + 0x4000;
    ptr[0x0000] = 19;
    ptr[0x3fff] = 12;

    mem.Unmap(0x5000, 0x2000);

    REQUIRE(ptr[0x0000] == 19);
    REQUIRE(ptr[0x3fff] == 12);
}
