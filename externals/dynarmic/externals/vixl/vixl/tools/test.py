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

import argparse
import fcntl
import git
import itertools
import multiprocessing
import os
from os.path import join
import platform
import subprocess
import sys
import time

import config
import clang_format
import clang_tidy
import lint
import printer
import test
import test_runner
import util


dir_root = config.dir_root


# Remove duplicates from a list
def RemoveDuplicates(values):
  # Convert the list into a set and back to list
  # as sets guarantee items are unique.
  return list(set(values))


# Custom argparse.Action to automatically add and handle an 'all' option.
# If no 'default' value is set, it will default to 'all.
# If accepted options are set using 'choices' then only these values will be
# allowed.
# If they're set using 'soft_choices' then 'all' will default to these values,
# but other values will also be accepted.
class AllChoiceAction(argparse.Action):

  # At least one option was set by the user.
  WasSetByUser = False

  def __init__(self, **kwargs):
    if 'choices' in kwargs:
      assert 'soft_choices' not in kwargs,\
          "Can't have both 'choices' and 'soft_choices' options"
      self.all_choices = list(kwargs['choices'])
      kwargs['choices'].append('all')
    else:
      self.all_choices = kwargs['soft_choices']
      kwargs['help'] += ' Supported values: {' + ','.join(
          ['all'] + self.all_choices) + '}'
      del kwargs['soft_choices']
    if 'default' not in kwargs:
      kwargs['default'] = self.all_choices
    super(AllChoiceAction, self).__init__(**kwargs)

  def __call__(self, parser, namespace, values, option_string=None):
    AllChoiceAction.WasSetByUser = True
    if 'all' in values:
      # Substitute 'all' by the actual values.
      values = self.all_choices + [value for value in values if value != 'all']

    setattr(namespace, self.dest, RemoveDuplicates(values))


def BuildOptions():
  args = argparse.ArgumentParser(
    description =
    '''This tool runs all tests matching the specified filters for multiple
    environment, build options, and runtime options configurations.''',
    # Print default values.
    formatter_class=argparse.ArgumentDefaultsHelpFormatter)

  args.add_argument('filters', metavar='filter', nargs='*',
                    help='Run tests matching all of the (regexp) filters.')

  # We automatically build the script options from the options to be tested.
  test_arguments = args.add_argument_group(
    'Test options',
    'These options indicate what should be tested')
  test_arguments.add_argument(
      '--negative_testing',
      help='Tests with negative testing enabled.',
      action='store_const',
      const='on',
      default='off')
  test_arguments.add_argument(
      '--compiler',
      help='Test for the specified compilers.',
      soft_choices=config.tested_compilers,
      action=AllChoiceAction,
      nargs="+")
  test_arguments.add_argument(
      '--mode',
      help='Test with the specified build modes.',
      choices=config.build_options_modes,
      action=AllChoiceAction,
      nargs="+")
  test_arguments.add_argument(
      '--std',
      help='Test with the specified C++ standard.',
      soft_choices=config.tested_cpp_standards,
      action=AllChoiceAction,
      nargs="+")
  test_arguments.add_argument(
      '--target',
      help='Test with the specified isa enabled.',
      soft_choices=config.build_options_target,
      action=AllChoiceAction,
      nargs="+")

  general_arguments = args.add_argument_group('General options')
  general_arguments.add_argument('--dry-run', action='store_true',
                                 help='''Don't actually build or run anything,
                                 but print the configurations that would be
                                 tested.''')
  general_arguments.add_argument('--verbose', action='store_true',
                                 help='''Print extra information.''')
  general_arguments.add_argument(
    '--jobs', '-j', metavar='N', type=int, nargs='?',
    default=multiprocessing.cpu_count(),
    const=multiprocessing.cpu_count(),
    help='''Runs the tests using N jobs. If the option is set but no value is
    provided, the script will use as many jobs as it thinks useful.''')
  general_arguments.add_argument('--clang-format',
                                 default=clang_format.DEFAULT_CLANG_FORMAT,
                                 help='Path to clang-format.')
  general_arguments.add_argument('--clang-tidy',
                                 default=clang_tidy.DEFAULT_CLANG_TIDY,
                                 help='Path to clang-tidy.')
  general_arguments.add_argument('--nobench', action='store_true',
                                 help='Do not run benchmarks.')
  general_arguments.add_argument('--nolint', action='store_true',
                                 help='Do not run the linter.')
  general_arguments.add_argument('--noclang-format', action='store_true',
                                 help='Do not run clang-format.')
  general_arguments.add_argument('--noclang-tidy', action='store_true',
                                 help='Do not run clang-tidy.')
  general_arguments.add_argument('--notest', action='store_true',
                                 help='Do not run tests.')
  general_arguments.add_argument('--fail-early', action='store_true',
                                 help='Exit as soon as a test fails.')
  general_arguments.add_argument(
    '--under_valgrind', action='store_true',
    help='''Run the test-runner commands under Valgrind.
            Note that a few tests are known to fail because of
            issues in Valgrind''')
  return args.parse_args()


