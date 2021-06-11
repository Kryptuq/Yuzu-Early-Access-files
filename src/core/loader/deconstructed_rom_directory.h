// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <string>
#include "common/common_types.h"
#include "core/file_sys/program_metadata.h"
#include "core/loader/loader.h"

namespace Core {
class System;
}

namespace Loader {

/**
 * This class loads a "deconstructed ROM directory", which are the typical format we see for Switch
 * game dumps. The path should be a "main" NSO, which must be in a directory that contains the other
 * standard ExeFS NSOs (e.g. rtld, sdk, etc.). It will automatically find and load these.
 * Furthermore, it will look for the first .romfs file (optionally) and use this for the RomFS.
 */
class AppLoader_DeconstructedRomDirectory final : public AppLoader {
public:
    explicit AppLoader_DeconstructedRomDirectory(FileSys::VirtualFile main_file,
                                                 bool override_update_ = false);

    // Overload to accept exefs directory. Must contain 'main' and 'main.npdm'
    explicit AppLoader_DeconstructedRomDirectory(FileSys::VirtualDir directory,
                                                 bool override_update_ = false);

    /**
     * Identifies whether or not the given file is a deconstructed ROM directory.
     *
     * @param dir_file The file to verify.
     *
     * @return FileType::DeconstructedRomDirectory, or FileType::Error
     *         if the file is not a deconstructed ROM directory.
     */
    static FileType IdentifyType(const FileSys::VirtualFile& dir_file);

    FileType GetFileType() const override {
        return IdentifyType(file);
    }

    LoadResult Load(Kernel::KProcess& process, Core::System& system) override;

    ResultStatus ReadRomFS(FileSys::VirtualFile& out_dir) override;
    ResultStatus ReadIcon(std::vector<u8>& out_buffer) override;
    ResultStatus ReadProgramId(u64& out_program_id) override;
    ResultStatus ReadTitle(std::string& title) override;
    bool IsRomFSUpdatable() const override;

    ResultStatus ReadNSOModules(Modules& out_modules) override;

private:
    FileSys::ProgramMetadata metadata;
    FileSys::VirtualFile romfs;
    FileSys::VirtualDir dir;

    std::vector<u8> icon_data;
    std::string name;
    u64 title_id{};
    bool override_update;

    Modules modules;
};

} // namespace Loader
