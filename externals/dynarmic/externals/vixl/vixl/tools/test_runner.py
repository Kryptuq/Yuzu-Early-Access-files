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

import re
import subprocess

from known_test_failures import FilterKnownTestFailures
import printer
import threaded_tests
from threaded_tests import Test
import util

# Scan matching tests and return a test manifest.
def GetTests(runner, filters = []):
  cmd = runner +  ' --list'
  rc, output = util.getstatusoutput(cmd)
  if rc != 0: util.abort("Failed to list all tests. Output of " + cmd + ":\n" + output)

  tests = output.split()
  for f in filters:
    tests = filter(re.compile(f).search, tests)

  return tests

def RunTest(test):
  command = test.args['command']
  p = subprocess.Popen(command,
                       stdout=subprocess.PIPE,
                       stderr=subprocess.STDOUT)
  p_out, p_err = p.communicate()
  rc = p.poll()

  if rc == 0:
    skipped = False
    lines = p_out.split('\n')
    skipped_id = "SKIPPED: "
    for i in range(len(lines)):
      if lines[i].startswith(skipped_id):
        skipped = True
        reason = lines[i][len(skipped_id):]
        with Test.n_tests_skipped.get_lock():
          Test.n_tests_skipped.value += 1
          test.shared.tests_skipped.setdefault(reason, 0)
          test.shared.tests_skipped[reason] += 1
        break
    if not skipped:
      with Test.n_tests_passed.get_lock(): Test.n_tests_passed.value += 1
  else:
    with Test.n_tests_failed.get_lock(): Test.n_tests_failed.value += 1

  printer.__print_lock__.acquire()

  printer.UpdateProgress(test.shared.start_time,
                         Test.n_tests_passed.value,
                         Test.n_tests_failed.value,
                         test.shared.n_tests,
                         Test.n_tests_skipped.value,
                         test.shared.n_known_failures,
                         test.name,
                         prevent_next_overwrite = (rc != 0),
                         has_lock = True,
                         prefix = test.shared.progress_prefix)

  if rc != 0:
    printer.Print('FAILED: ' + test.name, has_lock = True)
    printer.Print(printer.COLOUR_RED + ' '.join(command) + printer.NO_COLOUR,
                  has_lock = True)
    printer.Print(p_out, has_lock = True)

  printer.__print_lock__.release()

class TestQueue(threaded_tests.TestQueue):
  def __init__(self):
    super(TestQueue, self).__init__('test_runner: ')

  def AddTests(self, test_runner_command, filters, runtime_options, under_valgrind):
    tests = GetTests(test_runner_command, filters)
    n_tests_total = len(tests)
    tests, skipped  = FilterKnownTestFailures(tests, under_valgrind = under_valgrind)
    for n_tests, reason in skipped:
      if n_tests > 0:
          self.AddKnownFailures(reason, n_tests)

    if len(tests) == 0:
      printer.Print('No tests to run.')
      return

    base_command = []
    if under_valgrind:
      base_command += ['valgrind']
    base_command += [test_runner_command]
    for test in tests:
      command = base_command + [test] + runtime_options
      self.AddTest(test, command = command)

  def Run(self, jobs, verbose):
    return super(TestQueue, self).Run(jobs, verbose, RunTest)
