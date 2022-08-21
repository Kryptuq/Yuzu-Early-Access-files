// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <algorithm>
#include <array>
#include <cstring>
#include <iterator>
#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "common/assert.h"
#include "common/settings.h"
#include "video_core/control/channel_state_cache.h"
#include "video_core/engines/maxwell_3d.h"
#include "video_core/memory_manager.h"
#include "video_core/rasterizer_interface.h"

namespace VideoCommon {

template <class QueryCache, class HostCounter>
class CounterStreamBase {
public:
    explicit CounterStreamBase(QueryCache& cache_, VideoCore::QueryType type_)
        : cache{cache_}, type{type_} {}

    /// Updates the state of the stream, enabling or disabling as needed.
    void Update(bool enabled) {
        if (enabled) {
            Enable();
        } else {
            Disable();
        }
    }

    /// Resets the stream to zero. It doesn't disable the query after resetting.
    void Reset() {
        if (current) {
            current->EndQuery();

            // Immediately start a new query to avoid disabling its state.
            current = cache.Counter(nullptr, type);
        }
        last = nullptr;
    }

    /// Returns the current counter slicing as needed.
    std::shared_ptr<HostCounter> Current() {
        if (!current) {
            return nullptr;
        }
        current->EndQuery();
        last = std::move(current);
        current = cache.Counter(last, type);
        return last;
    }

    /// Returns true when the counter stream is enabled.
    bool IsEnabled() const {
        return current != nullptr;
    }

private:
    /// Enables the stream.
    void Enable() {
        if (current) {
            return;
        }
        current = cache.Counter(last, type);
    }

    // Disables the stream.
    void Disable() {
        if (current) {
            current->EndQuery();
        }
        last = std::exchange(current, nullptr);
    }

    QueryCache& cache;
    const VideoCore::QueryType type;

    std::shared_ptr<HostCounter> current;
    std::shared_ptr<HostCounter> last;
};

template <class QueryCache, class CachedQuery, class CounterStream, class HostCounter>
class QueryCacheBase : public VideoCommon::ChannelSetupCaches<VideoCommon::ChannelInfo> {
public:
    explicit QueryCacheBase(VideoCore::RasterizerInterface& rasterizer_)
        : rasterizer{rasterizer_}, streams{{CounterStream{static_cast<QueryCache&>(*this),
                                                          VideoCore::QueryType::SamplesPassed}}} {}

    void InvalidateRegion(VAddr addr, std::size_t size) {
        std::unique_lock lock{mutex};
        FlushAndRemoveRegion(addr, size);
    }

    void FlushRegion(VAddr addr, std::size_t size) {
        std::unique_lock lock{mutex};
        FlushAndRemoveRegion(addr, size);
    }

    /**
     * Records a query in GPU mapped memory, potentially marked with a timestamp.
     * @param gpu_addr  GPU address to flush to when the mapped memory is read.
     * @param type      Query type, e.g. SamplesPassed.
     * @param timestamp Timestamp, when empty the flushed query is assumed to be short.
     */
    void Query(GPUVAddr gpu_addr, VideoCore::QueryType type, std::optional<u64> timestamp) {
        std::unique_lock lock{mutex};
        const std::optional<VAddr> cpu_addr = gpu_memory->GpuToCpuAddress(gpu_addr);
        ASSERT(cpu_addr);

        CachedQuery* query = TryGet(*cpu_addr);
        if (!query) {
            ASSERT_OR_EXECUTE(cpu_addr, return;);
            u8* const host_ptr = gpu_memory->GetPointer(gpu_addr);

            query = Register(type, *cpu_addr, host_ptr, timestamp.has_value());
        }

        query->BindCounter(Stream(type).Current(), timestamp);
        if (Settings::values.use_asynchronous_gpu_emulation.GetValue()) {
            AsyncFlushQuery(*cpu_addr);
        }
    }

    /// Updates counters from GPU state. Expected to be called once per draw, clear or dispatch.
    void UpdateCounters() {
        std::unique_lock lock{mutex};
        if (maxwell3d) {
            const auto& regs = maxwell3d->regs;
            Stream(VideoCore::QueryType::SamplesPassed).Update(regs.samplecnt_enable);
        }
    }

    /// Resets a counter to zero. It doesn't disable the query after resetting.
    void ResetCounter(VideoCore::QueryType type) {
        std::unique_lock lock{mutex};
        Stream(type).Reset();
    }

