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

import glob
import itertools
import os
from os.path import join
import platform
import subprocess
import sys
from collections import OrderedDict

root_dir = os.path.dirname(File('SConstruct').rfile().abspath)
sys.path.insert(0, join(root_dir, 'tools'))
import config
import util

from SCons.Errors import UserError


Help('''
Build system for the VIXL project.
See README.md for documentation and details about the build system.
''')


# We track top-level targets to automatically generate help and alias them.
class VIXLTargets:
  def __init__(self):
    self.targets = []
    self.help_messages = []
  def Add(self, target, help_message):
    self.targets.append(target)
    self.help_messages.append(help_message)
  def Help(self):
    res = ""
    for i in range(len(self.targets)):
      res += '\t{0:<{1}}{2:<{3}}\n'.format(
        'scons ' + self.targets[i],
        len('scons ') + max(map(len, self.targets)),
        ' : ' + self.help_messages[i],
        len(' : ') + max(map(len, self.help_messages)))
    return res

top_level_targets = VIXLTargets()



# Build options ----------------------------------------------------------------

# Store all the options in a dictionary.
# The SConstruct will check the build variables and construct the build
# environment as appropriate.
options = {
    'all' : { # Unconditionally processed.
      'CCFLAGS' : ['-Wall',
                   '-Werror',
                   '-fdiagnostics-show-option',
                   '-Wextra',
                   '-Wredundant-decls',
                   '-pedantic',
                   '-Wwrite-strings',
                   '-Wunused',
                   '-Wno-missing-noreturn'],
      'CPPPATH' : [config.dir_src_vixl]
      },
#   'build_option:value' : {
#     'environment_key' : 'values to append'
#     },
    'mode:debug' : {
      'CCFLAGS' : ['-DVIXL_DEBUG', '-O0']
      },
    'mode:release' : {
      'CCFLAGS' : ['-O3'],
      },
    'simulator:aarch64' : {
      'CCFLAGS' : ['-DVIXL_INCLUDE_SIMULATOR_AARCH64'],
      },
    'symbols:on' : {
      'CCFLAGS' : ['-g'],
      'LINKFLAGS' : ['-g']
      },
    'negative_testing:on' : {
      'CCFLAGS' : ['-DVIXL_NEGATIVE_TESTING']
      },
    'code_buffer_allocator:mmap' : {
      'CCFLAGS' : ['-DVIXL_CODE_BUFFER_MMAP']
      },
    'code_buffer_allocator:malloc' : {
      'CCFLAGS' : ['-DVIXL_CODE_BUFFER_MALLOC']
      },
    'ubsan:on' : {
      'CCFLAGS': ['-fsanitize=undefined'],
      'LINKFLAGS': ['-fsanitize=undefined']
      }
    }


# A `DefaultVariable` has a default value that depends on elements not known
# when variables are first evaluated.
# Each `DefaultVariable` has a handler that will compute the default value for
# the given environment.
def modifiable_flags_handler(env):
  env['modifiable_flags'] = \
      'on' if 'mode' in env and env['mode'] == 'debug' else 'off'


def symbols_handler(env):
  env['symbols'] = 'on' if 'mode' in env and env['mode'] == 'debug' else 'off'

def Is32BitHost(env):
  return env['host_arch'] in ['aarch32', 'i386']

def IsAArch64Host(env):
  return env['host_arch'] == 'aarch64'

def CanTargetA32(env):
  return 'a32' in env['target']

def CanTargetT32(env):
  return 't32' in env['target']

def CanTargetAArch32(env):
  return CanTargetA32(env) or CanTargetT32(env)

def CanTargetA64(env):
  return 'a64' in env['target']

def CanTargetAArch64(env):
  return CanTargetA64(env)


# By default, include the simulator only if AArch64 is targeted and we are not
# building VIXL natively for AArch64.
def simulator_handler(env):
  if not IsAArch64Host(env) and CanTargetAArch64(env):
    env['simulator'] = 'aarch64'
  else:
    env['simulator'] = 'none'


