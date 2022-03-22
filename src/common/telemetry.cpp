// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <cstring>
#include "common/scm_rev.h"
#include "common/telemetry.h"

#ifdef ARCHITECTURE_x86_64
#include "common/x64/cpu_detect.h"
#endif

namespace Common::Telemetry {

void FieldCollection::Accept(VisitorInterface& visitor) const {
    for (const auto& field : fields) {
        field.second->Accept(visitor);
    }
}

void FieldCollection::AddField(std::unique_ptr<FieldInterface> field) {
    fields[field->GetName()] = std::move(field);
}

template <class T>
void Field<T>::Accept(VisitorInterface& visitor) const {
    visitor.Visit(*this);
}

template class Field<bool>;
template class Field<double>;
template class Field<float>;
template class Field<u8>;
template class Field<u16>;
template class Field<u32>;
template class Field<u64>;
template class Field<s8>;
template class Field<s16>;
template class Field<s32>;
template class Field<s64>;
template class Field<std::string>;
template class Field<const char*>;
template class Field<std::chrono::microseconds>;

void AppendBuildInfo(FieldCollection& fc) {
    const bool is_git_dirty{std::strstr(Common::g_scm_desc, "dirty") != nullptr};
    fc.AddField(FieldType::App, "Git_IsDirty", is_git_dirty);
    fc.AddField(FieldType::App, "Git_Branch", Common::g_scm_branch);
    fc.AddField(FieldType::App, "Git_Revision", Common::g_scm_rev);
    fc.AddField(FieldType::App, "BuildDate", Common::g_build_date);
    fc.AddField(FieldType::App, "BuildName", Common::g_build_name);
}

void AppendCPUInfo(FieldCollection& fc) {
#ifdef ARCHITECTURE_x86_64

    const auto& caps = Common::GetCPUCaps();
    const auto add_field = [&fc](std::string_view field_name, const auto& field_value) {
        fc.AddField(FieldType::UserSystem, field_name, field_value);
    };
    add_field("CPU_Model", caps.cpu_string);
    add_field("CPU_BrandString", caps.brand_string);

    add_field("CPU_Extension_x64_SSE", caps.sse);
    add_field("CPU_Extension_x64_SSE2", caps.sse2);
    add_field("CPU_Extension_x64_SSE3", caps.sse3);
    add_field("CPU_Extension_x64_SSSE3", caps.ssse3);
    add_field("CPU_Extension_x64_SSE41", caps.sse4_1);
    add_field("CPU_Extension_x64_SSE42", caps.sse4_2);

    add_field("CPU_Extension_x64_AVX", caps.avx);
    add_field("CPU_Extension_x64_AVX_VNNI", caps.avx_vnni);
    add_field("CPU_Extension_x64_AVX2", caps.avx2);

    // Skylake-X/SP level AVX512, for compatibility with the previous telemetry field
    add_field("CPU_Extension_x64_AVX512",
              caps.avx512f && caps.avx512cd && caps.avx512vl && caps.avx512dq && caps.avx512bw);

    add_field("CPU_Extension_x64_AVX512F", caps.avx512f);
    add_field("CPU_Extension_x64_AVX512CD", caps.avx512cd);
    add_field("CPU_Extension_x64_AVX512VL", caps.avx512vl);
    add_field("CPU_Extension_x64_AVX512DQ", caps.avx512dq);
    add_field("CPU_Extension_x64_AVX512BW", caps.avx512bw);
    add_field("CPU_Extension_x64_AVX512BITALG", caps.avx512bitalg);
    add_field("CPU_Extension_x64_AVX512VBMI", caps.avx512vbmi);

    add_field("CPU_Extension_x64_AES", caps.aes);
    add_field("CPU_Extension_x64_BMI1", caps.bmi1);
    add_field("CPU_Extension_x64_BMI2", caps.bmi2);
    add_field("CPU_Extension_x64_F16C", caps.f16c);
    add_field("CPU_Extension_x64_FMA", caps.fma);
    add_field("CPU_Extension_x64_FMA4", caps.fma4);
    add_field("CPU_Extension_x64_GFNI", caps.gfni);
    add_field("CPU_Extension_x64_INVARIANT_TSC", caps.invariant_tsc);
    add_field("CPU_Extension_x64_LZCNT", caps.lzcnt);
    add_field("CPU_Extension_x64_MOVBE", caps.movbe);
    add_field("CPU_Extension_x64_PCLMULQDQ", caps.pclmulqdq);
    add_field("CPU_Extension_x64_POPCNT", caps.popcnt);
    add_field("CPU_Extension_x64_SHA", caps.sha);
#else
    fc.AddField(FieldType::UserSystem, "CPU_Model", "Other");
#endif
}

void AppendOSInfo(FieldCollection& fc) {
#ifdef __APPLE__
    fc.AddField(FieldType::UserSystem, "OsPlatform", "Apple");
#elif defined(_WIN32)
    fc.AddField(FieldType::UserSystem, "OsPlatform", "Windows");
#elif defined(__linux__) || defined(linux) || defined(__linux)
    fc.AddField(FieldType::UserSystem, "OsPlatform", "Linux");
#else
    fc.AddField(FieldType::UserSystem, "OsPlatform", "Unknown");
#endif
}

} // namespace Common::Telemetry
