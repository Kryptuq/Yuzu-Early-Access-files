// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/loader/loader.h"

namespace Core {
class System;
}

namespace FileSys {
class KIP;
}

namespace Loader {

class AppLoader_KIP final : public AppLoader {
public:
    explicit AppLoader_KIP(FileSys::VirtualFile file);
    ~AppLoader_KIP() override;

    /**
     * Identifies whether or not the given file is a KIP.
     *
     * @param in_file The file to identify.
     *
     * @return FileType::KIP if found, or FileType::Error if unknown.
     */
    static FileType IdentifyType(const FileSys::VirtualFile& in_file);

    FileType GetFileType() const override;

    LoadResult Load(Kernel::KProcess& process, Core::System& system) override;

private:
    std::unique_ptr<FileSys::KIP> kip;
};

} // namespace Loader
