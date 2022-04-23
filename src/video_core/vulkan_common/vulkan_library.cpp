// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <string>

#include "common/dynamic_library.h"
#include "common/fs/path_util.h"
#include "video_core/vulkan_common/vulkan_library.h"

namespace Vulkan {

Common::DynamicLibrary OpenLibrary() {
    Common::DynamicLibrary library;
#ifdef __APPLE__
    // Check if a path to a specific Vulkan library has been specified.
    char* const libvulkan_env = std::getenv("LIBVULKAN_PATH");
    if (!libvulkan_env || !library.Open(libvulkan_env)) {
        // Use the libvulkan.dylib from the application bundle.
        const auto filename =
            Common::FS::GetBundleDirectory() / "Contents/Frameworks/libvulkan.dylib";
        void(library.Open(Common::FS::PathToUTF8String(filename).c_str()));
    }
#else
    std::string filename = Common::DynamicLibrary::GetVersionedFilename("vulkan", 1);
    if (!library.Open(filename.c_str())) {
        // Android devices may not have libvulkan.so.1, only libvulkan.so.
        filename = Common::DynamicLibrary::GetVersionedFilename("vulkan");
        void(library.Open(filename.c_str()));
    }
#endif
    return library;
}

} // namespace Vulkan