# 'mmap' is required for use with 'mprotect', which is needed for the tests
# (when running natively), so we use it by default where we can.
def code_buffer_allocator_handler(env):
  directives = util.GetCompilerDirectives(env)
  if '__linux__' in directives:
    env['code_buffer_allocator'] = 'mmap'
  else:
    env['code_buffer_allocator'] = 'malloc'

# A validator checks the consistency of provided options against the environment.
def default_validator(env):
  pass


def simulator_validator(env):
  if env['simulator'] == 'aarch64' and not CanTargetAArch64(env):
    raise UserError('Building an AArch64 simulator implies that VIXL targets '
                    'AArch64. Set `target` to include `aarch64` or `a64`.')


# Default variables may depend on each other, therefore we need this dictionnary
# to be ordered.
vars_default_handlers = OrderedDict({
    # variable_name    : [ 'default val', 'handler', 'validator']
    'symbols'          : [ 'mode==debug', symbols_handler, default_validator ],
    'modifiable_flags' : [ 'mode==debug', modifiable_flags_handler, default_validator],
    'simulator'        : [ 'on if the target architectures include AArch64 but '
                           'the host is not AArch64, else off',
                           simulator_handler, simulator_validator ],
    'code_buffer_allocator' : [ 'mmap with __linux__, malloc otherwise',
                                code_buffer_allocator_handler, default_validator ]
    })


def DefaultVariable(name, help, allowed_values):
  help = '%s (%s)' % (help, '|'.join(allowed_values))
  default_value = vars_default_handlers[name][0]
  def validator(name, value, env):
    if value != default_value and value not in allowed_values:
        raise UserError('Invalid value for option {name}: {value}.  '
                        'Valid values are: {allowed_values}'.format(
                            name, value, allowed_values))
  return (name, help, default_value, validator)


def SortListVariable(iterator):
  # Previously this code relied on the order of items in a list
  # converted from a set. However in Python 3 the order changes each run.
  # Here we do a custom partial sort to ensure that the build directory
  # name is stable, the same across Python 2 and 3, and the same as the
  # old code.
  result = list(sorted(iterator))
  result = sorted(result, key=lambda x: x == 't32', reverse=True)
  result = sorted(result, key=lambda x: x == 'a32', reverse=True)
  result = sorted(result, key=lambda x: x == 'a64', reverse=True)
  return result


def AliasedListVariable(name, help, default_value, allowed_values, aliasing):
  help = '%s (all|auto|comma-separated list) (any combination from [%s])' % \
         (help, ', '.join(allowed_values))

  def validator(name, value, env):
    # Here list has been converted to space separated strings.
    if value == '': return  # auto
    for v in value.split():
      if v not in allowed_values:
        raise UserError('Invalid value for %s: %s' % (name, value))

  def converter(value):
    if value == 'auto': return []
    if value == 'all':
      translated = [aliasing[v] for v in allowed_values]
      return SortListVariable(itertools.chain.from_iterable(translated))
    # The validator is run later hence the get.
    translated = [aliasing.get(v, v) for v in value.split(',')]
    return SortListVariable(itertools.chain.from_iterable(translated))

  return (name, help, default_value, validator, converter)


