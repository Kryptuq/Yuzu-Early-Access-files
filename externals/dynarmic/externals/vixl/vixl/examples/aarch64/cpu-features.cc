// Copyright 2020, VIXL authors
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

#include "examples.h"

using namespace vixl;
using namespace vixl::aarch64;

// Demonstrate the use of VIXL's CPU feature detection, printing the features
// that VIXL detects.

#ifndef TEST_EXAMPLES
int main() {
  // Simple native deployments should initialise CPU features using
  // `InferFromOS()`. If not on an AArch64 host, this returns nothing.
  std::cout << "==== CPUFeatures::InferFromOS() ====\n";
  std::cout << CPUFeatures::InferFromOS() << "\n";

  // VIXL assumes support for FP, NEON and CRC32 by default. These features were
  // implemented before the CPUFeatures mechanism.
  std::cout << "==== CPUFeatures::AArch64LegacyBaseline() ====\n";
  std::cout << CPUFeatures::AArch64LegacyBaseline() << "\n";

  // Retrieve a list of all supported CPU features.
  std::cout << "==== CPUFeatures::All() ====\n";
  std::cout << CPUFeatures::All() << "\n";

  return 0;
}
#endif
