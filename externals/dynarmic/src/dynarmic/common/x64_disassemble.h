/* This file is part of the dynarmic project.
 * Copyright (c) 2021 MerryMage
 * SPDX-License-Identifier: 0BSD
 */

#pragma once

#include "dynarmic/common/common_types.h"

namespace Dynarmic::Common {

void DumpDisassembledX64(const void* ptr, size_t size);

}  // namespace Dynarmic::Common
