// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <stdexcept>
#include <unordered_map>

#include <catch2/catch.hpp>

#include "common/alignment.h"
#include "common/common_types.h"
#include "video_core/buffer_cache/buffer_base.h"

namespace {
using VideoCommon::BufferBase;
using Range = std::pair<u64, u64>;

constexpr u64 PAGE = 4096;
constexpr u64 WORD = 4096 * 64;

constexpr VAddr c = 0x1328914000;

class RasterizerInterface {
public:
    void UpdatePagesCachedCount(VAddr addr, u64 size, int delta) {
        const u64 page_start{addr >> Core::Memory::YUZU_PAGEBITS};
        const u64 page_end{(addr + size + Core::Memory::YUZU_PAGESIZE - 1) >>
                           Core::Memory::YUZU_PAGEBITS};
        for (u64 page = page_start; page < page_end; ++page) {
            int& value = page_table[page];
            value += delta;
            if (value < 0) {
                throw std::logic_error{"negative page"};
            }
            if (value == 0) {
                page_table.erase(page);
            }
        }
    }

    [[nodiscard]] int Count(VAddr addr) const noexcept {
        const auto it = page_table.find(addr >> Core::Memory::YUZU_PAGEBITS);
        return it == page_table.end() ? 0 : it->second;
    }

    [[nodiscard]] unsigned Count() const noexcept {
        unsigned count = 0;
        for (const auto [index, value] : page_table) {
            count += value;
        }
        return count;
    }

private:
    std::unordered_map<u64, int> page_table;
};
} // Anonymous namespace

TEST_CASE("BufferBase: Small buffer", "[video_core]") {
    RasterizerInterface rasterizer;
    BufferBase buffer(rasterizer, c, WORD);
    REQUIRE(rasterizer.Count() == 0);
    buffer.UnmarkRegionAsCpuModified(c, WORD);
    REQUIRE(rasterizer.Count() == WORD / PAGE);
    REQUIRE(buffer.ModifiedCpuRegion(c, WORD) == Range{0, 0});

    buffer.MarkRegionAsCpuModified(c + PAGE, 1);
    REQUIRE(buffer.ModifiedCpuRegion(c, WORD) == Range{PAGE * 1, PAGE * 2});
}

TEST_CASE("BufferBase: Large buffer", "[video_core]") {
    RasterizerInterface rasterizer;
    BufferBase buffer(rasterizer, c, WORD * 32);
    buffer.UnmarkRegionAsCpuModified(c, WORD * 32);
    buffer.MarkRegionAsCpuModified(c + 4096, WORD * 4);
    REQUIRE(buffer.ModifiedCpuRegion(c, WORD + PAGE * 2) == Range{PAGE, WORD + PAGE * 2});
    REQUIRE(buffer.ModifiedCpuRegion(c + PAGE * 2, PAGE * 6) == Range{PAGE * 2, PAGE * 8});
    REQUIRE(buffer.ModifiedCpuRegion(c, WORD * 32) == Range{PAGE, WORD * 4 + PAGE});
    REQUIRE(buffer.ModifiedCpuRegion(c + WORD * 4, PAGE) == Range{WORD * 4, WORD * 4 + PAGE});
    REQUIRE(buffer.ModifiedCpuRegion(c + WORD * 3 + PAGE * 63, PAGE) ==
            Range{WORD * 3 + PAGE * 63, WORD * 4});

    buffer.MarkRegionAsCpuModified(c + WORD * 5 + PAGE * 6, PAGE);
    buffer.MarkRegionAsCpuModified(c + WORD * 5 + PAGE * 8, PAGE);
    REQUIRE(buffer.ModifiedCpuRegion(c + WORD * 5, WORD) ==
            Range{WORD * 5 + PAGE * 6, WORD * 5 + PAGE * 9});

    buffer.UnmarkRegionAsCpuModified(c + WORD * 5 + PAGE * 8, PAGE);
    REQUIRE(buffer.ModifiedCpuRegion(c + WORD * 5, WORD) ==
            Range{WORD * 5 + PAGE * 6, WORD * 5 + PAGE * 7});

    buffer.MarkRegionAsCpuModified(c + PAGE, WORD * 31 + PAGE * 63);
    REQUIRE(buffer.ModifiedCpuRegion(c, WORD * 32) == Range{PAGE, WORD * 32});

    buffer.UnmarkRegionAsCpuModified(c + PAGE * 4, PAGE);
    buffer.UnmarkRegionAsCpuModified(c + PAGE * 6, PAGE);

    buffer.UnmarkRegionAsCpuModified(c, WORD * 32);
    REQUIRE(buffer.ModifiedCpuRegion(c, WORD * 32) == Range{0, 0});
}

