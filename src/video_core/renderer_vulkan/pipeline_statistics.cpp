// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <string_view>

#include <fmt/format.h>

#include "common/common_types.h"
#include "common/logging/log.h"
#include "video_core/renderer_vulkan/pipeline_statistics.h"
#include "video_core/vulkan_common/vulkan_device.h"
#include "video_core/vulkan_common/vulkan_wrapper.h"

namespace Vulkan {

using namespace std::string_view_literals;

static u64 GetUint64(const VkPipelineExecutableStatisticKHR& statistic) {
    switch (statistic.format) {
    case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_INT64_KHR:
        return static_cast<u64>(statistic.value.i64);
    case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_UINT64_KHR:
        return statistic.value.u64;
    case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_FLOAT64_KHR:
        return static_cast<u64>(statistic.value.f64);
    default:
        return 0;
    }
}

PipelineStatistics::PipelineStatistics(const Device& device_) : device{device_} {}

void PipelineStatistics::Collect(VkPipeline pipeline) {
    const auto& dev{device.GetLogical()};
    const std::vector properties{dev.GetPipelineExecutablePropertiesKHR(pipeline)};
    const u32 num_executables{static_cast<u32>(properties.size())};
    for (u32 executable = 0; executable < num_executables; ++executable) {
        const auto statistics{dev.GetPipelineExecutableStatisticsKHR(pipeline, executable)};
        if (statistics.empty()) {
            continue;
        }
        Stats stage_stats;
        for (const auto& statistic : statistics) {
            const char* const name{statistic.name};
            if (name == "Binary Size"sv || name == "Code size"sv || name == "Instruction Count"sv) {
                stage_stats.code_size = GetUint64(statistic);
            } else if (name == "Register Count"sv) {
                stage_stats.register_count = GetUint64(statistic);
            } else if (name == "SGPRs"sv || name == "numUsedSgprs"sv) {
                stage_stats.sgpr_count = GetUint64(statistic);
            } else if (name == "VGPRs"sv || name == "numUsedVgprs"sv) {
                stage_stats.vgpr_count = GetUint64(statistic);
            } else if (name == "Branches"sv) {
                stage_stats.branches_count = GetUint64(statistic);
            } else if (name == "Basic Block Count"sv) {
                stage_stats.basic_block_count = GetUint64(statistic);
            }
        }
        std::scoped_lock lock{mutex};
        collected_stats.push_back(stage_stats);
    }
}

void PipelineStatistics::Report() const {
    double num{};
    Stats total;
    {
        std::scoped_lock lock{mutex};
        for (const Stats& stats : collected_stats) {
            total.code_size += stats.code_size;
            total.register_count += stats.register_count;
            total.sgpr_count += stats.sgpr_count;
            total.vgpr_count += stats.vgpr_count;
            total.branches_count += stats.branches_count;
            total.basic_block_count += stats.basic_block_count;
        }
        num = static_cast<double>(collected_stats.size());
    }
    std::string report;
    const auto add = [&](const char* fmt, u64 value) {
        if (value > 0) {
            report += fmt::format(fmt::runtime(fmt), static_cast<double>(value) / num);
        }
    };
    add("Code size:      {:9.03f}\n", total.code_size);
    add("Register count: {:9.03f}\n", total.register_count);
    add("SGPRs:          {:9.03f}\n", total.sgpr_count);
    add("VGPRs:          {:9.03f}\n", total.vgpr_count);
    add("Branches count: {:9.03f}\n", total.branches_count);
    add("Basic blocks:   {:9.03f}\n", total.basic_block_count);

    LOG_INFO(Render_Vulkan,
             "\nAverage pipeline statistics\n"
             "==========================================\n"
             "{}\n",
             report);
}

} // namespace Vulkan
