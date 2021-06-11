// Copyright 2015, VIXL authors
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

#ifndef VIXL_EXAMPLE_EXAMPLES_H_
#define VIXL_EXAMPLE_EXAMPLES_H_

extern "C" {
#include <stdint.h>
#ifndef VIXL_INCLUDE_SIMULATOR_AARCH32
#include <sys/mman.h>
#endif
}

#include <cstdio>
#include <string>

#include "aarch32/constants-aarch32.h"
#include "aarch32/instructions-aarch32.h"
#include "aarch32/macro-assembler-aarch32.h"

#ifndef VIXL_INCLUDE_SIMULATOR_AARCH32
class ExecutableMemory {
 public:
  ExecutableMemory(const byte* code_start, size_t size)
      : size_(size),
        buffer_(reinterpret_cast<byte*>(mmap(NULL,
                                             size,
                                             PROT_READ | PROT_WRITE | PROT_EXEC,
                                             MAP_SHARED | MAP_ANONYMOUS,
                                             -1,
                                             0))) {
    VIXL_ASSERT(reinterpret_cast<intptr_t>(buffer_) != -1);
    memcpy(buffer_, code_start, size_);
    __builtin___clear_cache(buffer_, buffer_ + size_);
  }
  ~ExecutableMemory() { munmap(buffer_, size_); }

  template <typename T>
  T GetEntryPoint(const Label& entry_point, InstructionSet isa) const {
    int32_t location = entry_point.GetLocation();
    if (isa == T32) location += 1;
    return GetOffsetAddress<T>(location);
  }

 protected:
  template <typename T>
  T GetOffsetAddress(int32_t offset) const {
    VIXL_ASSERT((offset >= 0) && (static_cast<size_t>(offset) <= size_));
    T function_address;
    byte* buffer_address = buffer_ + offset;
    memcpy(&function_address, &buffer_address, sizeof(T));
    return function_address;
  }

 private:
  size_t size_;
  byte* buffer_;
};
#endif

// Generate a function with the following prototype:
//   int32_t abs(int32_t x)
//
// The generated function computes the absolute value of an integer.
void GenerateAbs(vixl::aarch32::MacroAssembler* masm);

// Generate a function with the following prototype:
//   uint32_t demo_function(uint32_t x)
//
// This is the example used in doc/getting-started-aarch32.md
void GenerateDemo(vixl::aarch32::MacroAssembler* masm);

#endif  // VIXL_EXAMPLE_EXAMPLES_H_
