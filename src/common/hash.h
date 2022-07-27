// SPDX-FileCopyrightText: 2015 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstddef>
#include <utility>
#include <boost/functional/hash.hpp>

namespace Common {

struct PairHash {
    template <class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2>& pair) const noexcept {
        std::size_t seed = std::hash<T1>()(pair.first);
        boost::hash_combine(seed, std::hash<T2>()(pair.second));
        return seed;
    }
};

template <typename T>
struct IdentityHash {
    [[nodiscard]] size_t operator()(T value) const noexcept {
        return static_cast<size_t>(value);
    }
};

} // namespace Common
