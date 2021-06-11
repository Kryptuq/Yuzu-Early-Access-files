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

import collections
import multiprocessing
import re
import time

import printer
import thread_pool

REGEXP_MISSING_FEATURES = "Missing features: { ([^,}]+(, [^,}]+)*) }"

class Test(object):
  # Shared state for multiprocessing. Ideally the context should be passed with
  # arguments, but constraints from the multiprocessing module prevent us from
  # doing so: the shared variables (multiprocessing.Value) must be either global
  # or static, or no work is started.
  n_tests_passed = multiprocessing.Value('i', 0)
  n_tests_failed = multiprocessing.Value('i', 0)
  n_tests_skipped = multiprocessing.Value('i', 0)
  manager = multiprocessing.Manager()

  def __init__(self, name, shared, **kwargs):
      self.name = name
      self.shared = shared
      self.args = kwargs

class TestQueue(object):
  def __init__(self, prefix = ''):
    self.progress_prefix = prefix
    self.queue = []
    self.tests_skipped = Test.manager.dict()
    self.n_known_failures = 0
    self.known_failures = collections.Counter()

  def AddKnownFailures(self, reason, n_tests):
      self.n_known_failures += n_tests
      self.known_failures[reason] += n_tests

  def AddTest(self, name, **kwargs):
    self.queue.append(Test(name, self, **kwargs))

  # Run the specified tests.
  def Run(self, jobs, verbose, run_function):
    def InitGlobals():
      # Initialisation.
      self.start_time = time.time()
      self.n_tests = len(self.queue)
      if self.n_tests == 0:
        printer.Print('No tests to run.')
        return False
      Test.n_tests_passed.value = 0
      Test.n_tests_failed.value = 0
      Test.n_tests_skipped.value = 0
      self.tests_skipped.clear()
      return True

    thread_pool.Multithread(run_function, self.queue, jobs, InitGlobals)

    printer.UpdateProgress(self.start_time,
                           Test.n_tests_passed.value,
                           Test.n_tests_failed.value,
                           self.n_tests,
                           Test.n_tests_skipped.value,
                           self.n_known_failures,
                           '== Done ==',
                           prevent_next_overwrite = True,
                           prefix = self.progress_prefix)
    n_tests_features = 0
    features = set()
    for reason, n_tests in self.tests_skipped.items():
      m = re.match(REGEXP_MISSING_FEATURES, reason)
      if m:
        if verbose:
          printer.Print("%d tests skipped because the following features are not "
                        "available '%s'" % (n_tests, m.group(1)))
        else:
          n_tests_features += n_tests
          features.update(m.group(1).split(', '))
      else:
        printer.Print("%d tests skipped because '%s'" % (n_tests, reason))

    n_tests_other = 0
    if n_tests_features > 0 :
      printer.Print("%d tests skipped because the CPU does not support "
                    "the following features: '%s'" %
                    (n_tests_features, ", ".join(features)))

    for reason, n_tests in self.known_failures.items():
        printer.Print("%d tests skipped because '%s'" % (n_tests, reason))

    # Empty the queue now that the tests have been run.
    self.queue = []
    # `0` indicates success
    return Test.n_tests_failed.value