TEST_CASE("BufferBase: Rasterizer counting", "[video_core]") {
    RasterizerInterface rasterizer;
    BufferBase buffer(rasterizer, c, PAGE * 2);
    REQUIRE(rasterizer.Count() == 0);
    buffer.UnmarkRegionAsCpuModified(c, PAGE);
    REQUIRE(rasterizer.Count() == 1);
    buffer.MarkRegionAsCpuModified(c, PAGE * 2);
    REQUIRE(rasterizer.Count() == 0);
    buffer.UnmarkRegionAsCpuModified(c, PAGE);
    buffer.UnmarkRegionAsCpuModified(c + PAGE, PAGE);
    REQUIRE(rasterizer.Count() == 2);
    buffer.MarkRegionAsCpuModified(c, PAGE * 2);
    REQUIRE(rasterizer.Count() == 0);
}

TEST_CASE("BufferBase: Basic range", "[video_core]") {
    RasterizerInterface rasterizer;
    BufferBase buffer(rasterizer, c, WORD);
    buffer.UnmarkRegionAsCpuModified(c, WORD);
    buffer.MarkRegionAsCpuModified(c, PAGE);
    int num = 0;
    buffer.ForEachUploadRange(c, WORD, [&](u64 offset, u64 size) {
        REQUIRE(offset == 0U);
        REQUIRE(size == PAGE);
        ++num;
    });
    REQUIRE(num == 1U);
}

TEST_CASE("BufferBase: Border upload", "[video_core]") {
    RasterizerInterface rasterizer;
    BufferBase buffer(rasterizer, c, WORD * 2);
    buffer.UnmarkRegionAsCpuModified(c, WORD * 2);
    buffer.MarkRegionAsCpuModified(c + WORD - PAGE, PAGE * 2);
    buffer.ForEachUploadRange(c, WORD * 2, [](u64 offset, u64 size) {
        REQUIRE(offset == WORD - PAGE);
        REQUIRE(size == PAGE * 2);
    });
}

TEST_CASE("BufferBase: Border upload range", "[video_core]") {
    RasterizerInterface rasterizer;
    BufferBase buffer(rasterizer, c, WORD * 2);
    buffer.UnmarkRegionAsCpuModified(c, WORD * 2);
    buffer.MarkRegionAsCpuModified(c + WORD - PAGE, PAGE * 2);
    buffer.ForEachUploadRange(c + WORD - PAGE, PAGE * 2, [](u64 offset, u64 size) {
        REQUIRE(offset == WORD - PAGE);
        REQUIRE(size == PAGE * 2);
    });
    buffer.MarkRegionAsCpuModified(c + WORD - PAGE, PAGE * 2);
    buffer.ForEachUploadRange(c + WORD - PAGE, PAGE, [](u64 offset, u64 size) {
        REQUIRE(offset == WORD - PAGE);
        REQUIRE(size == PAGE);
    });
    buffer.ForEachUploadRange(c + WORD, PAGE, [](u64 offset, u64 size) {
        REQUIRE(offset == WORD);
        REQUIRE(size == PAGE);
    });
}

TEST_CASE("BufferBase: Border upload partial range", "[video_core]") {
    RasterizerInterface rasterizer;
    BufferBase buffer(rasterizer, c, WORD * 2);
    buffer.UnmarkRegionAsCpuModified(c, WORD * 2);
    buffer.MarkRegionAsCpuModified(c + WORD - PAGE, PAGE * 2);
    buffer.ForEachUploadRange(c + WORD - 1, 2, [](u64 offset, u64 size) {
        REQUIRE(offset == WORD - PAGE);
        REQUIRE(size == PAGE * 2);
    });
    buffer.MarkRegionAsCpuModified(c + WORD - PAGE, PAGE * 2);
    buffer.ForEachUploadRange(c + WORD - 1, 1, [](u64 offset, u64 size) {
        REQUIRE(offset == WORD - PAGE);
        REQUIRE(size == PAGE);
    });
    buffer.ForEachUploadRange(c + WORD + 50, 1, [](u64 offset, u64 size) {
        REQUIRE(offset == WORD);
        REQUIRE(size == PAGE);
    });
}

