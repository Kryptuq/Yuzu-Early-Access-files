#!/usr/bin/env python2.7

# Copyright 2015, VIXL authors
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
#   * Redistributions of source code must retain the above copyright notice,
#     this list of conditions and the following disclaimer.
#   * Redistributions in binary form must reproduce the above copyright notice,
#     this list of conditions and the following disclaimer in the documentation
#     and/or other materials provided with the distribution.
#   * Neither the name of ARM Limited nor the names of its contributors may be
#     used to endorse or promote products derived from this software without
#     specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import os
import sys
import argparse
import re
import util

copyright_header = """// Copyright 2015, VIXL authors
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

"""

master_trace_header = """
// This file holds the expected results for the instructions tested by
// test-simulator-aarch64.
//
// If you update input lists in test-simulator-inputs-aarch64.h, or add a new
// test to test-simulator-aarch64.cc, please run
// tools/generate_simulator_traces.py on a reference platform to regenerate
// this file and trace files.
//

#ifndef VIXL_TEST_AARCH64_SIMULATOR_TRACES_AARCH64_H_
#define VIXL_TEST_AARCH64_SIMULATOR_TRACES_AARCH64_H_

extern "C" {
#include <stdint.h>
}

// To add a new simulator test to test-simulator-aarch64.cc, add placeholder array(s)
// below to build test-simulator-aarch64 for reference platform. Then, run
// tools/generate_simulator_traces.py on a reference platform to regenerate this
// file and traces files.

// ---------------------------------------------------------------------
// ADD DUMMY ARRAYS FOR NEW SIMULATOR TEST HERE.
// ---------------------------------------------------------------------
const uint64_t kExpected_placeholder_64[] = {0};
const size_t kExpectedCount_placeholder_64 = 0;

const uint32_t kExpected_placeholder_32[] = {0};
const size_t kExpectedCount_placeholder_32 = 0;

// ---------------------------------------------------------------------
// Simulator test trace output files.
// ---------------------------------------------------------------------
"""
master_trace_footer = """
#endif  // VIXL_TEST_AARCH64_SIMULATOR_TRACES_AARCH64_H_
"""

trace_header = """
// ---------------------------------------------------------------------
// This file is auto generated using tools/generate_simulator_traces.py.
//
// PLEASE DO NOT EDIT.
// ---------------------------------------------------------------------
"""

def BuildOptions(root):
  result = argparse.ArgumentParser(description = 'Simulator test generator.')
  result.add_argument('--runner', action='store',
                      default=os.path.join(root, 'obj/latest/test/test-runner'),
                      help='The test executable to run.')
  result.add_argument('--aarch32-only', action='store_true')
  result.add_argument('--aarch64-only', action='store_true')
  result.add_argument('--out', action='store',
                      default='test/aarch64/test-simulator-traces-aarch64.h')
  result.add_argument('--filter', action='store', help='Test regexp filter.')
  return result.parse_args()

def ShouldGenerateAArch32(args):
  return (not args.aarch32_only and not args.aarch64_only) or args.aarch32_only

def ShouldGenerateAArch64(args):
  return (not args.aarch32_only and not args.aarch64_only) or args.aarch64_only

def GetAArch32Filename(test):
  return test.lower().replace('_', '-') + '.h'

def GetAArch64Filename(test):
  return test.lower().replace('_', '-') + '-trace-aarch64.h'

