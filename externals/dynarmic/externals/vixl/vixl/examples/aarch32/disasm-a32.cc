// Copyright 2016, VIXL authors
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//   * Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//   * Neither the name of ARM Limited nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


// The example assumes support for ELF binaries.
#ifdef __linux__

extern "C" {
#include <elf.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
}

#include <cerrno>
#include <iostream>
#include <map>
#include <string>

#include "globals-vixl.h"
#include "aarch32/disasm-aarch32.h"
#include "aarch32/instructions-aarch32.h"

class Symbol {
  Elf32_Addr addr_;
  int32_t offset_;
  uint32_t size_;
  int section_;
  std::string name_;

 public:
  Symbol(const char* name,
         Elf32_Addr addr,
         int32_t offset,
         uint32_t size,
         int section)
      : addr_(addr),
        offset_(offset),
        size_(size),
        section_(section),
        name_(name) {}
  Symbol(const Symbol& ref)
      : addr_(ref.addr_),
        offset_(ref.offset_),
        size_(ref.size_),
        section_(ref.section_),
        name_(ref.name_) {}

  Elf32_Addr GetAddress() const { return addr_; }
  Elf32_Addr GetMemoryAddress() const { return (addr_ & ~1) + offset_; }
  uint32_t GetSize() const { return size_; }
  const std::string& GetName() const { return name_; }
  int GetSection() const { return section_; }
};


class SymbolTable : public std::map<Elf32_Addr, Symbol> {
 public:
  void insert(const Symbol& sym) {
    VIXL_ASSERT(find(sym.GetAddress()) == end());
    std::map<Elf32_Addr, Symbol>::insert(
        std::make_pair(sym.GetMemoryAddress(), sym));
  }
};


class SectionLocator {
  const Elf32_Shdr* shdr_;
  int nsections_;
  const char* shstrtab_;

 public:
  explicit SectionLocator(const Elf32_Ehdr* ehdr) {
    shdr_ = reinterpret_cast<const Elf32_Shdr*>(
        reinterpret_cast<const char*>(ehdr) + ehdr->e_shoff);
    // shstrtab holds the section names as an offset in the file.
    shstrtab_ =
        reinterpret_cast<const char*>(ehdr) + shdr_[ehdr->e_shstrndx].sh_offset;
    nsections_ = ehdr->e_shnum;
  }

  const Elf32_Shdr* Locate(Elf32_Word type,
                           const std::string& section_name) const {
    for (int shnum = 1; shnum < nsections_; shnum++) {
      if ((shdr_[shnum].sh_type == type) &&
          std::string(shstrtab_ + shdr_[shnum].sh_name) == section_name) {
        return &shdr_[shnum];
      }
    }
    return NULL;
  }
};


template <typename VISITOR>
void LocateSymbols(const Elf32_Ehdr* ehdr,
                   const Elf32_Shdr* symtab,
                   const Elf32_Shdr* strtab,
                   VISITOR* visitor) {
  if ((symtab != NULL) && (strtab != NULL)) {
    const Elf32_Shdr* shdr = reinterpret_cast<const Elf32_Shdr*>(
        reinterpret_cast<const char*>(ehdr) + ehdr->e_shoff);

    const char* symnames =
        reinterpret_cast<const char*>(ehdr) + strtab->sh_offset;
    VIXL_CHECK(symnames != NULL);

    int nsym = symtab->sh_size / symtab->sh_entsize;
    const Elf32_Sym* sym = reinterpret_cast<const Elf32_Sym*>(
        reinterpret_cast<const char*>(ehdr) + symtab->sh_offset);
    for (int snum = 0; snum < nsym; snum++) {
      if ((sym[snum].st_shndx > 0) && (sym[snum].st_shndx < ehdr->e_shnum) &&
          (sym[snum].st_value != 0) &&
          (shdr[sym[snum].st_shndx].sh_type == SHT_PROGBITS) &&
          ((ELF32_ST_BIND(sym[snum].st_info) == STB_LOCAL) ||
           (ELF32_ST_BIND(sym[snum].st_info) == STB_GLOBAL)) &&
          (ELF32_ST_TYPE(sym[snum].st_info) == STT_FUNC)) {
        visitor->visit(symnames + sym[snum].st_name, sym[snum]);
      }
    }
  }
}


class DynamicSymbolVisitor {
  SymbolTable* symbols_;

 public:
  explicit DynamicSymbolVisitor(SymbolTable* symbols) : symbols_(symbols) {}
  void visit(const char* symname, const Elf32_Sym& sym) {
    symbols_->insert(
        Symbol(symname, sym.st_value, 0, sym.st_size, sym.st_shndx));
  }
};


class StaticSymbolVisitor {
  const Elf32_Ehdr* ehdr_;
  const Elf32_Shdr* shdr_;
  SymbolTable* symbols_;