TEST_CASE("BufferBase: Partial word uploads", "[video_core]") {
    RasterizerInterface rasterizer;
    BufferBase buffer(rasterizer, c, 0x9d000);
    int num = 0;
    buffer.ForEachUploadRange(c, WORD, [&](u64 offset, u64 size) {
        REQUIRE(offset == 0U);
        REQUIRE(size == WORD);
        ++num;
    });
    REQUIRE(num == 1);
    buffer.ForEachUploadRange(c + WORD, WORD, [&](u64 offset, u64 size) {
        REQUIRE(offset == WORD);
        REQUIRE(size == WORD);
        ++num;
    });
    REQUIRE(num == 2);
    buffer.ForEachUploadRange(c + 0x79000, 0x24000, [&](u64 offset, u64 size) {
        REQUIRE(offset == WORD * 2);
        REQUIRE(size == PAGE * 0x1d);
        ++num;
    });
    REQUIRE(num == 3);
}

TEST_CASE("BufferBase: Partial page upload", "[video_core]") {
    RasterizerInterface rasterizer;
    BufferBase buffer(rasterizer, c, WORD);
    buffer.UnmarkRegionAsCpuModified(c, WORD);
    int num = 0;
    buffer.MarkRegionAsCpuModified(c + PAGE * 2, PAGE);
    buffer.MarkRegionAsCpuModified(c + PAGE * 9, PAGE);
    buffer.ForEachUploadRange(c, PAGE * 3, [&](u64 offset, u64 size) {
        REQUIRE(offset == PAGE * 2);
        REQUIRE(size == PAGE);
        ++num;
    });
    REQUIRE(num == 1);
    buffer.ForEachUploadRange(c + PAGE * 7, PAGE * 3, [&](u64 offset, u64 size) {
        REQUIRE(offset == PAGE * 9);
        REQUIRE(size == PAGE);
        ++num;
    });
    REQUIRE(num == 2);
}

TEST_CASE("BufferBase: Partial page upload with multiple words on the right") {
    RasterizerInterface rasterizer;
    BufferBase buffer(rasterizer, c, WORD * 8);
    buffer.UnmarkRegionAsCpuModified(c, WORD * 8);
    buffer.MarkRegionAsCpuModified(c + PAGE * 13, WORD * 7);
    int num = 0;
    buffer.ForEachUploadRange(c + PAGE * 10, WORD * 7, [&](u64 offset, u64 size) {
        REQUIRE(offset == PAGE * 13);
        REQUIRE(size == WORD * 7 - PAGE * 3);
        ++num;
    });
    REQUIRE(num == 1);
    buffer.ForEachUploadRange(c + PAGE, WORD * 8, [&](u64 offset, u64 size) {
        REQUIRE(offset == WORD * 7 + PAGE * 10);
        REQUIRE(size == PAGE * 3);
        ++num;
    });
    REQUIRE(num == 2);
}

TEST_CASE("BufferBase: Partial page upload with multiple words on the left", "[video_core]") {
    RasterizerInterface rasterizer;
    BufferBase buffer(rasterizer, c, WORD * 8);
    buffer.UnmarkRegionAsCpuModified(c, WORD * 8);
    buffer.MarkRegionAsCpuModified(c + PAGE * 13, WORD * 7);
    int num = 0;
    buffer.ForEachUploadRange(c + PAGE * 16, WORD * 7, [&](u64 offset, u64 size) {
        REQUIRE(offset == PAGE * 16);
        REQUIRE(size == WORD * 7 - PAGE * 3);
        ++num;
    });
    REQUIRE(num == 1);
    buffer.ForEachUploadRange(c + PAGE, WORD, [&](u64 offset, u64 size) {
        REQUIRE(offset == PAGE * 13);
        REQUIRE(size == PAGE * 3);
        ++num;
    });
    REQUIRE(num == 2);
}