    /// Disable all active streams. Expected to be called at the end of a command buffer.
    void DisableStreams() {
        std::unique_lock lock{mutex};
        for (auto& stream : streams) {
            stream.Update(false);
        }
    }

    /// Returns a new host counter.
    std::shared_ptr<HostCounter> Counter(std::shared_ptr<HostCounter> dependency,
                                         VideoCore::QueryType type) {
        return std::make_shared<HostCounter>(static_cast<QueryCache&>(*this), std::move(dependency),
                                             type);
    }

    /// Returns the counter stream of the specified type.
    CounterStream& Stream(VideoCore::QueryType type) {
        return streams[static_cast<std::size_t>(type)];
    }

    /// Returns the counter stream of the specified type.
    const CounterStream& Stream(VideoCore::QueryType type) const {
        return streams[static_cast<std::size_t>(type)];
    }

    void CommitAsyncFlushes() {
        committed_flushes.push_back(uncommitted_flushes);
        uncommitted_flushes.reset();
    }

    bool HasUncommittedFlushes() const {
        return uncommitted_flushes != nullptr;
    }

    bool ShouldWaitAsyncFlushes() const {
        if (committed_flushes.empty()) {
            return false;
        }
        return committed_flushes.front() != nullptr;
    }

    void PopAsyncFlushes() {
        if (committed_flushes.empty()) {
            return;
        }
        auto& flush_list = committed_flushes.front();
        if (!flush_list) {
            committed_flushes.pop_front();
            return;
        }
        for (VAddr query_address : *flush_list) {
            FlushAndRemoveRegion(query_address, 4);
        }
        committed_flushes.pop_front();
    }

private:
    /// Flushes a memory range to guest memory and removes it from the cache.
    void FlushAndRemoveRegion(VAddr addr, std::size_t size) {
        const u64 addr_begin = addr;
        const u64 addr_end = addr_begin + size;
        const auto in_range = [addr_begin, addr_end](const CachedQuery& query) {
            const u64 cache_begin = query.GetCpuAddr();
            const u64 cache_end = cache_begin + query.SizeInBytes();
            return cache_begin < addr_end && addr_begin < cache_end;
        };

        const u64 page_end = addr_end >> YUZU_PAGEBITS;
        for (u64 page = addr_begin >> YUZU_PAGEBITS; page <= page_end; ++page) {
            const auto& it = cached_queries.find(page);
            if (it == std::end(cached_queries)) {
                continue;
            }
            auto& contents = it->second;
            for (auto& query : contents) {
                if (!in_range(query)) {
                    continue;
                }
                rasterizer.UpdatePagesCachedCount(query.GetCpuAddr(), query.SizeInBytes(), -1);
                query.Flush();
            }
            std::erase_if(contents, in_range);
        }
    }

    /// Registers the passed parameters as cached and returns a pointer to the stored cached query.
    CachedQuery* Register(VideoCore::QueryType type, VAddr cpu_addr, u8* host_ptr, bool timestamp) {
        rasterizer.UpdatePagesCachedCount(cpu_addr, CachedQuery::SizeInBytes(timestamp), 1);
        const u64 page = static_cast<u64>(cpu_addr) >> YUZU_PAGEBITS;
        return &cached_queries[page].emplace_back(static_cast<QueryCache&>(*this), type, cpu_addr,
                                                  host_ptr);
    }

    /// Tries to a get a cached query. Returns nullptr on failure.
    CachedQuery* TryGet(VAddr addr) {
        const u64 page = static_cast<u64>(addr) >> YUZU_PAGEBITS;
        const auto it = cached_queries.find(page);
        if (it == std::end(cached_queries)) {
            return nullptr;
        }
        auto& contents = it->second;
        const auto found = std::find_if(std::begin(contents), std::end(contents),
                                        [addr](auto& query) { return query.GetCpuAddr() == addr; });
        return found != std::end(contents) ? &*found : nullptr;
    }

    void AsyncFlushQuery(VAddr addr) {
        if (!uncommitted_flushes) {
            uncommitted_flushes = std::make_shared<std::vector<VAddr>>();
        }
        uncommitted_flushes->push_back(addr);
    }

    static constexpr std::uintptr_t YUZU_PAGESIZE = 4096;
    static constexpr unsigned YUZU_PAGEBITS = 12;

    VideoCore::RasterizerInterface& rasterizer;