def RunCommand(command, environment_options = None):
  # Create a copy of the environment. We do not want to pollute the environment
  # of future commands run.
  environment = os.environ.copy()

  printable_command = ''
  if environment_options:
    # Add the environment options to the environment:
    environment.update(environment_options)
    printable_command += ' ' + DictToString(environment_options) + ' '
  printable_command += ' '.join(command)

  printable_command_orange = \
    printer.COLOUR_ORANGE + printable_command + printer.NO_COLOUR
  printer.PrintOverwritableLine(printable_command_orange)
  sys.stdout.flush()

  # Start a process for the command.
  # Interleave `stderr` and `stdout`.
  p = subprocess.Popen(command,
                       stdout=subprocess.PIPE,
                       stderr=subprocess.STDOUT,
                       env=environment)

  # We want to be able to display a continuously updated 'work indicator' while
  # the process is running. Since the process can hang if the `stdout` pipe is
  # full, we need to pull from it regularly. We cannot do so via the
  # `readline()` function because it is blocking, and would thus cause the
  # indicator to not be updated properly. So use file control mechanisms
  # instead.
  indicator = ' (still working: %d seconds elapsed)'

  # Mark the process output as non-blocking.
  flags = fcntl.fcntl(p.stdout, fcntl.F_GETFL)
  fcntl.fcntl(p.stdout, fcntl.F_SETFL, flags | os.O_NONBLOCK)

  t_start = time.time()
  t_current = t_start
  t_last_indication = t_start
  t_current = t_start
  process_output = ''

  # Keep looping as long as the process is running.
  while p.poll() is None:
    # Avoid polling too often.
    time.sleep(0.1)
    # Update the progress indicator.
    t_current = time.time()
    if (t_current - t_start >= 2) and (t_current - t_last_indication >= 1):
      printer.PrintOverwritableLine(
        printable_command_orange + indicator % int(t_current - t_start))
      sys.stdout.flush()
      t_last_indication = t_current
    # Pull from the process output.
    while True:
      try:
        line = os.read(p.stdout.fileno(), 1024)
      except OSError:
        line = ''
        break
      if line == '': break
      process_output += line

  # The process has exited. Don't forget to retrieve the rest of its output.
  out, err = p.communicate()
  rc = p.poll()
  process_output += out

  printable_command += ' (took %d seconds)' % int(t_current - t_start)
  if rc == 0:
    printer.Print(printer.COLOUR_GREEN + printable_command + printer.NO_COLOUR)
  else:
    printer.Print(printer.COLOUR_RED + printable_command + printer.NO_COLOUR)
    printer.Print(process_output)
  return rc


def RunLinter(jobs):
  return lint.RunLinter(map(lambda x: join(dir_root, x),
                        util.get_source_files()),
                        jobs = args.jobs, progress_prefix = 'cpp lint: ')


def RunClangFormat(clang_path, jobs):
  return clang_format.ClangFormatFiles(util.get_source_files(),
                                       clang_path,
                                       jobs = jobs,
                                       progress_prefix = 'clang-format: ')

def RunClangTidy(clang_path, jobs):
  return clang_tidy.ClangTidyFiles(util.get_source_files(),
                                   clang_path,
                                   jobs = jobs,
                                   progress_prefix = 'clang-tidy: ')

def BuildAll(build_options, jobs, environment_options):
  scons_command = ['scons', '-C', dir_root, 'all', '-j', str(jobs)]
  if util.IsCommandAvailable('ccache'):
    scons_command += ['compiler_wrapper=ccache']
    # Fixes warnings for ccache 3.3.1 and lower:
    environment_options = environment_options.copy()
    environment_options["CCACHE_CPP2"] = 'yes'
  scons_command += DictToString(build_options).split()
  return RunCommand(scons_command, environment_options)


def CanRunAarch64(options, args):
  for target in options['target']:
    if target in ['aarch64', 'a64']:
      return True

  return False


