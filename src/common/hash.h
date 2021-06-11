// Copyright 2015 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

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

} // namespace Common
