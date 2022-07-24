// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "video_core/vulkan_common/vulkan_wrapper.h"

#include <filesystem>
#include <fstream>
#include "common/fs/fs.h"
#include "common/fs/path_util.h"
#include "common/logging/log.h"
#include "video_core/vulkan_common/vulkan_instance.h"
#include "video_core/vulkan_common/vulkan_library.h"
#include "yuzu/check_vulkan.h"
#include "yuzu/uisettings.h"

constexpr char TEMP_FILE_NAME[] = "vulkan_check";

bool CheckVulkan() {
    if (UISettings::values.has_broken_vulkan) {
        return true;
    }

    LOG_DEBUG(Frontend, "Checking presence of Vulkan");

    const auto fs_config_loc = Common::FS::GetYuzuPath(Common::FS::YuzuPath::ConfigDir);
    const auto temp_file_loc = fs_config_loc / TEMP_FILE_NAME;

    if (std::filesystem::exists(temp_file_loc)) {
        LOG_WARNING(Frontend, "Detected recovery from previous failed Vulkan initialization");

        UISettings::values.has_broken_vulkan = true;
        std::filesystem::remove(temp_file_loc);
        return false;
    }

    std::ofstream temp_file_handle(temp_file_loc);
    temp_file_handle.close();

    try {
        Vulkan::vk::InstanceDispatch dld;
        const Common::DynamicLibrary library = Vulkan::OpenLibrary();
        const Vulkan::vk::Instance instance =
            Vulkan::CreateInstance(library, dld, VK_API_VERSION_1_0);

    } catch (const Vulkan::vk::Exception& exception) {
        LOG_ERROR(Frontend, "Failed to initialize Vulkan: {}", exception.what());
        // Don't set has_broken_vulkan to true here: we care when loading Vulkan crashes the
        // application, not when we can handle it.
    }

    std::filesystem::remove(temp_file_loc);
    return true;
}
