/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A64/a64_location_descriptor.h"

#include <ostream>

#include <fmt/format.h>

namespace Dynarmic::A64 {

std::ostream& operator<<(std::ostream& o, const LocationDescriptor& descriptor) {
    o << fmt::format("{{{}, {}{}}}", descriptor.PC(), descriptor.FPCR().Value(), descriptor.SingleStepping() ? ", step" : "");
    return o;
}

}  // namespace Dynarmic::A64
