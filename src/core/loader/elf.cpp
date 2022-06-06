// SPDX-FileCopyrightText: 2013 Dolphin Emulator Project
// SPDX-FileCopyrightText: 2014 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cstring>
#include <memory>
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/elf.h"
#include "common/logging/log.h"
#include "core/hle/kernel/code_set.h"
#include "core/hle/kernel/k_page_table.h"
#include "core/hle/kernel/k_process.h"
#include "core/loader/elf.h"
#include "core/memory.h"

using namespace Common::ELF;

////////////////////////////////////////////////////////////////////////////////////////////////////
// ElfReader class

typedef int SectionID;

class ElfReader {
private:
    char* base;
    u32* base32;

    Elf32_Ehdr* header;
    Elf32_Phdr* segments;
    Elf32_Shdr* sections;

    u32* sectionAddrs;
    bool relocate;
    VAddr entryPoint;

public:
    explicit ElfReader(void* ptr);

    u32 Read32(int off) const {
        return base32[off >> 2];
    }

    // Quick accessors
    u16 GetType() const {
        return header->e_type;
    }
    u16 GetMachine() const {
        return header->e_machine;
    }
    VAddr GetEntryPoint() const {
        return entryPoint;
    }
    u32 GetFlags() const {
        return (u32)(header->e_flags);
    }
    Kernel::CodeSet LoadInto(VAddr vaddr);

    int GetNumSegments() const {
        return (int)(header->e_phnum);
    }
    int GetNumSections() const {
        return (int)(header->e_shnum);
    }
    const u8* GetPtr(int offset) const {
        return (u8*)base + offset;
    }
    const char* GetSectionName(int section) const;
    const u8* GetSectionDataPtr(int section) const {
        if (section < 0 || section >= header->e_shnum)
            return nullptr;
        if (sections[section].sh_type != ElfShtNobits)
            return GetPtr(sections[section].sh_offset);
        else
            return nullptr;
    }
    bool IsCodeSection(int section) const {
        return sections[section].sh_type == ElfShtProgBits;
    }
    const u8* GetSegmentPtr(int segment) {
        return GetPtr(segments[segment].p_offset);
    }
    u32 GetSectionAddr(SectionID section) const {
        return sectionAddrs[section];
    }
    unsigned int GetSectionSize(SectionID section) const {
        return sections[section].sh_size;
    }
    SectionID GetSectionByName(const char* name, int firstSection = 0) const; //-1 for not found

    bool DidRelocate() const {
        return relocate;
    }
};

ElfReader::ElfReader(void* ptr) {
    base = (char*)ptr;
    base32 = (u32*)ptr;
    header = (Elf32_Ehdr*)ptr;

    segments = (Elf32_Phdr*)(base + header->e_phoff);
    sections = (Elf32_Shdr*)(base + header->e_shoff);

    entryPoint = header->e_entry;
}

const char* ElfReader::GetSectionName(int section) const {
    if (sections[section].sh_type == ElfShtNull)
        return nullptr;

    int name_offset = sections[section].sh_name;
    const char* ptr = reinterpret_cast<const char*>(GetSectionDataPtr(header->e_shstrndx));

    if (ptr)
        return ptr + name_offset;

    return nullptr;
}

