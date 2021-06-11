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


#include "code-buffer-vixl.h"
#include "test-runner.h"

namespace vixl {

#define TEST(name) TEST_(CODE_BUFFER_##name)

TEST(align_grow) {
  CodeBuffer code_buffer(2);
  VIXL_CHECK(code_buffer.GetCapacity() == 2);
  VIXL_CHECK(code_buffer.GetRemainingBytes() == 2);

  code_buffer.Emit16(0);
  VIXL_CHECK(code_buffer.GetCapacity() == 2);
  VIXL_CHECK(code_buffer.GetRemainingBytes() == 0);

  // Check that the buffer can automatically grow when aligning the cursor.
  VIXL_CHECK(IsAligned<2>(code_buffer.GetCursorOffset()));
  code_buffer.Align();
  VIXL_CHECK(IsWordAligned(code_buffer.GetCursorOffset()));
  VIXL_CHECK(code_buffer.GetCapacity() > 2);

  code_buffer.SetClean();
}

static void TestDefaultsHelper(const CodeBuffer& buffer) {
  VIXL_CHECK(buffer.GetCapacity() == CodeBuffer::kDefaultCapacity);
  VIXL_CHECK(buffer.HasSpaceFor(CodeBuffer::kDefaultCapacity));
  VIXL_CHECK(!buffer.HasSpaceFor(CodeBuffer::kDefaultCapacity + 1));
  VIXL_CHECK(buffer.GetCursorOffset() == 0);
  VIXL_CHECK(buffer.GetOffsetFrom(0) == 0);
  VIXL_CHECK(buffer.IsManaged());
  VIXL_CHECK(!buffer.IsDirty());
  VIXL_CHECK(buffer.GetRemainingBytes() == CodeBuffer::kDefaultCapacity);
  VIXL_CHECK(buffer.GetSizeInBytes() == 0);
  VIXL_CHECK(buffer.GetEndAddress<uintptr_t>() ==
             buffer.GetStartAddress<uintptr_t>());
}

TEST(defaults) {
  CodeBuffer buffer;
  TestDefaultsHelper(buffer);
}

TEST(reset) {
  CodeBuffer buffer;
  // Update the buffer by writing to it.
  buffer.Emit("placeholder data");
  VIXL_CHECK(buffer.IsDirty());
  VIXL_CHECK(buffer.GetSizeInBytes() > 0);
  // Calling Reset() should reset it back to its default state. (It does not
  // shrink the capacity, but it should not have grown here.)
  VIXL_ASSERT(buffer.GetCapacity() == CodeBuffer::kDefaultCapacity);
  buffer.Reset();
  TestDefaultsHelper(buffer);
}

TEST(ensure_space) {
  const size_t initial_capacity = 1234;
  CodeBuffer buffer(initial_capacity);

  // Requesting less space than we already have should have no effect.
  for (size_t space = 0; space < initial_capacity; space++) {
    buffer.EnsureSpaceFor(space);
    VIXL_CHECK(buffer.GetCapacity() == initial_capacity);
  }

  // Requesting more memory grows the buffer by an unspecified amount.
  buffer.EnsureSpaceFor(initial_capacity + 1);
  VIXL_CHECK(buffer.GetCapacity() > initial_capacity);
}

TEST(emit) {
  CodeBuffer buffer;
  VIXL_ASSERT(buffer.GetSizeInBytes() == 0);

  uint64_t base_value = 0x0100001000100101;
  const char* test_string = "test string";
  size_t expected_size = 0;

  // Simple emissions. This should not align or pad in any way.
  buffer.EmitData(&base_value, 7);
  expected_size += 7;

  buffer.EmitString(test_string);
  expected_size += strlen(test_string) + 1;  // EmitString() emits the '\0'.

  buffer.Emit64(static_cast<uint64_t>(base_value * 1));
  buffer.Emit(static_cast<uint64_t>(base_value * 2));
  expected_size += 16;

  buffer.Emit32(static_cast<uint32_t>(base_value * 3));
  buffer.Emit(static_cast<uint32_t>(base_value * 4));
  expected_size += 8;

  buffer.Emit16(static_cast<uint16_t>(base_value * 5));
  buffer.Emit(static_cast<uint16_t>(base_value * 6));
  expected_size += 4;

  buffer.Emit8(static_cast<uint8_t>(base_value * 7));
  buffer.Emit(static_cast<uint8_t>(base_value * 8));
  expected_size += 2;

  VIXL_CHECK(buffer.GetSizeInBytes() == expected_size);

  buffer.SetClean();

  // clang-format off
  uint8_t expected[] = {
    // EmitData
    0x01, 0x01, 0x10, 0x00, 0x10, 0x00, 0x00,
    // EmitString
    't', 'e', 's', 't', ' ', 's', 't', 'r', 'i', 'n', 'g', '\0',
    // Emit64
    0x01, 0x01, 0x10, 0x00, 0x10, 0x00, 0x00, 0x01,
    // Emit<uint64_t>
    0x02, 0x02, 0x20, 0x00, 0x20, 0x00, 0x00, 0x02,
    // Emit32
    0x03, 0x03, 0x30, 0x00,
    // Emit<uint32_t>
    0x04, 0x04, 0x40, 0x00,
    // Emit16
    0x05, 0x05,
    // Emit<uint16_t>
    0x06, 0x06,
    // Emit8
    0x07,
    // Emit<uint8_t>
    0x08
  };
  // clang-format on

  VIXL_ASSERT(expected_size == sizeof(expected));
  VIXL_CHECK(memcmp(buffer.GetStartAddress<const void*>(),
                    expected,
                    expected_size) == 0);
}

}  // namespace vixl