TEST_CASE("BufferBase: Partial page upload with multiple words in the middle", "[video_core]") {
    RasterizerInterface rasterizer;
    BufferBase buffer(rasterizer, c, WORD * 8);
    buffer.UnmarkRegionAsCpuModified(c, WORD * 8);
    buffer.MarkRegionAsCpuModified(c + PAGE * 13, PAGE * 140);
    int num = 0;
    buffer.ForEachUploadRange(c + PAGE * 16, WORD, [&](u64 offset, u64 size) {
        REQUIRE(offset == PAGE * 16);
        REQUIRE(size == WORD);
        ++num;
    });
    REQUIRE(num == 1);
    buffer.ForEachUploadRange(c, WORD, [&](u64 offset, u64 size) {
        REQUIRE(offset == PAGE * 13);
        REQUIRE(size == PAGE * 3);
        ++num;
    });
    REQUIRE(num == 2);
    buffer.ForEachUploadRange(c, WORD * 8, [&](u64 offset, u64 size) {
        REQUIRE(offset == WORD + PAGE * 16);
        REQUIRE(size == PAGE * 73);
        ++num;
    });
    REQUIRE(num == 3);
}

TEST_CASE("BufferBase: Empty right bits", "[video_core]") {
    RasterizerInterface rasterizer;
    BufferBase buffer(rasterizer, c, WORD * 2048);
    buffer.UnmarkRegionAsCpuModified(c, WORD * 2048);
    buffer.MarkRegionAsCpuModified(c + WORD - PAGE, PAGE * 2);
    buffer.ForEachUploadRange(c, WORD * 2048, [](u64 offset, u64 size) {
        REQUIRE(offset == WORD - PAGE);
        REQUIRE(size == PAGE * 2);
    });
}

TEST_CASE("BufferBase: Out of bound ranges 1", "[video_core]") {
    RasterizerInterface rasterizer;
    BufferBase buffer(rasterizer, c, WORD);
    buffer.UnmarkRegionAsCpuModified(c, WORD);
    buffer.MarkRegionAsCpuModified(c, PAGE);
    int num = 0;
    buffer.ForEachUploadRange(c - WORD, WORD, [&](u64 offset, u64 size) { ++num; });
    buffer.ForEachUploadRange(c + WORD, WORD, [&](u64 offset, u64 size) { ++num; });
    buffer.ForEachUploadRange(c - PAGE, PAGE, [&](u64 offset, u64 size) { ++num; });
    REQUIRE(num == 0);
    buffer.ForEachUploadRange(c - PAGE, PAGE * 2, [&](u64 offset, u64 size) { ++num; });
    REQUIRE(num == 1);
    buffer.MarkRegionAsCpuModified(c, WORD);
    REQUIRE(rasterizer.Count() == 0);
}

TEST_CASE("BufferBase: Out of bound ranges 2", "[video_core]") {
    RasterizerInterface rasterizer;
    BufferBase buffer(rasterizer, c, 0x22000);
    REQUIRE_NOTHROW(buffer.UnmarkRegionAsCpuModified(c + 0x22000, PAGE));
    REQUIRE_NOTHROW(buffer.UnmarkRegionAsCpuModified(c + 0x28000, PAGE));
    REQUIRE(rasterizer.Count() == 0);
    REQUIRE_NOTHROW(buffer.UnmarkRegionAsCpuModified(c + 0x21100, PAGE - 0x100));
    REQUIRE(rasterizer.Count() == 1);
    REQUIRE_NOTHROW(buffer.UnmarkRegionAsCpuModified(c - 0x1000, PAGE * 2));
    buffer.UnmarkRegionAsCpuModified(c - 0x3000, PAGE * 2);
    buffer.UnmarkRegionAsCpuModified(c - 0x2000, PAGE * 2);
    REQUIRE(rasterizer.Count() == 2);
}

TEST_CASE("BufferBase: Out of bound ranges 3", "[video_core]") {
    RasterizerInterface rasterizer;
    BufferBase buffer(rasterizer, c, 0x310720);
    buffer.UnmarkRegionAsCpuModified(c, 0x310720);
    REQUIRE(rasterizer.Count(c) == 1);
    REQUIRE(rasterizer.Count(c + PAGE) == 1);
    REQUIRE(rasterizer.Count(c + WORD) == 1);
    REQUIRE(rasterizer.Count(c + WORD + PAGE) == 1);
}