Kernel::CodeSet ElfReader::LoadInto(VAddr vaddr) {
    LOG_DEBUG(Loader, "String section: {}", header->e_shstrndx);

    // Should we relocate?
    relocate = (header->e_type != ElfTypeExec);

    if (relocate) {
        LOG_DEBUG(Loader, "Relocatable module");
        entryPoint += vaddr;
    } else {
        LOG_DEBUG(Loader, "Prerelocated executable");
    }
    LOG_DEBUG(Loader, "{} segments:", header->e_phnum);

    // First pass : Get the bits into RAM
    const VAddr base_addr = relocate ? vaddr : 0;

    u64 total_image_size = 0;
    for (unsigned int i = 0; i < header->e_phnum; ++i) {
        const Elf32_Phdr* p = &segments[i];
        if (p->p_type == ElfPtLoad) {
            total_image_size += (p->p_memsz + 0xFFF) & ~0xFFF;
        }
    }

    Kernel::PhysicalMemory program_image(total_image_size);
    std::size_t current_image_position = 0;

    Kernel::CodeSet codeset;

    for (unsigned int i = 0; i < header->e_phnum; ++i) {
        const Elf32_Phdr* p = &segments[i];
        LOG_DEBUG(Loader, "Type: {} Vaddr: {:08X} Filesz: {:08X} Memsz: {:08X} ", p->p_type,
                  p->p_vaddr, p->p_filesz, p->p_memsz);

        if (p->p_type == ElfPtLoad) {
            Kernel::CodeSet::Segment* codeset_segment;
            u32 permission_flags = p->p_flags & (ElfPfRead | ElfPfWrite | ElfPfExec);
            if (permission_flags == (ElfPfRead | ElfPfExec)) {
                codeset_segment = &codeset.CodeSegment();
            } else if (permission_flags == (ElfPfRead)) {
                codeset_segment = &codeset.RODataSegment();
            } else if (permission_flags == (ElfPfRead | ElfPfWrite)) {
                codeset_segment = &codeset.DataSegment();
            } else {
                LOG_ERROR(Loader, "Unexpected ELF PT_LOAD segment id {} with flags {:X}", i,
                          p->p_flags);
                continue;
            }

            if (codeset_segment->size != 0) {
                LOG_ERROR(Loader,
                          "ELF has more than one segment of the same type. Skipping extra "
                          "segment (id {})",
                          i);
                continue;
            }

            const VAddr segment_addr = base_addr + p->p_vaddr;
            const u32 aligned_size = (p->p_memsz + 0xFFF) & ~0xFFF;

            codeset_segment->offset = current_image_position;
            codeset_segment->addr = segment_addr;
            codeset_segment->size = aligned_size;

            std::memcpy(program_image.data() + current_image_position, GetSegmentPtr(i),
                        p->p_filesz);
            current_image_position += aligned_size;
        }
    }

    codeset.entrypoint = base_addr + header->e_entry;
    codeset.memory = std::move(program_image);

    LOG_DEBUG(Loader, "Done loading.");

    return codeset;
}

SectionID ElfReader::GetSectionByName(const char* name, int firstSection) const {
    for (int i = firstSection; i < header->e_shnum; i++) {
        const char* secname = GetSectionName(i);

        if (secname != nullptr && strcmp(name, secname) == 0)
            return i;
    }
    return -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Loader namespace

namespace Loader {

AppLoader_ELF::AppLoader_ELF(FileSys::VirtualFile file_) : AppLoader(std::move(file_)) {}

FileType AppLoader_ELF::IdentifyType(const FileSys::VirtualFile& elf_file) {
    static constexpr u16 ELF_MACHINE_ARM{0x28};

    u32 magic = 0;
    if (4 != elf_file->ReadObject(&magic)) {
        return FileType::Error;
    }

    u16 machine = 0;
    if (2 != elf_file->ReadObject(&machine, 18)) {
        return FileType::Error;
    }

    if (Common::MakeMagic('\x7f', 'E', 'L', 'F') == magic && ELF_MACHINE_ARM == machine) {
        return FileType::ELF;
    }

    return FileType::Error;
}

AppLoader_ELF::LoadResult AppLoader_ELF::Load(Kernel::KProcess& process,
                                              [[maybe_unused]] Core::System& system) {
    if (is_loaded) {
        return {ResultStatus::ErrorAlreadyLoaded, {}};
    }

    std::vector<u8> buffer = file->ReadAllBytes();
    if (buffer.size() != file->GetSize()) {
        return {ResultStatus::ErrorIncorrectELFFileSize, {}};
    }

    const VAddr base_address = process.PageTable().GetCodeRegionStart();
    ElfReader elf_reader(&buffer[0]);
    Kernel::CodeSet codeset = elf_reader.LoadInto(base_address);
    const VAddr entry_point = codeset.entrypoint;

    // Setup the process code layout
    if (process.LoadFromMetadata(FileSys::ProgramMetadata::GetDefault(), buffer.size()).IsError()) {
        return {ResultStatus::ErrorNotInitialized, {}};
    }

    process.LoadModule(std::move(codeset), entry_point);

    is_loaded = true;
    return {ResultStatus::Success, LoadParameters{48, Core::Memory::DEFAULT_STACK_SIZE}};
}

} // namespace Loader
