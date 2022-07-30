/* This file is part of the dynarmic project.
 * Copyright (c) 2016 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#include "dynarmic/frontend/A64/a64_location_descriptor.h"

#include <fmt/format.h>

namespace Dynarmic::A64 {

std::string ToString(const LocationDescriptor& descriptor) {
    return fmt::format("{{{}, {}{}}}", descriptor.PC(), descriptor.FPCR().Value(), descriptor.SingleStepping() ? ", step" : "");
}

}  // namespace Dynarmic::A64