vars = Variables()
# Define command line build options.
vars.AddVariables(
    AliasedListVariable('target', 'Target ISA/Architecture', 'auto',
                        ['aarch32', 'a32', 't32', 'aarch64', 'a64'],
                        {'aarch32' : ['a32', 't32'],
                         'a32' : ['a32'], 't32' : ['t32'],
                         'aarch64' : ['a64'], 'a64' : ['a64']}),
    EnumVariable('mode', 'Build mode',
                 'release', allowed_values=config.build_options_modes),
    EnumVariable('ubsan', 'Enable undefined behavior checks',
                 'off', allowed_values=['on', 'off']),
    EnumVariable('negative_testing',
                  'Enable negative testing (needs exceptions)',
                 'off', allowed_values=['on', 'off']),
    DefaultVariable('symbols', 'Include debugging symbols in the binaries',
                    ['on', 'off']),
    DefaultVariable('simulator', 'Simulators to include', ['aarch64', 'none']),
    DefaultVariable('code_buffer_allocator',
                    'Configure the allocation mechanism in the CodeBuffer',
                    ['malloc', 'mmap']),
    ('std',
     'C++ standard. The standards tested are: %s.' % \
     ', '.join(config.tested_cpp_standards),
     config.tested_cpp_standards[0]),
    ('compiler_wrapper', 'Command to prefix to the C and C++ compiler (e.g ccache)', '')
    )

# We use 'variant directories' to avoid recompiling multiple times when build
# options are changed, different build paths are used depending on the options
# set. These are the options that should be reflected in the build directory
# path.
options_influencing_build_path = [
  'target', 'mode', 'symbols', 'compiler', 'std', 'simulator', 'negative_testing',
  'code_buffer_allocator'
]



# Build helpers ----------------------------------------------------------------

def RetrieveEnvironmentVariables(env):
  for key in ['CC', 'CXX', 'AR', 'RANLIB', 'LD']:
    if os.getenv(key): env[key] = os.getenv(key)
  if os.getenv('LD_LIBRARY_PATH'): env['LIBPATH'] = os.getenv('LD_LIBRARY_PATH')
  if os.getenv('CCFLAGS'):
    env.Append(CCFLAGS = os.getenv('CCFLAGS').split())
  if os.getenv('CXXFLAGS'):
    env.Append(CXXFLAGS = os.getenv('CXXFLAGS').split())
  if os.getenv('LINKFLAGS'):
    env.Append(LINKFLAGS = os.getenv('LINKFLAGS').split())

# The architecture targeted by default will depend on the compiler being
# used. 'host_arch' is extracted from the compiler while 'target' can be
# set by the user.
# By default, we target both AArch32 and AArch64 unless the compiler targets a
# 32-bit architecture. At the moment, we cannot build VIXL's AArch64 support on
# a 32-bit platform.
# TODO: Port VIXL to build on a 32-bit platform.
def target_handler(env):
  # Auto detect
  if Is32BitHost(env):
    # We use list(set(...)) to keep the same order as if it was specify as
    # an option.
    env['target'] = SortListVariable(['a32', 't32'])
  else:
    env['target'] = SortListVariable(['a64', 'a32', 't32'])


def target_validator(env):
  # TODO: Port VIXL64 to work on a 32-bit platform.
  if Is32BitHost(env) and CanTargetAArch64(env):
    raise UserError('Building VIXL for AArch64 in 32-bit is not supported. Set '
                    '`target` to `aarch32`')


# The target option is handled differently from the rest.
def ProcessTargetOption(env):
  if env['target'] == []: target_handler(env)

  if 'a32' in env['target']: env['CCFLAGS'] += ['-DVIXL_INCLUDE_TARGET_A32']
  if 't32' in env['target']: env['CCFLAGS'] += ['-DVIXL_INCLUDE_TARGET_T32']
  if 'a64' in env['target']: env['CCFLAGS'] += ['-DVIXL_INCLUDE_TARGET_A64']

  target_validator(env)


def ProcessBuildOptions(env):
  # 'all' is unconditionally processed.
  if 'all' in options:
    for var in options['all']:
      if var in env and env[var]:
        env[var] += options['all'][var]
      else:
        env[var] = options['all'][var]

  # The target option *must* be processed before the options defined in
  # vars_default_handlers.
  ProcessTargetOption(env)

  # Other build options must match 'option:value'
  env_dict = env.Dictionary()

  # First apply the default variables handlers in order.
  for key, value in vars_default_handlers.items():
    default = value[0]
    handler = value[1]
    if env_dict.get(key) == default:
      handler(env_dict)

  # Second, run the series of validators, to check for errors.
  for _, value in vars_default_handlers.items():
    validator = value[2]
    validator(env)

  for key in env_dict.keys():
    # Then update the environment according to the value of the variable.
    key_val_couple = key + ':%s' % env_dict[key]
    if key_val_couple in options:
      for var in options[key_val_couple]:
        env[var] += options[key_val_couple][var]


