// Copyright 2019, VIXL authors
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

#ifndef VIXL_AARCH64_BENCH_UTILS_H_
#define VIXL_AARCH64_BENCH_UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include <list>
#include <vector>

#include "globals-vixl.h"
#include "aarch64/macro-assembler-aarch64.h"

class BenchTimer {
 public:
  BenchTimer() { gettimeofday(&start_, NULL); }

  double GetElapsedSeconds() const {
    timeval elapsed = GetElapsed();
    double sec = elapsed.tv_sec;
    double usec = elapsed.tv_usec;
    return sec + (usec / 1000000.0);
  }

  bool HasRunFor(uint32_t seconds) {
    timeval elapsed = GetElapsed();
    VIXL_ASSERT(elapsed.tv_sec >= 0);
    return static_cast<uint64_t>(elapsed.tv_sec) >= seconds;
  }

 private:
  timeval GetElapsed() const {
    VIXL_ASSERT(timerisset(&start_));
    timeval now, elapsed;
    gettimeofday(&now, NULL);
    timersub(&now, &start_, &elapsed);
    return elapsed;
  }

  timeval start_;
};

// Provide a standard command-line interface for all benchmarks.
class BenchCLI {
 public:
  // Set default values.
  BenchCLI(int argc, char* argv[])
      : run_time_(kDefaultRunTime), status_(kRunBenchmark) {
    for (int i = 1; i < argc; i++) {
      if ((strcmp(argv[i], "-h") == 0) || (strcmp(argv[i], "--help") == 0)) {
        PrintUsage(argv[0]);
        status_ = kExitSuccess;
        return;
      }
    }

    // Use the default run time.
    if (argc == 1) return;

    if (argc != 2) {
      if (argc > 0) PrintUsage(argv[0]);
      status_ = kExitFailure;
      return;
    }

    char* end;
    unsigned long run_time = strtoul(argv[1], &end, 0);  // NOLINT(runtime/int)
    if ((end == argv[1]) || (run_time > UINT32_MAX)) {
      PrintUsage(argv[0]);
      status_ = kExitFailure;
      return;
    }
    run_time_ = static_cast<uint32_t>(run_time);
  }

  void PrintUsage(char* name) {
    printf("USAGE: %s [OPTIONS]... [RUN_TIME]\n", name);
    printf("\n");
    printf("Run a single VIXL benchmark for approximately RUN_TIME seconds,\n");
    printf("or %" PRIu32 " seconds if unspecified.\n", kDefaultRunTime);
    printf("\n");
#ifdef VIXL_DEBUG
    printf("This is a DEBUG build. VIXL's assertions will be enabled, and\n");
    printf("extra debug information may be printed. The benchmark results\n");
    printf("are not representative of expected VIXL deployments.\n");
    printf("\n");
#endif
    printf("OPTIONS:\n");
    printf("\n");
    printf("    -h, --help\n");
    printf("        Print this help message.\n");
  }

  void PrintResults(uint64_t iterations, double elapsed_seconds) {
    double score = iterations / elapsed_seconds;
    printf("%g iteration%s per second (%" PRIu64 " / %g)",
           score,
           (score == 1.0) ? "" : "s",
           iterations,
           elapsed_seconds);
#ifdef VIXL_DEBUG
    printf(" [Warning: DEBUG build]");
#endif
    printf("\n");
  }

  bool ShouldExitEarly() const {
    switch (status_) {
      case kRunBenchmark:
        return false;
      case kExitFailure:
      case kExitSuccess:
        return true;
    }
    VIXL_UNREACHABLE();
    return true;
  }

  int GetExitCode() const {
    switch (status_) {
      case kExitFailure:
        return EXIT_FAILURE;
      case kExitSuccess:
      case kRunBenchmark:
        return EXIT_SUCCESS;
    }
    VIXL_UNREACHABLE();
    return EXIT_FAILURE;
  }

  uint32_t GetRunTimeInSeconds() const { return run_time_; }

 private:
  static const uint32_t kDefaultRunTime = 5;

