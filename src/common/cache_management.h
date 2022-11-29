// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstddef>

namespace Common {

// Data cache instructions enabled at EL0 by SCTLR_EL1.UCI.
// VA = virtual address
// PoC = point of coherency
// PoU = point of unification

// dc cvau
void DataCacheLineCleanByVAToPoU(void* start, size_t size);

// dc civac
void DataCacheLineCleanAndInvalidateByVAToPoC(void* start, size_t size);

// dc cvac
void DataCacheLineCleanByVAToPoC(void* start, size_t size);

// dc zva
void DataCacheZeroByVA(void* start, size_t size);

} // namespace Common