if __name__ == '__main__':
  # $ROOT/tools/generate_simulator_traces.py
  root_dir = os.path.dirname(os.path.dirname(os.path.abspath(sys.argv[0])))
  os.chdir(root_dir)

  args = BuildOptions(root_dir)

  # List all tests.
  status, test_list = util.getstatusoutput(args.runner + ' --list')
  if status != 0: util.abort('Failed to list all tests')

  if ShouldGenerateAArch64(args):
    # Run each simulator test (AARCH64_SIM_*) with the --generate_test_trace
    # option, and use the output to create the traces header (from --out). In
    # addition, the test-simulator-traces-aarch64.h file, the master trace file,
    # which includes all other trace files is generated.

    # Create master trace file.
    master_trace_f = open(args.out, 'w')
    master_trace_f.write(copyright_header)
    master_trace_f.write(master_trace_header)
    master_trace_f.write('\n\n')

    # Find the AArch64 simulator tests.
    tests = sorted(filter(lambda t: 'AARCH64_SIM_' in t, test_list.split()),
                   key=lambda t: GetAArch64Filename(t))

    for test in tests:
      # Strip out 'AARCH64_' to get the name of the test.
      test_name = test[len('AARCH64_'):]
      trace_filename = GetAArch64Filename(test_name)
      if not args.filter or re.compile(args.filter).search(test):
        # Run each test.
        print 'Generating trace for ' + test;
        cmd = ' '.join([args.runner, '--generate_test_trace', test])
        status, output = util.getstatusoutput(cmd)
        if status != 0: util.abort('Failed to run ' + cmd + '.')

        # Create a new trace header file.
        trace_f =  open("test/aarch64/traces/" + trace_filename, 'w')
        trace_f.write(copyright_header)
        trace_f.write(trace_header)
        trace_f.write('\n')
        trace_f.write("#ifndef VIXL_" + test_name.upper() + "_TRACE_AARCH64_H_\n")
        trace_f.write("#define VIXL_" + test_name.upper() + "_TRACE_AARCH64_H_\n")
        trace_f.write('\n')
        trace_f.write(output)
        trace_f.write('\n')
        trace_f.write('\n' + "#endif  // VIXL_"
                      + test_name.upper() + "_TRACE_AARCH64_H_" + '\n')
        trace_f.close()

      # Update master trace file.
      master_trace_f.write(
          '#include \"aarch64/traces/' + trace_filename + '\"\n')

    # Close master trace file.
    master_trace_f.write(master_trace_footer)
    master_trace_f.close()

  if ShouldGenerateAArch32(args):
    # Run each test (AARCH32_{SIMULATOR,ASSEMBLER}_*) with the
    # --generate_test_trace option.

    # Find the AArch32 tests.
    tests = sorted(
        filter(
            lambda t: 'AARCH32_SIMULATOR_' in t or ('AARCH32_ASSEMBLER_' in t
                and not 'AARCH32_ASSEMBLER_NEGATIVE_' in t),
            test_list.split()),
        key=lambda t: GetAArch32Filename(t))
    if args.filter:
      tests = filter(re.compile(args.filter).search, tests)

    for test in tests:
      # Run each test.
      print 'Generating trace for ' + test;
      # Strip out 'AARCH32_' to get the name of the test.
      test_name = test[len('AARCH32_'):]

      # An "and" instruction will be called "and_" since we cannot clash with
      # the C++ operator. Rename "and_" to "and" to keep sane filenames.
      test_name = test_name.replace('and_', 'and')

      cmd = ' '.join([args.runner, '--generate_test_trace', test])
      status, output = util.getstatusoutput(cmd)
      if status != 0: util.abort('Failed to run ' + cmd + '.')

      # Create a new trace header file.
      trace_filename = GetAArch32Filename(test_name)
      trace_f =  open("test/aarch32/traces/" + trace_filename, 'w')
      trace_f.write(copyright_header)
      trace_f.write(trace_header)
      trace_f.write('\n')
      trace_f.write("#ifndef VIXL_" + test_name.upper() + "_H_\n")
      trace_f.write("#define VIXL_" + test_name.upper() + "_H_\n")
      trace_f.write('\n')
      trace_f.write(output)
      trace_f.write('\n')
      trace_f.write(
          '\n' + "#endif  // VIXL_" + test_name.upper() + "_H_" + '\n')
      trace_f.close()

  print 'Trace generation COMPLETE'
