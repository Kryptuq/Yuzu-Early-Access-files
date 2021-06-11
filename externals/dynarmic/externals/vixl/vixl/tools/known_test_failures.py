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

import re

import util

def FilterKnownValgrindTestFailures(tests):
  rc, output = util.getstatusoutput('valgrind --version')

  if rc != 0:
    util.abort('Failed to get the Valgrind version.')

  version = re.search('^valgrind-([0-9]+)\.([0-9]+)\.([0-9]+)', output)

  if not version:
    util.abort('Failed to get the Valgrind version.')

  major = int(version.group(1))
  minor = int(version.group(2))

  if major > 3 or (major == 3 and minor > 10):
    return tests

  reason = "Valgrind versions before 3.11 have issues with fused multiply-add, " \
           "so disable the affected tests."
  known_valgrind_test_failures = {
    'AARCH64_SIM_fmadd_d',
    'AARCH64_SIM_fmadd_s',
    'AARCH64_SIM_fmla_2D',
    'AARCH64_SIM_fmla_2D_2D_D',
    'AARCH64_SIM_fmla_2S',
    'AARCH64_SIM_fmla_2S_2S_S',
    'AARCH64_SIM_fmla_4S',
    'AARCH64_SIM_fmla_4S_4S_S',
    'AARCH64_SIM_fmla_D_D_D',
    'AARCH64_SIM_fmls_2D',
    'AARCH64_SIM_fmls_2D_2D_D',
    'AARCH64_SIM_fmls_2S',
    'AARCH64_SIM_fmls_2S_2S_S',
    'AARCH64_SIM_fmls_4S',
    'AARCH64_SIM_fmls_4S_4S_S',
    'AARCH64_SIM_fmls_D_D_D',
    'AARCH64_SIM_fmsub_d',
    'AARCH64_SIM_fmsub_s',
    'AARCH64_SIM_fnmadd_d',
    'AARCH64_SIM_fnmadd_s',
    'AARCH64_SIM_fnmsub_d',
    'AARCH64_SIM_fnmsub_s',
    'AARCH64_SIM_frecps_2D',
    'AARCH64_SIM_frecps_D',
    'AARCH64_SIM_frsqrts_2D',
    'AARCH64_SIM_frsqrts_D'
  }

  filtered_list = filter(lambda x: x not in known_valgrind_test_failures, tests)
  return (filtered_list, len(tests) - len(filtered_list), reason)

def FilterKnownTestFailures(tests, **env):
  skipped = []
  if env.get('under_valgrind'):
    tests, n_tests_skipped, reason = FilterKnownValgrindTestFailures(tests)
    skipped.append( (n_tests_skipped, reason) )

  return (tests, skipped)