TEST_CASE("BufferBase: Sparse regions 1", "[video_core]") {
    RasterizerInterface rasterizer;
    BufferBase buffer(rasterizer, c, WORD);
    buffer.UnmarkRegionAsCpuModified(c, WORD);
    buffer.MarkRegionAsCpuModified(c + PAGE * 1, PAGE);
    buffer.MarkRegionAsCpuModified(c + PAGE * 3, PAGE * 4);
    buffer.ForEachUploadRange(c, WORD, [i = 0](u64 offset, u64 size) mutable {
        static constexpr std::array<u64, 2> offsets{PAGE, PAGE * 3};
        static constexpr std::array<u64, 2> sizes{PAGE, PAGE * 4};
        REQUIRE(offset == offsets.at(i));
        REQUIRE(size == sizes.at(i));
        ++i;
    });
}

TEST_CASE("BufferBase: Sparse regions 2", "[video_core]") {
    RasterizerInterface rasterizer;
    BufferBase buffer(rasterizer, c, 0x22000);
    buffer.UnmarkRegionAsCpuModified(c, 0x22000);
    REQUIRE(rasterizer.Count() == 0x22);
    buffer.MarkRegionAsCpuModified(c + PAGE * 0x1B, PAGE);
    buffer.MarkRegionAsCpuModified(c + PAGE * 0x21, PAGE);
    buffer.ForEachUploadRange(c, WORD, [i = 0](u64 offset, u64 size) mutable {
        static constexpr std::array<u64, 2> offsets{PAGE * 0x1B, PAGE * 0x21};
        static constexpr std::array<u64, 2> sizes{PAGE, PAGE};
        REQUIRE(offset == offsets.at(i));
        REQUIRE(size == sizes.at(i));
        ++i;
    });
}

TEST_CASE("BufferBase: Single page modified range", "[video_core]") {
    RasterizerInterface rasterizer;
    BufferBase buffer(rasterizer, c, PAGE);
    REQUIRE(buffer.IsRegionCpuModified(c, PAGE));
    buffer.UnmarkRegionAsCpuModified(c, PAGE);
    REQUIRE(!buffer.IsRegionCpuModified(c, PAGE));
}

TEST_CASE("BufferBase: Two page modified range", "[video_core]") {
    RasterizerInterface rasterizer;
    BufferBase buffer(rasterizer, c, PAGE * 2);
    REQUIRE(buffer.IsRegionCpuModified(c, PAGE));
    REQUIRE(buffer.IsRegionCpuModified(c + PAGE, PAGE));
    REQUIRE(buffer.IsRegionCpuModified(c, PAGE * 2));
    buffer.UnmarkRegionAsCpuModified(c, PAGE);
    REQUIRE(!buffer.IsRegionCpuModified(c, PAGE));
}

TEST_CASE("BufferBase: Multi word modified ranges", "[video_core]") {
    for (int offset = 0; offset < 4; ++offset) {
        const VAddr address = c + WORD * offset;
        RasterizerInterface rasterizer;
        BufferBase buffer(rasterizer, address, WORD * 4);
        REQUIRE(buffer.IsRegionCpuModified(address, PAGE));
        REQUIRE(buffer.IsRegionCpuModified(address + PAGE * 48, PAGE));
        REQUIRE(buffer.IsRegionCpuModified(address + PAGE * 56, PAGE));

        buffer.UnmarkRegionAsCpuModified(address + PAGE * 32, PAGE);
        REQUIRE(buffer.IsRegionCpuModified(address + PAGE, WORD));
        REQUIRE(buffer.IsRegionCpuModified(address + PAGE * 31, PAGE));
        REQUIRE(!buffer.IsRegionCpuModified(address + PAGE * 32, PAGE));
        REQUIRE(buffer.IsRegionCpuModified(address + PAGE * 33, PAGE));
        REQUIRE(buffer.IsRegionCpuModified(address + PAGE * 31, PAGE * 2));
        REQUIRE(buffer.IsRegionCpuModified(address + PAGE * 32, PAGE * 2));

        buffer.UnmarkRegionAsCpuModified(address + PAGE * 33, PAGE);
        REQUIRE(!buffer.IsRegionCpuModified(address + PAGE * 32, PAGE * 2));
    }
}