 public:
  StaticSymbolVisitor(const Elf32_Ehdr* ehdr, SymbolTable* symbols)
      : ehdr_(ehdr),
        shdr_(reinterpret_cast<const Elf32_Shdr*>(
            reinterpret_cast<const char*>(ehdr) + ehdr->e_shoff)),
        symbols_(symbols) {}
  void visit(const char* symname, const Elf32_Sym& sym) {
    if (ehdr_->e_type == ET_REL) {
      symbols_->insert(Symbol(symname,
                              sym.st_value,
                              shdr_[sym.st_shndx].sh_offset,
                              sym.st_size,
                              sym.st_shndx));
    } else {
      symbols_->insert(
          Symbol(symname,
                 sym.st_value,
                 shdr_[sym.st_shndx].sh_offset - shdr_[sym.st_shndx].sh_addr,
                 sym.st_size,
                 sym.st_shndx));
    }
  }
};


void usage() {
  std::cout << "usage: disasm-a32 <file>\n"
               "where <file> is an ELF ARM binaryfile, either an executable, "
               "a shared object, or an object file."
            << std::endl;
}


int main(int argc, char** argv) {
  const int kErrorNotARMELF32 = -1;
  const int kErrorArguments = -2;
  if (argc < 2) {
    usage();
    return kErrorArguments;
  }

  const char* filename = argv[1];
  struct stat sb;


  if (lstat(filename, &sb) == -1) {
    std::cerr << "Cannot stat this file" << filename << std::endl;
    return errno;
  }

  if (S_ISLNK(sb.st_mode)) {
    static char linkname[4096];
    filename = realpath(argv[1], linkname);
    if (lstat(linkname, &sb) == -1) {
      std::cerr << "Cannot stat this file: " << linkname << std::endl;
      return errno;
    }
  }

  int elf_in;
  if ((elf_in = open(filename, O_RDONLY)) < 0) {
    std::cerr << "Cannot open: " << argv[1];
    if (filename != argv[1]) std::cerr << " aka " << filename;
    std::cerr << std::endl;
    return errno;
  }

  char* base_addr;
  VIXL_CHECK((base_addr = reinterpret_cast<char*>(
                  mmap(0, sb.st_size, PROT_READ, MAP_PRIVATE, elf_in, 0))) !=
             0);

  const Elf32_Ehdr* ehdr = reinterpret_cast<const Elf32_Ehdr*>(base_addr);
  if ((ehdr->e_ident[0] != 0x7f) || (ehdr->e_ident[1] != 'E') ||
      (ehdr->e_ident[2] != 'L') || (ehdr->e_ident[3] != 'F') ||
      (ehdr->e_ehsize != sizeof(Elf32_Ehdr))) {
    std::cerr << "This file is not an 32-bit ELF file." << std::endl;
    munmap(base_addr, sb.st_size);
    return kErrorNotARMELF32;
  }

  if (ehdr->e_machine != EM_ARM) {
    std::cerr << "This file is not using the ARM isa." << std::endl;
    munmap(base_addr, sb.st_size);
    return kErrorNotARMELF32;
  }

  // shstrtab holds the section names as an offset in the file.
  const Elf32_Shdr* shdr =
      reinterpret_cast<const Elf32_Shdr*>(base_addr + ehdr->e_shoff);

  SectionLocator section_locator(ehdr);

  SymbolTable symbol_names;

  // Traverse the dynamic symbols defined in any text section
  DynamicSymbolVisitor dynamic_visitor(&symbol_names);
  LocateSymbols(ehdr,
                section_locator.Locate(SHT_DYNSYM, ".dynsym"),
                section_locator.Locate(SHT_STRTAB, ".dynstr"),
                &dynamic_visitor);

  // Traverse the static symbols defined in the any test section
  StaticSymbolVisitor static_visitor(ehdr, &symbol_names);
  LocateSymbols(ehdr,
                section_locator.Locate(SHT_SYMTAB, ".symtab"),
                section_locator.Locate(SHT_STRTAB, ".strtab"),
                &static_visitor);


  vixl::aarch32::PrintDisassembler dis(std::cout, 0);
  for (SymbolTable::iterator sres = symbol_names.begin();
       sres != symbol_names.end();
       sres++) {
    const Symbol& symbol = sres->second;
    uint32_t func_addr = symbol.GetAddress();
    uint32_t func_size = symbol.GetSize();
    if (func_size == 0) {
      SymbolTable::iterator next_func = sres;
      next_func++;
      if (next_func == symbol_names.end()) {
        const Elf32_Shdr& shndx = shdr[sres->second.GetSection()];
        func_size = (shndx.sh_offset + shndx.sh_size) - sres->first;
      } else {
        func_size = next_func->first - sres->first;
      }
    }

    std::cout << "--- " << symbol.GetName() << ":" << std::endl;
    if ((func_addr & 1) == 1) {
      func_addr &= ~1;
      dis.SetCodeAddress(func_addr);
      dis.DisassembleT32Buffer(reinterpret_cast<uint16_t*>(
                                   base_addr + symbol.GetMemoryAddress()),
                               func_size);
    } else {
      dis.SetCodeAddress(func_addr);
      dis.DisassembleA32Buffer(reinterpret_cast<uint32_t*>(
                                   base_addr + symbol.GetMemoryAddress()),
                               func_size);
    }
  }
  munmap(base_addr, sb.st_size);
  return 0;
}


#else

#include "globals-vixl.h"

// TODO: Implement this example for macOS.
int main(void) {
  VIXL_WARNING("This example has not been implemented for macOS.");
  return 0;
}

#endif  // __linux__
