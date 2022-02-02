// Copyright 2020 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <array>
#include <chrono>
#include <limits>
#include <mutex>
#include <thread>

#include "common/atomic_ops.h"
#include "common/uint128.h"
#include "common/x64/native_clock.h"

namespace Common {

u64 EstimateRDTSCFrequency() {
    // Discard the first result measuring the rdtsc.
    _mm_mfence();
    __rdtsc();
    std::this_thread::sleep_for(std::chrono::milliseconds{1});
    _mm_mfence();
    __rdtsc();

    // Get the current time.
    const auto start_time = std::chrono::steady_clock::now();
    _mm_mfence();
    const u64 tsc_start = __rdtsc();
    // Wait for 200 milliseconds.
    std::this_thread::sleep_for(std::chrono::milliseconds{200});
    const auto end_time = std::chrono::steady_clock::now();
    _mm_mfence();
    const u64 tsc_end = __rdtsc();
    // Calculate differences.
    const u64 timer_diff = static_cast<u64>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count());
    const u64 tsc_diff = tsc_end - tsc_start;
    const u64 tsc_freq = MultiplyAndDivide64(tsc_diff, 1000000000ULL, timer_diff);
    return tsc_freq;
}

namespace X64 {
NativeClock::NativeClock(u64 emulated_cpu_frequency_, u64 emulated_clock_frequency_,
                         u64 rtsc_frequency_)
    : WallClock(emulated_cpu_frequency_, emulated_clock_frequency_, true), rtsc_frequency{
                                                                               rtsc_frequency_} {
    _mm_mfence();
    time_point.inner.last_measure = __rdtsc();
    time_point.inner.accumulated_ticks = 0U;
    ns_rtsc_factor = GetFixedPoint64Factor(NS_RATIO, rtsc_frequency);
    us_rtsc_factor = GetFixedPoint64Factor(US_RATIO, rtsc_frequency);
    ms_rtsc_factor = GetFixedPoint64Factor(MS_RATIO, rtsc_frequency);
    clock_rtsc_factor = GetFixedPoint64Factor(emulated_clock_frequency, rtsc_frequency);
    cpu_rtsc_factor = GetFixedPoint64Factor(emulated_cpu_frequency, rtsc_frequency);
}

u64 NativeClock::GetRTSC() {
    TimePoint new_time_point{};
    TimePoint current_time_point{};
    do {
        current_time_point.pack = time_point.pack;
        _mm_mfence();
        const u64 current_measure = __rdtsc();
        u64 diff = current_measure - current_time_point.inner.last_measure;
        diff = diff & ~static_cast<u64>(static_cast<s64>(diff) >> 63); // max(diff, 0)
        new_time_point.inner.last_measure = current_measure > current_time_point.inner.last_measure
                                                ? current_measure
                                                : current_time_point.inner.last_measure;
        new_time_point.inner.accumulated_ticks = current_time_point.inner.accumulated_ticks + diff;
    } while (!Common::AtomicCompareAndSwap(time_point.pack.data(), new_time_point.pack,
                                           current_time_point.pack));
    /// The clock cannot be more precise than the guest timer, remove the lower bits
    return new_time_point.inner.accumulated_ticks & inaccuracy_mask;
}

void NativeClock::Pause(bool is_paused) {
    if (!is_paused) {
        TimePoint current_time_point{};
        TimePoint new_time_point{};
        do {
            current_time_point.pack = time_point.pack;
            new_time_point.pack = current_time_point.pack;
            _mm_mfence();
            new_time_point.inner.last_measure = __rdtsc();
        } while (!Common::AtomicCompareAndSwap(time_point.pack.data(), new_time_point.pack,
                                               current_time_point.pack));
    }
}

std::chrono::nanoseconds NativeClock::GetTimeNS() {
    const u64 rtsc_value = GetRTSC();
    return std::chrono::nanoseconds{MultiplyHigh(rtsc_value, ns_rtsc_factor)};
}

std::chrono::microseconds NativeClock::GetTimeUS() {
    const u64 rtsc_value = GetRTSC();
    return std::chrono::microseconds{MultiplyHigh(rtsc_value, us_rtsc_factor)};
}

std::chrono::milliseconds NativeClock::GetTimeMS() {
    const u64 rtsc_value = GetRTSC();
    return std::chrono::milliseconds{MultiplyHigh(rtsc_value, ms_rtsc_factor)};
}

u64 NativeClock::GetClockCycles() {
    const u64 rtsc_value = GetRTSC();
    return MultiplyHigh(rtsc_value, clock_rtsc_factor);
}

u64 NativeClock::GetCPUCycles() {
    const u64 rtsc_value = GetRTSC();
    return MultiplyHigh(rtsc_value, cpu_rtsc_factor);
}

} // namespace X64

} // namespace Common