def ConfigureEnvironmentForCompiler(env):
  compiler = util.CompilerInformation(env)
  if compiler == 'clang':
    # These warnings only work for Clang.
    # -Wimplicit-fallthrough only works when compiling the code base as C++11 or
    # newer. The compiler does not complain if the option is passed when
    # compiling earlier C++ standards.
    env.Append(CPPFLAGS = ['-Wimplicit-fallthrough', '-Wshorten-64-to-32'])

    # The '-Wunreachable-code' flag breaks builds for clang 3.4.
    if compiler != 'clang-3.4':
      env.Append(CPPFLAGS = ['-Wunreachable-code'])

    if env['ubsan'] == 'on':
      env.Append(LINKFLAGS = ['-fuse-ld=lld'])

  # GCC 4.8 has a bug which produces a warning saying that an anonymous Operand
  # object might be used uninitialized:
  #   http://gcc.gnu.org/bugzilla/show_bug.cgi?id=57045
  # The bug does not seem to appear in GCC 4.7, or in debug builds with GCC 4.8.
  if env['mode'] == 'release':
    if compiler == 'gcc-4.8':
      env.Append(CPPFLAGS = ['-Wno-maybe-uninitialized'])

  # GCC 6 and higher is able to detect throwing from inside a destructor and
  # reports a warning. However, if negative testing is enabled then assertions
  # will throw exceptions.
  if env['negative_testing'] == 'on' and env['mode'] == 'debug' \
      and compiler >= 'gcc-6':
    env.Append(CPPFLAGS = ['-Wno-terminate'])

  # Suggest missing override keywords on methods.
  if compiler >= 'gcc-5':
    env.Append(CPPFLAGS = ['-Wsuggest-override'])
  elif compiler >= 'clang-3.6':
    env.Append(CPPFLAGS = ['-Winconsistent-missing-override'])


def ConfigureEnvironment(env):
  RetrieveEnvironmentVariables(env)
  env['compiler'] = env['CXX']
  if env['compiler_wrapper'] != '':
    env['CXX'] = env['compiler_wrapper'] + ' ' + env['CXX']
    env['CC'] = env['compiler_wrapper'] + ' ' + env['CC']
  env['host_arch'] = util.GetHostArch(env)
  ProcessBuildOptions(env)
  if 'std' in env:
    env.Append(CPPFLAGS = ['-std=' + env['std']])
    std_path = env['std']
  ConfigureEnvironmentForCompiler(env)


def TargetBuildDir(env):
  # Build-time option values are embedded in the build path to avoid requiring a
  # full build when an option changes.
  build_dir = config.dir_build
  for option in options_influencing_build_path:
    option_value = ''.join(env[option]) if option in env else ''
    build_dir = join(build_dir, option + '_'+ option_value)
  return build_dir


def PrepareVariantDir(location, build_dir):
  location_build_dir = join(build_dir, location)
  VariantDir(location_build_dir, location)
  return location_build_dir


def VIXLLibraryTarget(env):
  build_dir = TargetBuildDir(env)
  # Create a link to the latest build directory.
  # Use `-r` to avoid failure when `latest` exists and is a directory.
  subprocess.check_call(["rm", "-rf", config.dir_build_latest])
  util.ensure_dir(build_dir)
  subprocess.check_call(["ln", "-s", build_dir, config.dir_build_latest])
  # Source files are in `src` and in `src/aarch64/`.
  variant_dir_vixl = PrepareVariantDir(join('src'), build_dir)
  sources = [Glob(join(variant_dir_vixl, '*.cc'))]
  if CanTargetAArch32(env):
    variant_dir_aarch32 = PrepareVariantDir(join('src', 'aarch32'), build_dir)
    sources.append(Glob(join(variant_dir_aarch32, '*.cc')))
  if CanTargetAArch64(env):
    variant_dir_aarch64 = PrepareVariantDir(join('src', 'aarch64'), build_dir)
    sources.append(Glob(join(variant_dir_aarch64, '*.cc')))
  return env.Library(join(build_dir, 'vixl'), sources)



