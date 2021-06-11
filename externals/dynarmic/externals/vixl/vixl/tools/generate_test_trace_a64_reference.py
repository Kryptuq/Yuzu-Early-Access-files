#!/usr/bin/env python2.7

# Copyright 2016, VIXL authors
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


def BuildOptions(root):
  result = argparse.ArgumentParser(
      description = 'Generate reference output for TRACE_* tests')
  result.add_argument('--runner', action='store',
                      default=os.path.join(root, 'obj/latest/test/test-runner'),
                      help='The test executable to run.')
  result.add_argument('--outdir', action='store',
                      default='test/test-trace-reference/')
  return result.parse_args()

if __name__ == '__main__':
  root_dir = os.path.dirname(os.path.dirname(os.path.abspath(sys.argv[0])))
  os.chdir(root_dir)

  args = BuildOptions(root_dir)

  # Run each trace test (TRACE_*) with the --generate_test_trace option, and
  # use the output to create the reference traces (in --outdir).

  # Find the trace tests.
  status, output = util.getstatusoutput(args.runner + ' --list')
  if status != 0: util.abort('Failed to list all tests')
  tests = filter(lambda t: 'TRACE_' in t, output.split())
  tests.sort()

  if not os.path.exists(args.outdir):
    os.makedirs(args.outdir)

  for test in tests:
    # Run each test.
    print 'Generating trace for ' + test;
    cmd = ' '.join([args.runner, '--generate_test_trace', test])
    status, output = util.getstatusoutput(cmd)
    if status != 0: util.abort('Failed to run ' + cmd + '.')

    # Create a new trace header file.
    trace_filename = 'log-' + test.replace('TRACE_', '').lower().replace('_', '-')
    trace_f =  open(os.path.join(args.outdir, trace_filename), 'w')
    trace_f.write(output)
    # `getstatusoutput` removes the trailing newline. Put it back, but only if
    # the output was not empty.
    if len(output) > 0: trace_f.write('\n')
