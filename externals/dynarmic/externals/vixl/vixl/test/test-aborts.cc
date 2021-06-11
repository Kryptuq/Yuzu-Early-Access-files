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

#include <stdlib.h>

#include "globals-vixl.h"
#include "test-runner.h"

// These are all negative tests to check that the aborts work and print sensible
// messages. These tests cannot check the aborts with negative_testing=off.

#ifdef VIXL_NEGATIVE_TESTING
#include <stdexcept>

namespace vixl {

#define TEST(name, code, expected_prefix)                            \
  TEST_(ABORTS_##name) {                                             \
    try {                                                            \
      code;                                                          \
      printf("\n%s:%d\nNo exception raised.\n", __FILE__, __LINE__); \
      abort();                                                       \
    } catch (const std::runtime_error& e) {                          \
      size_t prefix_length = strlen(expected_prefix);                \
      if (strncmp(expected_prefix, e.what(), prefix_length) != 0) {  \
        printf("\n%s:%d\nFound:\n%sExpected:\n%s...\n",              \
               __FILE__,                                             \
               __LINE__,                                             \
               e.what(),                                             \
               expected_prefix);                                     \
        abort();                                                     \
      }                                                              \
    }                                                                \
  }

TEST(abort, VIXL_ABORT(), "Aborting in ")
TEST(abort_with_msg, VIXL_ABORT_WITH_MSG("message\n"), "message\nin ")
TEST(check_simple, VIXL_CHECK(false), "Assertion failed (false)\nin ")
TEST(check_expression, VIXL_CHECK(1 == 2), "Assertion failed (1 == 2)\nin ")
#ifdef VIXL_DEBUG
TEST(unimplemented, VIXL_UNIMPLEMENTED(), "UNIMPLEMENTED in ")
TEST(unreachable, VIXL_UNREACHABLE(), "UNREACHABLE in ")
TEST(assert, VIXL_ASSERT(false), "Assertion failed (false)\nin ")
#endif

}  // namespace vixl

#endif  // VIXL_NEGATIVE_TESTING