# Build ------------------------------------------------------------------------

# The VIXL library, built by default.
env = Environment(variables = vars,
                  BUILDERS = {
                      'Markdown': Builder(action = 'markdown $SOURCE > $TARGET',
                                          suffix = '.html')
                  }, ENV = os.environ)
# Abort the build if any command line option is unknown or invalid.
unknown_build_options = vars.UnknownVariables()
if unknown_build_options:
  print('Unknown build options: ' + str(unknown_build_options.keys()))
  Exit(1)

if env['negative_testing'] == 'on' and env['mode'] != 'debug':
  print('negative_testing only works in debug mode')
  Exit(1)

ConfigureEnvironment(env)
Help(vars.GenerateHelpText(env))
libvixl = VIXLLibraryTarget(env)
Default(libvixl)
env.Alias('libvixl', libvixl)
top_level_targets.Add('', 'Build the VIXL library.')


# Common test code.
test_build_dir = PrepareVariantDir('test', TargetBuildDir(env))
test_objects = [env.Object(Glob(join(test_build_dir, '*.cc')))]

# AArch32 support
if CanTargetAArch32(env):
  # The examples.
  aarch32_example_names = util.ListCCFilesWithoutExt(config.dir_aarch32_examples)
  aarch32_examples_build_dir = PrepareVariantDir('examples/aarch32', TargetBuildDir(env))
  aarch32_example_targets = []
  for example in aarch32_example_names:
    prog = env.Program(join(aarch32_examples_build_dir, example),
                       join(aarch32_examples_build_dir, example + '.cc'),
                       LIBS=[libvixl])
    aarch32_example_targets.append(prog)
  env.Alias('aarch32_examples', aarch32_example_targets)
  top_level_targets.Add('aarch32_examples', 'Build the examples for AArch32.')

  # The benchmarks
  aarch32_benchmark_names = util.ListCCFilesWithoutExt(config.dir_aarch32_benchmarks)
  aarch32_benchmarks_build_dir = PrepareVariantDir('benchmarks/aarch32', TargetBuildDir(env))
  aarch32_benchmark_targets = []
  for bench in aarch32_benchmark_names:
    prog = env.Program(join(aarch32_benchmarks_build_dir, bench),
                       join(aarch32_benchmarks_build_dir, bench + '.cc'),
                       LIBS=[libvixl])
    aarch32_benchmark_targets.append(prog)
  env.Alias('aarch32_benchmarks', aarch32_benchmark_targets)
  top_level_targets.Add('aarch32_benchmarks', 'Build the benchmarks for AArch32.')

  # The tests.
  test_aarch32_build_dir = PrepareVariantDir(join('test', 'aarch32'), TargetBuildDir(env))
  test_objects.append(env.Object(
      Glob(join(test_aarch32_build_dir, '*.cc')),
      CPPPATH = env['CPPPATH'] + [config.dir_tests],
      CCFLAGS = [flag for flag in env['CCFLAGS'] if flag != '-O3']))

