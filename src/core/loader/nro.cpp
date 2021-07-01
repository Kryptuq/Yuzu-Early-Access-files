// Copyright 2018 yuzu emulator team
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <utility>
#include <vector>

#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/logging/log.h"
#include "common/settings.h"
#include "common/swap.h"
#include "core/core.h"
#include "core/file_sys/control_metadata.h"
#include "core/file_sys/romfs_factory.h"
#include "core/file_sys/vfs_offset.h"
#include "core/hle/kernel/code_set.h"
#include "core/hle/kernel/k_page_table.h"
#include "core/hle/kernel/k_process.h"
#include "core/hle/kernel/k_thread.h"
#include "core/hle/service/filesystem/filesystem.h"
#include "core/loader/nro.h"
#include "core/loader/nso.h"
#include "core/memory.h"

namespace Loader {

struct NroSegmentHeader {
    u32_le offset;
    u32_le size;
};
static_assert(sizeof(NroSegmentHeader) == 0x8, "NroSegmentHeader has incorrect size.");

struct NroHeader {
    INSERT_PADDING_BYTES(0x4);
    u32_le module_header_offset;
    INSERT_PADDING_BYTES(0x8);
    u32_le magic;
    INSERT_PADDING_BYTES(0x4);
    u32_le file_size;
    INSERT_PADDING_BYTES(0x4);
    std::array<NroSegmentHeader, 3> segments; // Text, RoData, Data (in that order)
    u32_le bss_size;
    INSERT_PADDING_BYTES(0x44);
};
static_assert(sizeof(NroHeader) == 0x80, "NroHeader has incorrect size.");

struct ModHeader {
    u32_le magic;
    u32_le dynamic_offset;
    u32_le bss_start_offset;
    u32_le bss_end_offset;
    u32_le unwind_start_offset;
    u32_le unwind_end_offset;
    u32_le module_offset; // Offset to runtime-generated module object. typically equal to .bss base
};
static_assert(sizeof(ModHeader) == 0x1c, "ModHeader has incorrect size.");

struct AssetSection {
    u64_le offset;
    u64_le size;
};
static_assert(sizeof(AssetSection) == 0x10, "AssetSection has incorrect size.");

struct AssetHeader {
    u32_le magic;
    u32_le format_version;
    AssetSection icon;
    AssetSection nacp;
    AssetSection romfs;
};
static_assert(sizeof(AssetHeader) == 0x38, "AssetHeader has incorrect size.");

AppLoader_NRO::AppLoader_NRO(FileSys::VirtualFile file_) : AppLoader(std::move(file_)) {
    NroHeader nro_header{};
    if (file->ReadObject(&nro_header) != sizeof(NroHeader)) {
        return;
    }

    if (file->GetSize() >= nro_header.file_size + sizeof(AssetHeader)) {
        const u64 offset = nro_header.file_size;
        AssetHeader asset_header{};
        if (file->ReadObject(&asset_header, offset) != sizeof(AssetHeader)) {
            return;
        }

        if (asset_header.format_version != 0) {
            LOG_WARNING(Loader,
                        "NRO Asset Header has format {}, currently supported format is 0. If "
                        "strange glitches occur with metadata, check NRO assets.",
                        asset_header.format_version);
        }

        if (asset_header.magic != Common::MakeMagic('A', 'S', 'E', 'T')) {
            return;
        }

        if (asset_header.nacp.size > 0) {
            nacp = std::make_unique<FileSys::NACP>(std::make_shared<FileSys::OffsetVfsFile>(
                file, asset_header.nacp.size, offset + asset_header.nacp.offset, "Control.nacp"));
        }

        if (asset_header.romfs.size > 0) {
            romfs = std::make_shared<FileSys::OffsetVfsFile>(
                file, asset_header.romfs.size, offset + asset_header.romfs.offset, "game.romfs");
        }

        if (asset_header.icon.size > 0) {
            icon_data = file->ReadBytes(asset_header.icon.size, offset + asset_header.icon.offset);
        }
    }
}

AppLoader_NRO::~AppLoader_NRO() = default;

FileType AppLoader_NRO::IdentifyType(const FileSys::VirtualFile& nro_file) {
    // Read NSO header
    NroHeader nro_header{};
    if (sizeof(NroHeader) != nro_file->ReadObject(&nro_header)) {
        return FileType::Error;
    }
    if (nro_header.magic == Common::MakeMagic('N', 'R', 'O', '0')) {
        return FileType::NRO;
    }
    return FileType::Error;
}

static constexpr u32 PageAlignSize(u32 size) {
    return static_cast<u32>((size + Core::Memory::PAGE_MASK) & ~Core::Memory::PAGE_MASK);
}

static bool LoadNroImpl(Kernel::KProcess& process, const std::vector<u8>& data) {
    if (data.size() < sizeof(NroHeader)) {
        return {};
    }

    // Read NSO header
    NroHeader nro_header{};
    std::memcpy(&nro_header, data.data(), sizeof(NroHeader));
    if (nro_header.magic != Common::MakeMagic('N', 'R', 'O', '0')) {
        return {};
    }

    // Build program image
    Kernel::PhysicalMemory program_image(PageAlignSize(nro_header.file_size));
    std::memcpy(program_image.data(), data.data(), program_image.size());
    if (program_image.size() != PageAlignSize(nro_header.file_size)) {
        return {};
    }

    Kernel::CodeSet codeset;
    for (std::size_t i = 0; i < nro_header.segments.size(); ++i) {
        codeset.segments[i].addr = nro_header.segments[i].offset;
        codeset.segments[i].offset = nro_header.segments[i].offset;
        codeset.segments[i].size = PageAlignSize(nro_header.segments[i].size);
    }

    if (!Settings::values.program_args.empty()) {
        const auto arg_data = Settings::values.program_args;
        codeset.DataSegment().size += NSO_ARGUMENT_DATA_ALLOCATION_SIZE;
        NSOArgumentHeader args_header{
            NSO_ARGUMENT_DATA_ALLOCATION_SIZE, static_cast<u32_le>(arg_data.size()), {}};
        const auto end_offset = program_image.size();
        program_image.resize(static_cast<u32>(program_image.size()) +
                             NSO_ARGUMENT_DATA_ALLOCATION_SIZE);
        std::memcpy(program_image.data() + end_offset, &args_header, sizeof(NSOArgumentHeader));
        std::memcpy(program_image.data() + end_offset + sizeof(NSOArgumentHeader), arg_data.data(),
                    arg_data.size());
    }

    // Default .bss to NRO header bss size if MOD0 section doesn't exist
    u32 bss_size{PageAlignSize(nro_header.bss_size)};

    // Read MOD header
    ModHeader mod_header{};
    std::memcpy(&mod_header, program_image.data() + nro_header.module_header_offset,
                sizeof(ModHeader));

    const bool has_mod_header{mod_header.magic == Common::MakeMagic('M', 'O', 'D', '0')};
    if (has_mod_header) {
        // Resize program image to include .bss section and page align each section
        bss_size = PageAlignSize(mod_header.bss_end_offset - mod_header.bss_start_offset);
    }

    codeset.DataSegment().size += bss_size;
    program_image.resize(static_cast<u32>(program_image.size()) + bss_size);

    // Setup the process code layout
    if (process.LoadFromMetadata(FileSys::ProgramMetadata::GetDefault(), program_image.size())
            .IsError()) {
        return false;
    }

    // Load codeset for current process
    codeset.memory = std::move(program_image);
    process.LoadModule(std::move(codeset), process.PageTable().GetCodeRegionStart());

    return true;
}

bool AppLoader_NRO::LoadNro(Kernel::KProcess& process, const FileSys::VfsFile& nro_file) {
    return LoadNroImpl(process, nro_file.ReadAllBytes());
}

AppLoader_NRO::LoadResult AppLoader_NRO::Load(Kernel::KProcess& process, Core::System& system) {
    if (is_loaded) {
        return {ResultStatus::ErrorAlreadyLoaded, {}};
    }

    if (!LoadNro(process, *file)) {
        return {ResultStatus::ErrorLoadingNRO, {}};
    }

    if (romfs != nullptr) {
        system.GetFileSystemController().RegisterRomFS(std::make_unique<FileSys::RomFSFactory>(
            *this, system.GetContentProvider(), system.GetFileSystemController()));
    }

    is_loaded = true;
    return {ResultStatus::Success, LoadParameters{Kernel::KThread::DefaultThreadPriority,
                                                  Core::Memory::DEFAULT_STACK_SIZE}};
}

ResultStatus AppLoader_NRO::ReadIcon(std::vector<u8>& buffer) {
    if (icon_data.empty()) {
        return ResultStatus::ErrorNoIcon;
    }

    buffer = icon_data;
    return ResultStatus::Success;
}

ResultStatus AppLoader_NRO::ReadProgramId(u64& out_program_id) {
    if (nacp == nullptr) {
        return ResultStatus::ErrorNoControl;
    }

    out_program_id = nacp->GetTitleId();
    return ResultStatus::Success;
}

ResultStatus AppLoader_NRO::ReadRomFS(FileSys::VirtualFile& dir) {
    if (romfs == nullptr) {
        return ResultStatus::ErrorNoRomFS;
    }

    dir = romfs;
    return ResultStatus::Success;
}

ResultStatus AppLoader_NRO::ReadTitle(std::string& title) {
    if (nacp == nullptr) {
        return ResultStatus::ErrorNoControl;
    }

    title = nacp->GetApplicationName();
    return ResultStatus::Success;
}

ResultStatus AppLoader_NRO::ReadControlData(FileSys::NACP& control) {
    if (nacp == nullptr) {
        return ResultStatus::ErrorNoControl;
    }

    control = *nacp;
    return ResultStatus::Success;
}

bool AppLoader_NRO::IsRomFSUpdatable() const {
    return false;
}

} // namespace Loader
