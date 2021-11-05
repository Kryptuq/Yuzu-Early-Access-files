// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/file_sys/content_archive.h"
#include "core/file_sys/romfs.h"
#include "core/file_sys/xts_archive.h"
#include "core/hle/kernel/k_process.h"
#include "core/loader/nax.h"
#include "core/loader/nca.h"

namespace Loader {
namespace {
FileType IdentifyTypeImpl(const FileSys::NAX& nax) {
    if (nax.GetStatus() != ResultStatus::Success) {
        return FileType::Error;
    }

    const auto nca = nax.AsNCA();
    if (nca == nullptr || nca->GetStatus() != ResultStatus::Success) {
        return FileType::Error;
    }

    return FileType::NAX;
}
} // Anonymous namespace

AppLoader_NAX::AppLoader_NAX(FileSys::VirtualFile file_)
    : AppLoader(file_), nax(std::make_unique<FileSys::NAX>(file_)),
      nca_loader(std::make_unique<AppLoader_NCA>(nax->GetDecrypted())) {}

AppLoader_NAX::~AppLoader_NAX() = default;

FileType AppLoader_NAX::IdentifyType(const FileSys::VirtualFile& nax_file) {
    const FileSys::NAX nax(nax_file);
    return IdentifyTypeImpl(nax);
}

FileType AppLoader_NAX::GetFileType() const {
    return IdentifyTypeImpl(*nax);
}

AppLoader_NAX::LoadResult AppLoader_NAX::Load(Kernel::KProcess& process, Core::System& system) {
    if (is_loaded) {
        return {ResultStatus::ErrorAlreadyLoaded, {}};
    }

    const auto nax_status = nax->GetStatus();
    if (nax_status != ResultStatus::Success) {
        return {nax_status, {}};
    }

    const auto nca = nax->AsNCA();
    if (nca == nullptr) {
        if (!Core::Crypto::KeyManager::KeyFileExists(false)) {
            return {ResultStatus::ErrorMissingProductionKeyFile, {}};
        }

        return {ResultStatus::ErrorNAXInconvertibleToNCA, {}};
    }

    const auto nca_status = nca->GetStatus();
    if (nca_status != ResultStatus::Success) {
        return {nca_status, {}};
    }

    const auto result = nca_loader->Load(process, system);
    if (result.first != ResultStatus::Success) {
        return result;
    }

    is_loaded = true;
    return result;
}

ResultStatus AppLoader_NAX::ReadRomFS(FileSys::VirtualFile& dir) {
    return nca_loader->ReadRomFS(dir);
}

u64 AppLoader_NAX::ReadRomFSIVFCOffset() const {
    return nca_loader->ReadRomFSIVFCOffset();
}

ResultStatus AppLoader_NAX::ReadProgramId(u64& out_program_id) {
    return nca_loader->ReadProgramId(out_program_id);
}

ResultStatus AppLoader_NAX::ReadBanner(std::vector<u8>& buffer) {
    return nca_loader->ReadBanner(buffer);
}

ResultStatus AppLoader_NAX::ReadLogo(std::vector<u8>& buffer) {
    return nca_loader->ReadLogo(buffer);
}

ResultStatus AppLoader_NAX::ReadNSOModules(Modules& modules) {
    return nca_loader->ReadNSOModules(modules);
}

} // namespace Loader