# AArch64 support
if CanTargetAArch64(env):
  # The benchmarks.
  aarch64_benchmark_names = util.ListCCFilesWithoutExt(config.dir_aarch64_benchmarks)
  aarch64_benchmarks_build_dir = PrepareVariantDir('benchmarks/aarch64', TargetBuildDir(env))
  aarch64_benchmark_targets = []
  bench_utils = env.Object(join(aarch64_benchmarks_build_dir, 'bench-utils.o'),
                           join(aarch64_benchmarks_build_dir, 'bench-utils.cc'))
  for bench in aarch64_benchmark_names:
    if bench != 'bench-utils':
      prog = env.Program(join(aarch64_benchmarks_build_dir, bench),
                         [join(aarch64_benchmarks_build_dir, bench + '.cc'), bench_utils],
                         LIBS=[libvixl])
      aarch64_benchmark_targets.append(prog)
  env.Alias('aarch64_benchmarks', aarch64_benchmark_targets)
  top_level_targets.Add('aarch64_benchmarks', 'Build the benchmarks for AArch64.')

  # The examples.
  aarch64_example_names = util.ListCCFilesWithoutExt(config.dir_aarch64_examples)
  aarch64_examples_build_dir = PrepareVariantDir('examples/aarch64', TargetBuildDir(env))
  aarch64_example_targets = []
  for example in aarch64_example_names:
    prog = env.Program(join(aarch64_examples_build_dir, example),
                       join(aarch64_examples_build_dir, example + '.cc'),
                       LIBS=[libvixl])
    aarch64_example_targets.append(prog)
  env.Alias('aarch64_examples', aarch64_example_targets)
  top_level_targets.Add('aarch64_examples', 'Build the examples for AArch64.')

  # The tests.
  test_aarch64_build_dir = PrepareVariantDir(join('test', 'aarch64'), TargetBuildDir(env))
  test_objects.append(env.Object(
      Glob(join(test_aarch64_build_dir, '*.cc')),
      CPPPATH = env['CPPPATH'] + [config.dir_tests],
      CCFLAGS = [flag for flag in env['CCFLAGS'] if flag != '-O3']))

  # The test requires building the example files with specific options, so we
  # create a separate variant dir for the example objects built this way.
  test_aarch64_examples_vdir = join(TargetBuildDir(env), 'test', 'aarch64', 'test_examples')
  VariantDir(test_aarch64_examples_vdir, '.')
  test_aarch64_examples_obj = env.Object(
      [Glob(join(test_aarch64_examples_vdir, join('test', 'aarch64', 'examples', '*.cc'))),
       Glob(join(test_aarch64_examples_vdir, join('examples/aarch64', '*.cc')))],
      CCFLAGS = env['CCFLAGS'] + ['-DTEST_EXAMPLES'],
      CPPPATH = env['CPPPATH'] + [config.dir_aarch64_examples] + [config.dir_tests])
  test_objects.append(test_aarch64_examples_obj)

test = env.Program(join(test_build_dir, 'test-runner'), test_objects,
                   LIBS=[libvixl])
env.Alias('tests', test)
top_level_targets.Add('tests', 'Build the tests.')


env.Alias('all', top_level_targets.targets)
top_level_targets.Add('all', 'Build all the targets above.')

Help('\n\nAvailable top level targets:\n' + top_level_targets.Help())

extra_targets = VIXLTargets()

# Build documentation
doc = [
    env.Markdown('README.md'),
    env.Markdown('doc/changelog.md'),
    env.Markdown('doc/aarch32/getting-started-aarch32.md'),
    env.Markdown('doc/aarch32/design/code-generation-aarch32.md'),
    env.Markdown('doc/aarch32/design/literal-pool-aarch32.md'),
    env.Markdown('doc/aarch64/supported-instructions-aarch64.md'),
    env.Markdown('doc/aarch64/getting-started-aarch64.md'),
    env.Markdown('doc/aarch64/topics/ycm.md'),
    env.Markdown('doc/aarch64/topics/extending-the-disassembler.md'),
    env.Markdown('doc/aarch64/topics/index.md'),
]
env.Alias('doc', doc)
extra_targets.Add('doc', 'Convert documentation to HTML (requires the '
                         '`markdown` program).')

Help('\nAvailable extra targets:\n' + extra_targets.Help())
