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

import argparse
import multiprocessing
import os
import re
import subprocess
import sys
import tempfile

from threaded_tests import Test, TestQueue
import printer
import util

CLANG_FORMAT_VERSION_MAJOR = 4
CLANG_FORMAT_VERSION_MINOR = 0

DEFAULT_CLANG_FORMAT = \
    'clang-format-{}.{}'.format(CLANG_FORMAT_VERSION_MAJOR,
                                CLANG_FORMAT_VERSION_MINOR)

is_output_redirected = not sys.stdout.isatty()

def BuildOptions():
  parser = argparse.ArgumentParser(
    description = '''This tool runs `clang-format` on C++ files.
    If no files are provided on the command-line, all C++ source files are
    processed, except for the test traces.
    When available, `colordiff` is automatically used to colour the output.''',
    # Print default values.
    formatter_class = argparse.ArgumentDefaultsHelpFormatter)
  parser.add_argument('files', nargs = '*')
  parser.add_argument('--clang-format', default=DEFAULT_CLANG_FORMAT,
                      help='Path to clang-format.')
  parser.add_argument('--in-place', '-i',
                      action = 'store_true', default = False,
                      help = 'Edit files in place.')
  parser.add_argument('--jobs', '-j', metavar = 'N', type = int, nargs = '?',
                      default = multiprocessing.cpu_count(),
                      const = multiprocessing.cpu_count(),
                      help = '''Runs the tests using N jobs. If the option is set
                      but no value is provided, the script will use as many jobs
                      as it thinks useful.''')
  return parser.parse_args()


def ClangFormatIsAvailable(clang_format):
  if not util.IsCommandAvailable(clang_format):
    return False
  cmd = '%s -version' % clang_format
  rc, version = util.getstatusoutput(cmd)
  if rc != 0:
      util.abort("Failed to execute %s: %s" % (cmd, version))
  m = re.search("^clang-format version (\d)\.(\d)\.\d.*$",
                version.decode(), re.M)
  if not m:
      util.abort("Failed to get clang-format's version: %s" % version)
  major, minor = m.groups()
  return int(major) == CLANG_FORMAT_VERSION_MAJOR and \
      int(minor) == CLANG_FORMAT_VERSION_MINOR

def RunTest(test):
  filename = test.args['filename']
  clang_format = test.args['clang_format']
  in_place = test.args['in_place']

  rc = 0

  cmd_format = [clang_format, filename]
  temp_file, temp_file_name = tempfile.mkstemp(prefix = 'clang_format_')
  cmd_format_string = '$ ' + ' '.join(cmd_format) + ' > %s' % temp_file_name
  p_format = subprocess.Popen(cmd_format,
                              stdout = temp_file, stderr = subprocess.STDOUT)

  rc += p_format.wait()

  cmd_diff = ['diff', '--unified', filename, temp_file_name]
  cmd_diff_string = '$ ' + ' '.join(cmd_diff)
  p_diff = subprocess.Popen(cmd_diff,
                            stdout = subprocess.PIPE, stderr = subprocess.STDOUT)

  if util.IsCommandAvailable('colordiff') and not is_output_redirected:
    p_colordiff = subprocess.Popen(
            ['colordiff', '--unified'],
            stdin = p_diff.stdout,
            stdout = subprocess.PIPE, stderr = subprocess.STDOUT)
    out, unused = p_colordiff.communicate()
  else:
    out, unused = p_diff.communicate()

  rc += p_diff.wait()

  if in_place:
      cmd_format = [clang_format, '-i', filename]
      p_format = subprocess.Popen(cmd_format,
                                  stdout=temp_file, stderr=subprocess.STDOUT)

  if rc != 0:
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
                         prevent_next_overwrite = rc != 0,
                         has_lock = True,
                         prefix = test.shared.progress_prefix)

  if rc != 0:
    printer.Print('Incorrectly formatted file: ' + filename + '\n' + \
                  cmd_format_string + '\n' + \
                  cmd_diff_string + '\n' + \
                  out, has_lock = True)
  printer.__print_lock__.release()

  os.remove(temp_file_name)

# Returns the total number of files incorrectly formatted.
def ClangFormatFiles(files, clang_format, in_place = False, jobs = 1,
                     progress_prefix = ''):
  if not ClangFormatIsAvailable(clang_format):
    error_message = "`{}` version {}.{} not found. Please ensure it " \
                    "is installed, in your PATH and the correct version." \
                    .format(clang_format,
                            CLANG_FORMAT_VERSION_MAJOR,
                            CLANG_FORMAT_VERSION_MINOR)
    print(printer.COLOUR_RED + error_message + printer.NO_COLOUR)
    return -1

  queue = TestQueue(prefix = progress_prefix)
  for f in files:
    queue.AddTest(f, filename = f, clang_format = clang_format, in_place = in_place)

  rc = queue.Run(jobs, True, RunTest)

  printer.PrintOverwritableLine(
      progress_prefix + '%d files are incorrectly formatted.' % rc,
      type = printer.LINE_TYPE_LINTER)
  printer.EnsureNewLine()

  return rc

if __name__ == '__main__':
  # Parse the arguments.
  args = BuildOptions()
  files = args.files or util.get_source_files()

  rc = ClangFormatFiles(files, clang_format = args.clang_format,
                        in_place = args.in_place, jobs = args.jobs)

  sys.exit(rc)
