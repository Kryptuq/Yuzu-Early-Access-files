// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/common_types.h"
#include "core/loader/loader.h"

namespace Core {
class System;
}

namespace FileSys {
class NCA;
}

namespace Loader {

class AppLoader_DeconstructedRomDirectory;

/// Loads an NCA file
class AppLoader_NCA final : public AppLoader {
public:
    explicit AppLoader_NCA(FileSys::VirtualFile file_);
    ~AppLoader_NCA() override;

    /**
     * Identifies whether or not the given file is an NCA file.
     *
     * @param nca_file The file to identify.
     *
     * @return FileType::NCA, or FileType::Error if the file is not an NCA file.
     */
    static FileType IdentifyType(const FileSys::VirtualFile& nca_file);

    FileType GetFileType() const override {
        return IdentifyType(file);
    }

    LoadResult Load(Kernel::KProcess& process, Core::System& system) override;

    ResultStatus ReadRomFS(FileSys::VirtualFile& dir) override;
    u64 ReadRomFSIVFCOffset() const override;
    ResultStatus ReadProgramId(u64& out_program_id) override;

    ResultStatus ReadBanner(std::vector<u8>& buffer) override;
    ResultStatus ReadLogo(std::vector<u8>& buffer) override;

    ResultStatus ReadNSOModules(Modules& modules) override;

private:
    std::unique_ptr<FileSys::NCA> nca;
    std::unique_ptr<AppLoader_DeconstructedRomDirectory> directory_loader;
};

} // namespace Loader
