// SPDX-FileCopyrightText: 2013 Dolphin Emulator Project
// SPDX-FileCopyrightText: 2014 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/loader/loader.h"

namespace Core {
class System;
}

namespace Loader {

/// Loads an ELF/AXF file
class AppLoader_ELF final : public AppLoader {
public:
    explicit AppLoader_ELF(FileSys::VirtualFile file);

    /**
     * Identifies whether or not the given file is an ELF file.
     *
     * @param elf_file The file to identify.
     *
     * @return FileType::ELF, or FileType::Error if the file is not an ELF file.
     */
    static FileType IdentifyType(const FileSys::VirtualFile& elf_file);

    FileType GetFileType() const override {
        return IdentifyType(file);
    }

    LoadResult Load(Kernel::KProcess& process, Core::System& system) override;
};

} // namespace Loader