TEST_CASE("BufferBase: Single page in large buffer", "[video_core]") {
    RasterizerInterface rasterizer;
    BufferBase buffer(rasterizer, c, WORD * 16);
    buffer.UnmarkRegionAsCpuModified(c, WORD * 16);
    REQUIRE(!buffer.IsRegionCpuModified(c, WORD * 16));

    buffer.MarkRegionAsCpuModified(c + WORD * 12 + PAGE * 8, PAGE);
    REQUIRE(buffer.IsRegionCpuModified(c, WORD * 16));
    REQUIRE(buffer.IsRegionCpuModified(c + WORD * 10, WORD * 2));
    REQUIRE(buffer.IsRegionCpuModified(c + WORD * 11, WORD * 2));
    REQUIRE(buffer.IsRegionCpuModified(c + WORD * 12, WORD * 2));
    REQUIRE(buffer.IsRegionCpuModified(c + WORD * 12 + PAGE * 4, PAGE * 8));
    REQUIRE(buffer.IsRegionCpuModified(c + WORD * 12 + PAGE * 6, PAGE * 8));
    REQUIRE(!buffer.IsRegionCpuModified(c + WORD * 12 + PAGE * 6, PAGE));
    REQUIRE(buffer.IsRegionCpuModified(c + WORD * 12 + PAGE * 7, PAGE * 2));
    REQUIRE(buffer.IsRegionCpuModified(c + WORD * 12 + PAGE * 8, PAGE * 2));
}

TEST_CASE("BufferBase: Out of bounds region query") {
    RasterizerInterface rasterizer;
    BufferBase buffer(rasterizer, c, WORD * 16);
    REQUIRE(!buffer.IsRegionCpuModified(c - PAGE, PAGE));
    REQUIRE(!buffer.IsRegionCpuModified(c - PAGE * 2, PAGE));
    REQUIRE(!buffer.IsRegionCpuModified(c + WORD * 16, PAGE));
    REQUIRE(buffer.IsRegionCpuModified(c + WORD * 16 - PAGE, WORD * 64));
    REQUIRE(!buffer.IsRegionCpuModified(c + WORD * 16, WORD * 64));
}

TEST_CASE("BufferBase: Wrap word regions") {
    RasterizerInterface rasterizer;
    BufferBase buffer(rasterizer, c, WORD * 2);
    buffer.UnmarkRegionAsCpuModified(c, WORD * 2);
    buffer.MarkRegionAsCpuModified(c + PAGE * 63, PAGE * 2);
    REQUIRE(buffer.IsRegionCpuModified(c, WORD * 2));
    REQUIRE(!buffer.IsRegionCpuModified(c + PAGE * 62, PAGE));
    REQUIRE(buffer.IsRegionCpuModified(c + PAGE * 63, PAGE));
    REQUIRE(buffer.IsRegionCpuModified(c + PAGE * 64, PAGE));
    REQUIRE(buffer.IsRegionCpuModified(c + PAGE * 63, PAGE * 2));
    REQUIRE(buffer.IsRegionCpuModified(c + PAGE * 63, PAGE * 8));
    REQUIRE(buffer.IsRegionCpuModified(c + PAGE * 60, PAGE * 8));

    REQUIRE(!buffer.IsRegionCpuModified(c + PAGE * 127, WORD * 16));
    buffer.MarkRegionAsCpuModified(c + PAGE * 127, PAGE);
    REQUIRE(buffer.IsRegionCpuModified(c + PAGE * 127, WORD * 16));
    REQUIRE(buffer.IsRegionCpuModified(c + PAGE * 127, PAGE));
    REQUIRE(!buffer.IsRegionCpuModified(c + PAGE * 126, PAGE));
    REQUIRE(buffer.IsRegionCpuModified(c + PAGE * 126, PAGE * 2));
    REQUIRE(!buffer.IsRegionCpuModified(c + PAGE * 128, WORD * 16));
}

TEST_CASE("BufferBase: Unaligned page region query") {
    RasterizerInterface rasterizer;
    BufferBase buffer(rasterizer, c, WORD);
    buffer.UnmarkRegionAsCpuModified(c, WORD);
    buffer.MarkRegionAsCpuModified(c + 4000, 1000);
    REQUIRE(buffer.IsRegionCpuModified(c, PAGE));
    REQUIRE(buffer.IsRegionCpuModified(c + PAGE, PAGE));
    REQUIRE(buffer.IsRegionCpuModified(c + 4000, 1000));
    REQUIRE(buffer.IsRegionCpuModified(c + 4000, 1));
}

