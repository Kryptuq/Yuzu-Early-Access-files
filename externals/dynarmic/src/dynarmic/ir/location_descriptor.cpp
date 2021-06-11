/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/ir/location_descriptor.h"

#include <ostream>

#include <fmt/format.h>

namespace Dynarmic::IR {

std::ostream& operator<<(std::ostream& o, const LocationDescriptor& descriptor) {
    o << fmt::format("{{{:016x}}}", descriptor.Value());
    return o;
}

}  // namespace Dynarmic::IR