  uint32_t run_time_;

  enum { kRunBenchmark, kExitSuccess, kExitFailure } status_;
};

// Generate random, but valid (and simulatable) instruction sequences.
//
// The effect of the generated code is meaningless, but not harmful. That is,
// it will not abort, callee-saved registers are properly preserved and so on.
// It is possible to call it as a `void fn(void)` function.
class BenchCodeGenerator {
 public:
  explicit BenchCodeGenerator(vixl::aarch64::MacroAssembler* masm)
      : masm_(masm), rnd_(0), rnd_bits_(0), call_depth_(0) {
    // Arbitrarily initialise rand_state_ using the behaviour of srand48(42).
    rand_state_[2] = 0;
    rand_state_[1] = 42;
    rand_state_[0] = 0x330e;
  }

  void Generate(size_t min_size_in_bytes);

 private:
  void GeneratePrologue();
  void GenerateEpilogue();

  // Arbitrarily pick one of the other Generate*Sequence() functions.
  // TODO: Consider allowing this to be biased, so that a benchmark can focus on
  // a subset of sequences.
  void GenerateArbitrarySequence();

  // Instructions with a trivial pass-through to Emit().
  void GenerateTrivialSequence();

  // Instructions using the Operand and MemOperand abstractions. These have a
  // run-time cost, and many common VIXL APIs use them.
  void GenerateOperandSequence();
  void GenerateMemOperandSequence();

  // Generate instructions taking immediates that require analysis (and may
  // result in multiple instructions per macro).
  void GenerateImmediateSequence();

  // Immediate-offset and register branches. This also (necessarily) covers adr.
  void GenerateBranchSequence();

  // Generate nested, conventional (blr+ret) calls.
  void GenerateCallReturnSequence();

  void GenerateFPSequence();
  void GenerateNEONSequence();

  // To exercise veneer pools, GenerateBranchSequence links labels that are
  // expected to be bound later. This helper binds them.
  // The Nth youngest label is bound if bit <N> is set in `bind_mask`. That
  // means that this helper can bind at most 64 pending labels.
  void BindPendingLabels(uint64_t bind_mask);

  // As above, but unconditionally bind all pending labels (even if there are
  // more than 64 of them).
  void BindAllPendingLabels();

  // Argument selection helpers. These only return caller-saved registers.

  uint64_t GetRandomBits(int bits);
  bool PickBool() { return GetRandomBits(1) != 0; }

  unsigned PickRSize();
  unsigned PickFPSize();

  vixl::aarch64::Register PickR(unsigned size_in_bits);
  vixl::aarch64::VRegister PickV(
      unsigned size_in_bits = vixl::aarch64::kQRegSize);

  vixl::aarch64::Register PickW() { return PickR(vixl::aarch64::kWRegSize); }
  vixl::aarch64::Register PickX() { return PickR(vixl::aarch64::kXRegSize); }
  vixl::aarch64::VRegister PickH() { return PickV(vixl::aarch64::kHRegSize); }
  vixl::aarch64::VRegister PickS() { return PickV(vixl::aarch64::kSRegSize); }
  vixl::aarch64::VRegister PickD() { return PickV(vixl::aarch64::kDRegSize); }

  vixl::aarch64::MacroAssembler* masm_;

  // State for *rand48(), used to randomise code generation.
  unsigned short rand_state_[3];  // NOLINT(runtime/int)

  uint32_t rnd_;
  int rnd_bits_;

  // The generator can produce nested calls. The probability of it doing this is
  // influenced by the current call depth, so we have to track it here.
  int call_depth_;

  struct LabelPair {
    // We can't copy labels, so we have to allocate them dynamically to store
    // them in a std::list.
    vixl::aarch64::Label* target;
    vixl::aarch64::Label* cont;
  };
  std::list<LabelPair> labels_;

  // Some sequences need a scratch area. Space for this is allocated on the
  // stack, and stored in this register.
  static const vixl::aarch64::Register scratch;
};

#endif  // VIXL_AARCH64_BENCH_UTILS_H_
