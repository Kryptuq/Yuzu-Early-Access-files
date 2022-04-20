/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include <functional>
#include <iosfwd>

#include <mcl/stdint.hpp>

namespace Dynarmic::IR {

class LocationDescriptor {
public:
    explicit LocationDescriptor(u64 value)
            : value(value) {}

    bool operator==(const LocationDescriptor& o) const {
        return value == o.Value();
    }

    bool operator!=(const LocationDescriptor& o) const {
        return !operator==(o);
    }

    u64 Value() const { return value; }

private:
    u64 value;
};

std::ostream& operator<<(std::ostream& o, const LocationDescriptor& descriptor);

inline bool operator<(const LocationDescriptor& x, const LocationDescriptor& y) noexcept {
    return x.Value() < y.Value();
}

}  // namespace Dynarmic::IR

namespace std {
template<>
struct less<Dynarmic::IR::LocationDescriptor> {
    bool operator()(const Dynarmic::IR::LocationDescriptor& x, const Dynarmic::IR::LocationDescriptor& y) const noexcept {
        return x < y;
    }
};
template<>
struct hash<Dynarmic::IR::LocationDescriptor> {
    size_t operator()(const Dynarmic::IR::LocationDescriptor& x) const noexcept {
        return std::hash<u64>()(x.Value());
    }
};
}  // namespace std
