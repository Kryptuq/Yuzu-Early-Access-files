// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstdint>
#include <cstring>

#include "common/cache_management.h"

namespace Common {

#if defined(ARCHITECTURE_x86_64)

// Most cache operations are no-ops on x86

void DataCacheLineCleanByVAToPoU(void* start, size_t size) {}
void DataCacheLineCleanAndInvalidateByVAToPoC(void* start, size_t size) {}
void DataCacheLineCleanByVAToPoC(void* start, size_t size) {}
void DataCacheZeroByVA(void* start, size_t size) {
    std::memset(start, 0, size);
}

#elif defined(ARCHITECTURE_arm64)

// BS/DminLine is log2(cache size in words), we want size in bytes
#define EXTRACT_DMINLINE(ctr_el0) (1 << ((((ctr_el0) >> 16) & 0xf) + 2))
#define EXTRACT_BS(dczid_el0) (1 << (((dczid_el0)&0xf) + 2))

#define DEFINE_DC_OP(op_name, function_name)                                                       \
    void function_name(void* start, size_t size) {                                                 \
        size_t ctr_el0;                                                                            \
        asm volatile("mrs %[ctr_el0], ctr_el0\n\t" : [ctr_el0] "=r"(ctr_el0));                     \
        size_t cacheline_size = EXTRACT_DMINLINE(ctr_el0);                                         \
        uintptr_t va_start = reinterpret_cast<uintptr_t>(start);                                   \
        uintptr_t va_end = va_start + size;                                                        \
        for (uintptr_t va = va_start; va < va_end; va += cacheline_size) {                         \
            asm volatile("dc " #op_name ", %[va]\n\t" : : [va] "r"(va) : "memory");                \
        }                                                                                          \
    }

#define DEFINE_DC_OP_DCZID(op_name, function_name)                                                 \
    void function_name(void* start, size_t size) {                                                 \
        size_t dczid_el0;                                                                          \
        asm volatile("mrs %[dczid_el0], dczid_el0\n\t" : [dczid_el0] "=r"(dczid_el0));             \
        size_t cacheline_size = EXTRACT_BS(dczid_el0);                                             \
        uintptr_t va_start = reinterpret_cast<uintptr_t>(start);                                   \
        uintptr_t va_end = va_start + size;                                                        \
        for (uintptr_t va = va_start; va < va_end; va += cacheline_size) {                         \
            asm volatile("dc " #op_name ", %[va]\n\t" : : [va] "r"(va) : "memory");                \
        }                                                                                          \
    }

DEFINE_DC_OP(cvau, DataCacheLineCleanByVAToPoU);
DEFINE_DC_OP(civac, DataCacheLineCleanAndInvalidateByVAToPoC);
DEFINE_DC_OP(cvac, DataCacheLineCleanByVAToPoC);
DEFINE_DC_OP_DCZID(zva, DataCacheZeroByVA);

#endif

} // namespace Common
