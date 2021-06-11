#!/usr/bin/env python2.7

# Copyright 2019, VIXL authors
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
import argparse
import multiprocessing
import os
import re
import sys

from clang_format import CLANG_FORMAT_VERSION_MAJOR, CLANG_FORMAT_VERSION_MINOR
from threaded_tests import Test, TestQueue
import config
import printer
import util

CLANG_TIDY_VERSION_MAJOR = CLANG_FORMAT_VERSION_MAJOR
CLANG_TIDY_VERSION_MINOR = CLANG_FORMAT_VERSION_MINOR

DEFAULT_CLANG_TIDY = \
    'clang-tidy-{}.{}'.format(CLANG_TIDY_VERSION_MAJOR,
                              CLANG_TIDY_VERSION_MINOR)

def BuildOptions():
  parser = argparse.ArgumentParser(
    description = '''This tool runs `clang-tidy` on C++ files.''',
    # Print default values.
    formatter_class = argparse.ArgumentDefaultsHelpFormatter)
  parser.add_argument('files', nargs='*')
  parser.add_argument('--jobs', '-j', metavar='N', type=int, nargs='?',
                      default=multiprocessing.cpu_count(),
                      const=multiprocessing.cpu_count(),
                      help='''Runs the tests using N jobs. If the option is set
                      but no value is provided, the script will use as many jobs
                      as it thinks useful.''')
  parser.add_argument('--clang-tidy', default=DEFAULT_CLANG_TIDY,
                      help='Path to clang-tidy.')
  return parser.parse_args()

def ClangTidyIsAvailable(clang_tidy):
  if not util.IsCommandAvailable(clang_tidy):
    return False
  cmd = '%s -version' % clang_tidy
  rc, version = util.getstatusoutput(cmd)
  if rc != 0:
    util.abort("Failed to execute %s: %s" % (cmd, version))
  m = re.search("LLVM version (\d)\.(\d)\.\d.*$", version.decode(), re.M)
  if not m:
    util.abort("Failed to get clang-tidy's version: %s" % version)
  major, minor = m.groups()
  return int(major) == CLANG_TIDY_VERSION_MAJOR and \
    int(minor) == CLANG_TIDY_VERSION_MINOR

def FilterClangTidyLines(lines):
  out = []
  print_context = False
  for line in lines:
    if ("Error: no checks enabled" in line) or ("USAGE" in line):
      # We've incorrectly invoked or configured clang-tidy. This should never
      # happen, but it if it does, make sure that the test fails.
      return line
    elif "error:" in line:
      out.append(line)
      print_context = True
    elif "warning:" in line:
      out.append(line)
      print_context = True
    elif print_context:
      out.append(line)
  return "\n".join(out)

def FilterFiles(list_files):
  return list(filter(lambda x: x.endswith('.cc'), list_files))

def RunTest(test):
  cmd = " ".join(test.args['command'])
  rc, p_out = util.getstatusoutput(cmd)
  if rc != 0:
    # This usually happens when the compiler hits a '#error' because of
    # a missing define.
    printer.Print("%sFATAL ERROR: failed to run command '%s': %s%s" %
                  (printer.COLOUR_RED, cmd, p_out, printer.NO_COLOUR))
  p_out = FilterClangTidyLines(p_out.split('\n'))

  failed = (len(p_out) > 0) or (rc != 0)

  if failed:
    with Test.n_tests_failed.get_lock(): Test.n_tests_failed.value += 1
  else:
    with Test.n_tests_passed.get_lock(): Test.n_tests_passed.value += 1

  printer.__print_lock__.acquire()

  printer.UpdateProgress(test.shared.start_time,
                         Test.n_tests_passed.value,
                         Test.n_tests_failed.value,
                         test.shared.n_tests,
                         Test.n_tests_skipped.value,
                         test.shared.n_known_failures,
                         test.name,
                         prevent_next_overwrite = failed ,
                         has_lock = True,
                         prefix = test.shared.progress_prefix)

  if failed:
    printer.Print(printer.COLOUR_RED + 'FAILED: '+ cmd + printer.NO_COLOUR,
                  has_lock = True)
    printer.Print(p_out, has_lock = True)

  printer.__print_lock__.release()

def ClangTidyFiles(files, clang_tidy, jobs = 1, progress_prefix = ''):
  if not ClangTidyIsAvailable(clang_tidy):
    error_message = "`{}` version {}.{} not found. Please ensure it " \
                    "is installed, in your PATH and the correct version." \
                    .format(clang_tidy,
                            CLANG_TIDY_VERSION_MAJOR,
                            CLANG_TIDY_VERSION_MINOR)
    print(printer.COLOUR_RED + error_message + printer.NO_COLOUR)
    return -1

  opts = ['--', '-DVIXL_INCLUDE_TARGET_AARCH64', '-DVIXL_CODE_BUFFER_MALLOC',
          '-DVIXL_DEBUG','-DVIXL_INCLUDE_SIMLUATOR_AARCH64',
          '-DVIXL_INCLUDE_TARGET_A32','-DVIXL_INCLUDE_TARGET_T32',
          '-DVIXL_INCLUDE_TARGET_A64']
  opts += ['-I%s' % config.dir_src_vixl]
  opts += ['-I%s' % config.dir_tests]
  opts += ['-I%s' % config.dir_aarch64_examples]
  opts += ['-I%s' % config.dir_aarch32_examples]

  to_check = FilterFiles(files)
  printer.Print("clang-tidy: %d files to check" % len(to_check))

  queue = TestQueue(prefix = progress_prefix)

  for file in to_check:
    for cpp_version in config.tested_cpp_standards:
      command = [clang_tidy, file] + opts + ['-std=%s' % cpp_version]
      queue.AddTest(file, command=command)

  return queue.Run(jobs, True, RunTest)

if __name__ == '__main__':
  # Parse the arguments.
  args = BuildOptions()
  files = args.files or util.get_source_files()

  rc = ClangTidyFiles(files, args.clang_tidy, args.jobs)

  sys.exit(rc)
