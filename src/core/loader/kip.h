// Copyright 2019 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

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
