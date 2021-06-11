// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <vector>
#include "common/common_types.h"

namespace AudioCore {

/// Digital biquad filter:
///
///          b0 + b1 z^-1 + b2 z^-2
///  H(z) = ------------------------
///          a0 + a1 z^-1 + b2 z^-2
class Filter {
public:
    /// Creates a low-pass filter.
    /// @param cutoff Determines the cutoff frequency. A value from 0.0 to 1.0.
    /// @param Q Determines the quality factor of this filter.
    static Filter LowPass(double cutoff, double Q = 0.7071);

    /// Passthrough filter.
    Filter();

    Filter(double a0_, double a1_, double a2_, double b0_, double b1_, double b2_);

    void Process(std::vector<s16>& signal);

private:
    static constexpr std::size_t channel_count = 2;

    /// Coefficients are in normalized form (a0 = 1.0).
    double a1, a2, b0, b1, b2;
    /// Input History
    std::array<std::array<double, channel_count>, 3> in;
    /// Output History
    std::array<std::array<double, channel_count>, 3> out;
};

/// Cascade filters to build up higher-order filters from lower-order ones.
class CascadingFilter {
public:
    /// Creates a cascading low-pass filter.
    /// @param cutoff Determines the cutoff frequency. A value from 0.0 to 1.0.
    /// @param cascade_size Number of biquads in cascade.
    static CascadingFilter LowPass(double cutoff, std::size_t cascade_size);

    /// Passthrough.
    CascadingFilter();

    explicit CascadingFilter(std::vector<Filter> filters_);

    void Process(std::vector<s16>& signal);

private:
    std::vector<Filter> filters;
};

} // namespace AudioCore