    std::recursive_mutex mutex;

    std::unordered_map<u64, std::vector<CachedQuery>> cached_queries;

    std::array<CounterStream, VideoCore::NumQueryTypes> streams;

    std::shared_ptr<std::vector<VAddr>> uncommitted_flushes{};
    std::list<std::shared_ptr<std::vector<VAddr>>> committed_flushes;
};

template <class QueryCache, class HostCounter>
class HostCounterBase {
public:
    explicit HostCounterBase(std::shared_ptr<HostCounter> dependency_)
        : dependency{std::move(dependency_)}, depth{dependency ? (dependency->Depth() + 1) : 0} {
        // Avoid nesting too many dependencies to avoid a stack overflow when these are deleted.
        constexpr u64 depth_threshold = 96;
        if (depth > depth_threshold) {
            depth = 0;
            base_result = dependency->Query();
            dependency = nullptr;
        }
    }
    virtual ~HostCounterBase() = default;

    /// Returns the current value of the query.
    u64 Query() {
        if (result) {
            return *result;
        }

        u64 value = BlockingQuery() + base_result;
        if (dependency) {
            value += dependency->Query();
            dependency = nullptr;
        }

        result = value;
        return *result;
    }

    /// Returns true when flushing this query will potentially wait.
    bool WaitPending() const noexcept {
        return result.has_value();
    }

    u64 Depth() const noexcept {
        return depth;
    }

protected:
    /// Returns the value of query from the backend API blocking as needed.
    virtual u64 BlockingQuery() const = 0;

private:
    std::shared_ptr<HostCounter> dependency; ///< Counter to add to this value.
    std::optional<u64> result;               ///< Filled with the already returned value.
    u64 depth;                               ///< Number of nested dependencies.
    u64 base_result = 0;                     ///< Equivalent to nested dependencies value.
};

template <class HostCounter>
class CachedQueryBase {
public:
    explicit CachedQueryBase(VAddr cpu_addr_, u8* host_ptr_)
        : cpu_addr{cpu_addr_}, host_ptr{host_ptr_} {}
    virtual ~CachedQueryBase() = default;

    CachedQueryBase(CachedQueryBase&&) noexcept = default;
    CachedQueryBase(const CachedQueryBase&) = delete;

    CachedQueryBase& operator=(CachedQueryBase&&) noexcept = default;
    CachedQueryBase& operator=(const CachedQueryBase&) = delete;

    /// Flushes the query to guest memory.
    virtual void Flush() {
        // When counter is nullptr it means that it's just been reseted. We are supposed to write a
        // zero in these cases.
        const u64 value = counter ? counter->Query() : 0;
        std::memcpy(host_ptr, &value, sizeof(u64));

        if (timestamp) {
            std::memcpy(host_ptr + TIMESTAMP_OFFSET, &*timestamp, sizeof(u64));
        }
    }

    /// Binds a counter to this query.
    void BindCounter(std::shared_ptr<HostCounter> counter_, std::optional<u64> timestamp_) {
        if (counter) {
            // If there's an old counter set it means the query is being rewritten by the game.
            // To avoid losing the data forever, flush here.
            Flush();
        }
        counter = std::move(counter_);
        timestamp = timestamp_;
    }

    VAddr GetCpuAddr() const noexcept {
        return cpu_addr;
    }

    u64 SizeInBytes() const noexcept {
        return SizeInBytes(timestamp.has_value());
    }

    static constexpr u64 SizeInBytes(bool with_timestamp) noexcept {
        return with_timestamp ? LARGE_QUERY_SIZE : SMALL_QUERY_SIZE;
    }

protected:
    /// Returns true when querying the counter may potentially block.
    bool WaitPending() const noexcept {
        return counter && counter->WaitPending();
    }

private:
    static constexpr std::size_t SMALL_QUERY_SIZE = 8;   // Query size without timestamp.
    static constexpr std::size_t LARGE_QUERY_SIZE = 16;  // Query size with timestamp.
    static constexpr std::intptr_t TIMESTAMP_OFFSET = 8; // Timestamp offset in a large query.

    VAddr cpu_addr;                       ///< Guest CPU address.
    u8* host_ptr;                         ///< Writable host pointer.
    std::shared_ptr<HostCounter> counter; ///< Host counter to query, owns the dependency tree.
    std::optional<u64> timestamp;         ///< Timestamp to flush to guest memory.
};

} // namespace VideoCommon
