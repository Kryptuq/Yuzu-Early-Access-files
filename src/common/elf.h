// SPDX-FileCopyrightText: 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <cstddef>

#include "common_types.h"

namespace Common {
namespace ELF {

/* Type for a 16-bit quantity.  */
using Elf32_Half = u16;
using Elf64_Half = u16;

/* Types for signed and unsigned 32-bit quantities.  */
using Elf32_Word = u32;
using Elf32_Sword = s32;
using Elf64_Word = u32;
using Elf64_Sword = s32;

/* Types for signed and unsigned 64-bit quantities.  */
using Elf32_Xword = u64;
using Elf32_Sxword = s64;
using Elf64_Xword = u64;
using Elf64_Sxword = s64;

/* Type of addresses.  */
using Elf32_Addr = u32;
using Elf64_Addr = u64;

/* Type of file offsets.  */
using Elf32_Off = u32;
using Elf64_Off = u64;

/* Type for section indices, which are 16-bit quantities.  */
using Elf32_Section = u16;
using Elf64_Section = u16;

/* Type for version symbol information.  */
using Elf32_Versym = Elf32_Half;
using Elf64_Versym = Elf64_Half;

constexpr size_t ElfIdentSize = 16;

/* The ELF file header.  This appears at the start of every ELF file.  */

struct Elf32_Ehdr {
    std::array<u8, ElfIdentSize> e_ident; /* Magic number and other info */
    Elf32_Half e_type;                    /* Object file type */
    Elf32_Half e_machine;                 /* Architecture */
    Elf32_Word e_version;                 /* Object file version */
    Elf32_Addr e_entry;                   /* Entry point virtual address */
    Elf32_Off e_phoff;                    /* Program header table file offset */
    Elf32_Off e_shoff;                    /* Section header table file offset */
    Elf32_Word e_flags;                   /* Processor-specific flags */
    Elf32_Half e_ehsize;                  /* ELF header size in bytes */
    Elf32_Half e_phentsize;               /* Program header table entry size */
    Elf32_Half e_phnum;                   /* Program header table entry count */
    Elf32_Half e_shentsize;               /* Section header table entry size */
    Elf32_Half e_shnum;                   /* Section header table entry count */
    Elf32_Half e_shstrndx;                /* Section header string table index */
};

struct Elf64_Ehdr {
    std::array<u8, ElfIdentSize> e_ident; /* Magic number and other info */
    Elf64_Half e_type;                    /* Object file type */
    Elf64_Half e_machine;                 /* Architecture */
    Elf64_Word e_version;                 /* Object file version */
    Elf64_Addr e_entry;                   /* Entry point virtual address */
    Elf64_Off e_phoff;                    /* Program header table file offset */
    Elf64_Off e_shoff;                    /* Section header table file offset */
    Elf64_Word e_flags;                   /* Processor-specific flags */
    Elf64_Half e_ehsize;                  /* ELF header size in bytes */
    Elf64_Half e_phentsize;               /* Program header table entry size */
    Elf64_Half e_phnum;                   /* Program header table entry count */
    Elf64_Half e_shentsize;               /* Section header table entry size */
    Elf64_Half e_shnum;                   /* Section header table entry count */
    Elf64_Half e_shstrndx;                /* Section header string table index */
};

constexpr u8 ElfClass32 = 1;        /* 32-bit objects */
constexpr u8 ElfClass64 = 2;        /* 64-bit objects */
constexpr u8 ElfData2Lsb = 1;       /* 2's complement, little endian */
constexpr u8 ElfVersionCurrent = 1; /* EV_CURRENT */
constexpr u8 ElfOsAbiNone = 0;      /* System V ABI */

constexpr u16 ElfTypeNone = 0; /* No file type */
constexpr u16 ElfTypeRel = 0;  /* Relocatable file */
constexpr u16 ElfTypeExec = 0; /* Executable file */
constexpr u16 ElfTypeDyn = 0;  /* Shared object file */

constexpr u16 ElfMachineArm = 40;      /* ARM */
constexpr u16 ElfMachineAArch64 = 183; /* ARM AARCH64 */

constexpr std::array<u8, ElfIdentSize> Elf32Ident{
    0x7f, 'E', 'L', 'F', ElfClass32, ElfData2Lsb, ElfVersionCurrent, ElfOsAbiNone};

constexpr std::array<u8, ElfIdentSize> Elf64Ident{
    0x7f, 'E', 'L', 'F', ElfClass64, ElfData2Lsb, ElfVersionCurrent, ElfOsAbiNone};

/* Section header.  */

struct Elf32_Shdr {
    Elf32_Word sh_name;      /* Section name (string tbl index) */
    Elf32_Word sh_type;      /* Section type */
    Elf32_Word sh_flags;     /* Section flags */
    Elf32_Addr sh_addr;      /* Section virtual addr at execution */
    Elf32_Off sh_offset;     /* Section file offset */
    Elf32_Word sh_size;      /* Section size in bytes */
    Elf32_Word sh_link;      /* Link to another section */
    Elf32_Word sh_info;      /* Additional section information */
    Elf32_Word sh_addralign; /* Section alignment */
    Elf32_Word sh_entsize;   /* Entry size if section holds table */
};

struct Elf64_Shdr {
    Elf64_Word sh_name;       /* Section name (string tbl index) */
    Elf64_Word sh_type;       /* Section type */
    Elf64_Xword sh_flags;     /* Section flags */
    Elf64_Addr sh_addr;       /* Section virtual addr at execution */
    Elf64_Off sh_offset;      /* Section file offset */
    Elf64_Xword sh_size;      /* Section size in bytes */
    Elf64_Word sh_link;       /* Link to another section */
    Elf64_Word sh_info;       /* Additional section information */
    Elf64_Xword sh_addralign; /* Section alignment */
    Elf64_Xword sh_entsize;   /* Entry size if section holds table */
};

constexpr u32 ElfShnUndef = 0; /* Undefined section */

constexpr u32 ElfShtNull = 0;     /* Section header table entry unused */
constexpr u32 ElfShtProgBits = 1; /* Program data */
constexpr u32 ElfShtSymtab = 2;   /* Symbol table */
constexpr u32 ElfShtStrtab = 3;   /* String table */
constexpr u32 ElfShtRela = 4;     /* Relocation entries with addends */
constexpr u32 ElfShtDynamic = 6;  /* Dynamic linking information */
constexpr u32 ElfShtNobits = 7;   /* Program space with no data (bss) */
constexpr u32 ElfShtRel = 9;      /* Relocation entries, no addends */
constexpr u32 ElfShtDynsym = 11;  /* Dynamic linker symbol table */

/* Symbol table entry.  */

struct Elf32_Sym {
    Elf32_Word st_name;     /* Symbol name (string tbl index) */
    Elf32_Addr st_value;    /* Symbol value */
    Elf32_Word st_size;     /* Symbol size */
    u8 st_info;             /* Symbol type and binding */
    u8 st_other;            /* Symbol visibility */
    Elf32_Section st_shndx; /* Section index */
};

struct Elf64_Sym {
    Elf64_Word st_name;     /* Symbol name (string tbl index) */
    u8 st_info;             /* Symbol type and binding */
    u8 st_other;            /* Symbol visibility */
    Elf64_Section st_shndx; /* Section index */
    Elf64_Addr st_value;    /* Symbol value */
    Elf64_Xword st_size;    /* Symbol size */
};

/* How to extract and insert information held in the st_info field.  */

static inline u8 ElfStBind(u8 st_info) {
    return st_info >> 4;
}
static inline u8 ElfStType(u8 st_info) {
    return st_info & 0xf;
}
static inline u8 ElfStInfo(u8 st_bind, u8 st_type) {
    return static_cast<u8>((st_bind << 4) + (st_type & 0xf));
}

constexpr u8 ElfBindLocal = 0;  /* Local symbol */
constexpr u8 ElfBindGlobal = 1; /* Global symbol */
constexpr u8 ElfBindWeak = 2;   /* Weak symbol */

constexpr u8 ElfTypeUnspec = 0; /* Symbol type is unspecified */
constexpr u8 ElfTypeObject = 1; /* Symbol is a data object */
constexpr u8 ElfTypeFunc = 2;   /* Symbol is a code object */

static inline u8 ElfStVisibility(u8 st_other) {
    return static_cast<u8>(st_other & 0x3);
}

constexpr u8 ElfVisibilityDefault = 0;   /* Default symbol visibility rules */
constexpr u8 ElfVisibilityInternal = 1;  /* Processor specific hidden class */
constexpr u8 ElfVisibilityHidden = 2;    /* Sym unavailable in other modules */
constexpr u8 ElfVisibilityProtected = 3; /* Not preemptible, not exported */

/* Relocation table entry without addend (in section of type ShtRel).  */

struct Elf32_Rel {
    Elf32_Addr r_offset; /* Address */
    Elf32_Word r_info;   /* Relocation type and symbol index */
};

/* Relocation table entry with addend (in section of type ShtRela).  */

struct Elf32_Rela {
    Elf32_Addr r_offset;  /* Address */
    Elf32_Word r_info;    /* Relocation type and symbol index */
    Elf32_Sword r_addend; /* Addend */
};

struct Elf64_Rela {
    Elf64_Addr r_offset;   /* Address */
    Elf64_Xword r_info;    /* Relocation type and symbol index */
    Elf64_Sxword r_addend; /* Addend */
};

/* How to extract and insert information held in the r_info field.  */

static inline u32 Elf32RelSymIndex(Elf32_Word r_info) {
    return r_info >> 8;
}
static inline u8 Elf32RelType(Elf32_Word r_info) {
    return static_cast<u8>(r_info & 0xff);
}
static inline Elf32_Word Elf32RelInfo(u32 sym_index, u8 type) {
    return (sym_index << 8) + type;
}
static inline u32 Elf64RelSymIndex(Elf64_Xword r_info) {
    return static_cast<u32>(r_info >> 32);
}
static inline u32 Elf64RelType(Elf64_Xword r_info) {
    return r_info & 0xffffffff;
}
static inline Elf64_Xword Elf64RelInfo(u32 sym_index, u32 type) {
    return (static_cast<Elf64_Xword>(sym_index) << 32) + type;
}

constexpr u32 ElfArmCopy = 20;     /* Copy symbol at runtime */
constexpr u32 ElfArmGlobDat = 21;  /* Create GOT entry */
constexpr u32 ElfArmJumpSlot = 22; /* Create PLT entry */
constexpr u32 ElfArmRelative = 23; /* Adjust by program base */

constexpr u32 ElfAArch64Copy = 1024;     /* Copy symbol at runtime */
constexpr u32 ElfAArch64GlobDat = 1025;  /* Create GOT entry */
constexpr u32 ElfAArch64JumpSlot = 1026; /* Create PLT entry */
constexpr u32 ElfAArch64Relative = 1027; /* Adjust by program base */

/* Program segment header.  */

struct Elf32_Phdr {
    Elf32_Word p_type;   /* Segment type */
    Elf32_Off p_offset;  /* Segment file offset */
    Elf32_Addr p_vaddr;  /* Segment virtual address */
    Elf32_Addr p_paddr;  /* Segment physical address */
    Elf32_Word p_filesz; /* Segment size in file */
    Elf32_Word p_memsz;  /* Segment size in memory */
    Elf32_Word p_flags;  /* Segment flags */
    Elf32_Word p_align;  /* Segment alignment */
};

struct Elf64_Phdr {
    Elf64_Word p_type;    /* Segment type */
    Elf64_Word p_flags;   /* Segment flags */
    Elf64_Off p_offset;   /* Segment file offset */
    Elf64_Addr p_vaddr;   /* Segment virtual address */
    Elf64_Addr p_paddr;   /* Segment physical address */
    Elf64_Xword p_filesz; /* Segment size in file */
    Elf64_Xword p_memsz;  /* Segment size in memory */
    Elf64_Xword p_align;  /* Segment alignment */
};

/* Legal values for p_type (segment type).  */

constexpr u32 ElfPtNull = 0;    /* Program header table entry unused */
constexpr u32 ElfPtLoad = 1;    /* Loadable program segment */
constexpr u32 ElfPtDynamic = 2; /* Dynamic linking information */
constexpr u32 ElfPtInterp = 3;  /* Program interpreter */
constexpr u32 ElfPtNote = 4;    /* Auxiliary information */
constexpr u32 ElfPtPhdr = 6;    /* Entry for header table itself */
constexpr u32 ElfPtTls = 7;     /* Thread-local storage segment */

/* Legal values for p_flags (segment flags).  */

constexpr u32 ElfPfExec = 0;  /* Segment is executable */
constexpr u32 ElfPfWrite = 1; /* Segment is writable */
constexpr u32 ElfPfRead = 2;  /* Segment is readable */

/* Dynamic section entry.  */

struct Elf32_Dyn {
    Elf32_Sword d_tag; /* Dynamic entry type */
    union {
        Elf32_Word d_val; /* Integer value */
        Elf32_Addr d_ptr; /* Address value */
    } d_un;
};

struct Elf64_Dyn {
    Elf64_Sxword d_tag; /* Dynamic entry type */
    union {
        Elf64_Xword d_val; /* Integer value */
        Elf64_Addr d_ptr;  /* Address value */
    } d_un;
};

/* Legal values for d_tag (dynamic entry type).  */

constexpr u32 ElfDtNull = 0;         /* Marks end of dynamic section */
constexpr u32 ElfDtNeeded = 1;       /* Name of needed library */
constexpr u32 ElfDtPltRelSz = 2;     /* Size in bytes of PLT relocs */
constexpr u32 ElfDtPltGot = 3;       /* Processor defined value */
constexpr u32 ElfDtHash = 4;         /* Address of symbol hash table */
constexpr u32 ElfDtStrtab = 5;       /* Address of string table */
constexpr u32 ElfDtSymtab = 6;       /* Address of symbol table */
constexpr u32 ElfDtRela = 7;         /* Address of Rela relocs */
constexpr u32 ElfDtRelasz = 8;       /* Total size of Rela relocs */
constexpr u32 ElfDtRelaent = 9;      /* Size of one Rela reloc */
constexpr u32 ElfDtStrsz = 10;       /* Size of string table */
constexpr u32 ElfDtSyment = 11;      /* Size of one symbol table entry */
constexpr u32 ElfDtInit = 12;        /* Address of init function */
constexpr u32 ElfDtFini = 13;        /* Address of termination function */
constexpr u32 ElfDtRel = 17;         /* Address of Rel relocs */
constexpr u32 ElfDtRelsz = 18;       /* Total size of Rel relocs */
constexpr u32 ElfDtRelent = 19;      /* Size of one Rel reloc */
constexpr u32 ElfDtPltRel = 20;      /* Type of reloc in PLT */
constexpr u32 ElfDtTextRel = 22;     /* Reloc might modify .text */
constexpr u32 ElfDtJmpRel = 23;      /* Address of PLT relocs */
constexpr u32 ElfDtBindNow = 24;     /* Process relocations of object */
constexpr u32 ElfDtInitArray = 25;   /* Array with addresses of init fct */
constexpr u32 ElfDtFiniArray = 26;   /* Array with addresses of fini fct */
constexpr u32 ElfDtInitArraySz = 27; /* Size in bytes of DT_INIT_ARRAY */
constexpr u32 ElfDtFiniArraySz = 28; /* Size in bytes of DT_FINI_ARRAY */
constexpr u32 ElfDtSymtabShndx = 34; /* Address of SYMTAB_SHNDX section */

} // namespace ELF
} // namespace Common