def CanRunAarch32(options, args):
  for target in options['target']:
    if target in ['aarch32', 'a32', 't32']:
      return True
  return False


def RunBenchmarks(options, args):
  rc = 0
  if CanRunAarch32(options, args):
    benchmark_names = util.ListCCFilesWithoutExt(config.dir_aarch32_benchmarks)
    for bench in benchmark_names:
      rc |= RunCommand(
        [os.path.realpath(
          join(config.dir_build_latest, 'benchmarks/aarch32', bench)), '10'])
  if CanRunAarch64(options, args):
    benchmark_names = util.ListCCFilesWithoutExt(config.dir_aarch64_benchmarks)
    for bench in benchmark_names:
      rc |= RunCommand(
        [util.relrealpath(
            join(config.dir_build_latest,
                'benchmarks/aarch64', bench)), '10'])
  return rc



# It is a precommit run if the user did not specify any of the
# options that would affect the automatically generated combinations.
def IsPrecommitRun(args):
  return args.negative_testing == "off" and not AllChoiceAction.WasSetByUser

# Generate a list of all the possible combinations of the passed list:
# ListCombinations( a = [a0, a1], b = [b0, b1] ) will return
# [ {a : a0, b : b0}, {a : a0, b : b1}, {a: a1, b : b0}, {a : a1, b : b1}]
def ListCombinations(**kwargs):
  # End of recursion: no options passed
  if not kwargs:
    return [{}]
  option, values = kwargs.popitem()
  configs = ListCombinations(**kwargs)
  retval = []
  if not isinstance(values, list):
    values = [values]
  for value in values:
    for config in configs:
      new_config = config.copy()
      new_config[option] = value
      retval.append(new_config)
  return retval

# Convert a dictionary into a space separated string
# {a : a0, b : b0} --> "a=a0 b=b0"
def DictToString(options):
  return " ".join(
      ["{}={}".format(option, value) for option, value in options.items()])


if __name__ == '__main__':
  util.require_program('scons')

  args = BuildOptions()

  rc = util.ReturnCode(args.fail_early, printer.Print)

  if args.under_valgrind:
    util.require_program('valgrind')

  tests = test_runner.TestQueue()
  if not args.nolint and not args.dry_run:
    rc.Combine(RunLinter(args.jobs))

  if not args.noclang_format and not args.dry_run:
    rc.Combine(RunClangFormat(args.clang_format, args.jobs))

  if not args.noclang_tidy and not args.dry_run:
    rc.Combine(RunClangTidy(args.clang_tidy, args.jobs))

  list_options = []
  if IsPrecommitRun(args):
    # Maximize the coverage for precommit testing.

    # Debug builds with negative testing and all targets enabled.
    list_options += ListCombinations(
        compiler = args.compiler,
        negative_testing = 'on',
        std = args.std,
        mode = 'debug',
        target = 'a64,a32,t32')

    # Release builds with all targets enabled.
    list_options += ListCombinations(
        compiler = args.compiler,
        negative_testing = 'off',
        std = args.std,
        mode = 'release',
        target = 'a64,a32,t32')

    # Debug builds for individual targets.
    list_options += ListCombinations(
        compiler = args.compiler[0],
        negative_testing = 'off',
        std = args.std,
        mode = 'debug',
        target = ['a32', 't32', 'a64'])
  else:
    list_options = ListCombinations(
        compiler = args.compiler,
        negative_testing = args.negative_testing,
        std = args.std,
        mode = args.mode,
        target = args.target)

  for options in list_options:
    if (args.dry_run):
      print(DictToString(options))
      continue
    # Convert 'compiler' into an environment variable:
    environment_options = {'CXX': options['compiler']}
    del options['compiler']

    # Avoid going through the build stage if we are not using the build
    # result.
    if not (args.notest and args.nobench):
      build_rc = BuildAll(options, args.jobs, environment_options)
      # Don't run the tests for this configuration if the build failed.
      if build_rc != 0:
        rc.Combine(build_rc)
        continue

      # Use the realpath of the test executable so that the commands printed
      # can be copy-pasted and run.
      test_executable = util.relrealpath(
        join(config.dir_build_latest, 'test', 'test-runner'))

      if not args.notest:
        printer.Print(test_executable)
        tests.AddTests(
            test_executable,
            args.filters,
            list(),
            args.under_valgrind)

      if not args.nobench:
        rc.Combine(RunBenchmarks(options, args))

  rc.Combine(tests.Run(args.jobs, args.verbose))
  if not args.dry_run:
    rc.PrintStatus()

  sys.exit(rc.Value)