TEST_CASE("BufferBase: Cached write") {
    RasterizerInterface rasterizer;
    BufferBase buffer(rasterizer, c, WORD);
    buffer.UnmarkRegionAsCpuModified(c, WORD);
    buffer.CachedCpuWrite(c + PAGE, PAGE);
    REQUIRE(!buffer.IsRegionCpuModified(c + PAGE, PAGE));
    buffer.FlushCachedWrites();
    REQUIRE(buffer.IsRegionCpuModified(c + PAGE, PAGE));
    buffer.MarkRegionAsCpuModified(c, WORD);
    REQUIRE(rasterizer.Count() == 0);
}

TEST_CASE("BufferBase: Multiple cached write") {
    RasterizerInterface rasterizer;
    BufferBase buffer(rasterizer, c, WORD);
    buffer.UnmarkRegionAsCpuModified(c, WORD);
    buffer.CachedCpuWrite(c + PAGE, PAGE);
    buffer.CachedCpuWrite(c + PAGE * 3, PAGE);
    REQUIRE(!buffer.IsRegionCpuModified(c + PAGE, PAGE));
    REQUIRE(!buffer.IsRegionCpuModified(c + PAGE * 3, PAGE));
    buffer.FlushCachedWrites();
    REQUIRE(buffer.IsRegionCpuModified(c + PAGE, PAGE));
    REQUIRE(buffer.IsRegionCpuModified(c + PAGE * 3, PAGE));
    buffer.MarkRegionAsCpuModified(c, WORD);
    REQUIRE(rasterizer.Count() == 0);
}

TEST_CASE("BufferBase: Cached write unmarked") {
    RasterizerInterface rasterizer;
    BufferBase buffer(rasterizer, c, WORD);
    buffer.UnmarkRegionAsCpuModified(c, WORD);
    buffer.CachedCpuWrite(c + PAGE, PAGE);
    buffer.UnmarkRegionAsCpuModified(c + PAGE, PAGE);
    REQUIRE(!buffer.IsRegionCpuModified(c + PAGE, PAGE));
    buffer.FlushCachedWrites();
    REQUIRE(buffer.IsRegionCpuModified(c + PAGE, PAGE));
    buffer.MarkRegionAsCpuModified(c, WORD);
    REQUIRE(rasterizer.Count() == 0);
}

TEST_CASE("BufferBase: Cached write iterated") {
    RasterizerInterface rasterizer;
    BufferBase buffer(rasterizer, c, WORD);
    buffer.UnmarkRegionAsCpuModified(c, WORD);
    buffer.CachedCpuWrite(c + PAGE, PAGE);
    int num = 0;
    buffer.ForEachUploadRange(c, WORD, [&](u64 offset, u64 size) { ++num; });
    REQUIRE(num == 0);
    REQUIRE(!buffer.IsRegionCpuModified(c + PAGE, PAGE));
    buffer.FlushCachedWrites();
    REQUIRE(buffer.IsRegionCpuModified(c + PAGE, PAGE));
    buffer.MarkRegionAsCpuModified(c, WORD);
    REQUIRE(rasterizer.Count() == 0);
}

TEST_CASE("BufferBase: Cached write downloads") {
    RasterizerInterface rasterizer;
    BufferBase buffer(rasterizer, c, WORD);
    buffer.UnmarkRegionAsCpuModified(c, WORD);
    REQUIRE(rasterizer.Count() == 64);
    buffer.CachedCpuWrite(c + PAGE, PAGE);
    REQUIRE(rasterizer.Count() == 63);
    buffer.MarkRegionAsGpuModified(c + PAGE, PAGE);
    int num = 0;
    buffer.ForEachDownloadRangeAndClear(c, WORD, [&](u64 offset, u64 size) { ++num; });
    buffer.ForEachUploadRange(c, WORD, [&](u64 offset, u64 size) { ++num; });
    REQUIRE(num == 0);
    REQUIRE(!buffer.IsRegionCpuModified(c + PAGE, PAGE));
    REQUIRE(!buffer.IsRegionGpuModified(c + PAGE, PAGE));
    buffer.FlushCachedWrites();
    REQUIRE(buffer.IsRegionCpuModified(c + PAGE, PAGE));
    REQUIRE(!buffer.IsRegionGpuModified(c + PAGE, PAGE));
    buffer.MarkRegionAsCpuModified(c, WORD);
    REQUIRE(rasterizer.Count() == 0);
}
