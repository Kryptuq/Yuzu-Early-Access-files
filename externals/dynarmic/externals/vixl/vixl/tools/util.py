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

from distutils.version import LooseVersion
import config
import fnmatch
import glob
import operator
import os
import re
import shlex
import subprocess
import sys


def ListCCFilesWithoutExt(path):
  return map(lambda x : os.path.splitext(os.path.basename(x))[0],
             glob.glob(os.path.join(path, '*.cc')))


def abort(message):
  print('ABORTING: ' + message)
  sys.exit(1)


# Emulate python3 subprocess.getstatusoutput.
def getstatusoutput(command):
  try:
    args = shlex.split(command)
    output = subprocess.check_output(args, stderr=subprocess.STDOUT)
    return 0, output.rstrip('\n')
  except subprocess.CalledProcessError as e:
    return e.returncode, e.output.rstrip('\n')


def IsCommandAvailable(command):
    retcode, unused_output = getstatusoutput('which %s' % command)
    return retcode == 0


def ensure_dir(path_name):
  if not os.path.exists(path_name):
    os.makedirs(path_name)


# Check that the specified program is available.
def require_program(program_name):
  rc, out = getstatusoutput('which %s' % program_name)
  if rc != 0:
    print('ERROR: The required program %s was not found.' % program_name)
    sys.exit(rc)

def relrealpath(path, start=os.getcwd()):
  return os.path.relpath(os.path.realpath(path), start)

# Query the compiler about its preprocessor directives and return all of them as
# a dictionnary.
def GetCompilerDirectives(env):
  args = [env['compiler']]
  # Pass the CXXFLAGS varables to the compile, in case we've used "-m32" to
  # compile for i386.
  if env['CXXFLAGS']:
    args.append(str(env['CXXFLAGS']))
  args += ['-E', '-dM', '-']

  # Instruct the compiler to dump all its preprocessor macros.
  dump = subprocess.Popen(args, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                          universal_newlines=True)
  out, _ = dump.communicate()
  return {
    # Extract the macro name as key and value as element.
    match.group(1): match.group(2)
    for match in [
      # Capture macro name.
      re.search('^#define (\S+?) (.+)$', macro)
      for macro in out.split('\n')
    ]
    # Filter out non-matches.
    if match
  }

# Query the target architecture of the compiler. The 'target' architecture of
# the compiler used to build VIXL is considered to be the 'host' architecture of
# VIXL itself.
def GetHostArch(env):
  directives = GetCompilerDirectives(env)
  if "__x86_64__" in directives:
    return "x86_64"
  elif "__i386__" in directives:
    return "i386"
  elif "__arm__" in directives:
    return "aarch32"
  elif "__aarch64__" in directives:
    return "aarch64"
  else:
    raise Exception("Unsupported archtecture")

# Class representing the compiler toolchain and version.
class CompilerInformation(object):
  def __init__(self, env):
    directives = GetCompilerDirectives(env)
    if '__llvm__' in directives:
      major = int(directives['__clang_major__'])
      minor = int(directives['__clang_minor__'])
      self.compiler = 'clang'
      self.version = '{}.{}'.format(major, minor)
    elif '__GNUC__' in directives:
      major = int(directives['__GNUC__'])
      minor = int(directives['__GNUC_MINOR__'])
      self.compiler = 'gcc'
      self.version = '{}.{}'.format(major, minor)
    else:
      # Allow other compilers to be used for building VIXL. However, one would
      # need to teach this class how to extract version information in order to
      # make use of it.
      self.compiler = None
      self.version = None

  def GetDescription(self):
    return "{}-{}".format(self.compiler, self.version)

  def __str__(self):
    return self.GetDescription()

  # Compare string descriptions with our object. The semantics are:
  #
  # - Equality
  #
  #   If the description does not have a version number, then we compare the
  #   compiler names. For instance, "clang-3.6" is still equal to "clang" but of
  #   course is not to "gcc".
  #
  # - Ordering
  #
  #   Asking whether a compiler is lower than another depends on the version
  #   number. What we are really asking here when using these operator is
  #   "Is my compiler in the allowed range?". So with this in mind, comparing
  #   two different compilers will always return false. If the compilers are the
  #   same, then the version numbers are compared. Of course, we cannot use
  #   ordering operators if no version number is provided.

  def __eq__(self, description):
    if description == self.GetDescription():
      return True
    else:
      # The user may not have provided a version, let's see if it matches the
      # compiler.
      return self.compiler == description

  def __ne__(self, description):
    return not self.__eq__(description)

  def __lt__(self, description):
    return self.CompareVersion(operator.lt, description)

  def __le__(self, description):
    return self.CompareVersion(operator.le, description)

  def __ge__(self, description):
    return self.CompareVersion(operator.ge, description)

  def __gt__(self, description):
    return self.CompareVersion(operator.gt, description)

  # Comparing the provided `description` string, in the form of
  # "{compiler}-{major}.{minor}". The comparison is done using the provided
  # `operator` argument.
  def CompareVersion(self, operator, description):
    match = re.search('^(\S+)-(.*?)$', description)
    if not match:
      raise Exception("A version number is required when comparing compilers")
    compiler, version = match.group(1), match.group(2)
    # The result is false if the compilers are different, otherwise compare the
    # version numbers.
    return self.compiler == compiler and \
           operator(LooseVersion(self.version), LooseVersion(version))

class ReturnCode:
  def __init__(self, exit_on_error, printer_fn):
    self.rc = 0
    self.exit_on_error = exit_on_error
    self.printer_fn = printer_fn

  def Combine(self, rc):
    self.rc |= rc
    if self.exit_on_error and rc != 0:
      self.PrintStatus()
      sys.exit(rc)

  @property
  def Value(self):
    return self.rc

  def PrintStatus(self):
    self.printer_fn('\n$ ' + ' '.join(sys.argv))
    if self.rc == 0:
      self.printer_fn('SUCCESS')
    else:
      self.printer_fn('FAILURE')

# Return a list of files whose name matches at least one `include` pattern, and
# no `exclude` patterns, and whose directory (relative to the repository base)
# matches at least one `include_dirs` and no `exclude_dirs` patterns.
#
# For directory matches, leading and trailing slashes are added first (so that
# "*/foo/*" matches all of 'foo/bar', 'bar/foo' and 'bar/foo/bar').
def get_source_files(
    include = ['*.h', '*.cc'],
    include_dirs = ['/src/*', '/test/*', '/examples/*', '/benchmarks/*'],
    exclude = [],
    exclude_dirs = ['.*', '*/traces/*']):
  def NameMatchesAnyFilter(name, filters):
    for f in filters:
      if fnmatch.fnmatch(name, f):
        return True
    return False

  files_found = []
  for root, dirs, files in os.walk(config.dir_root):
    git_path = os.path.join('/', os.path.relpath(root, config.dir_root), '')
    if NameMatchesAnyFilter(git_path, include_dirs) and \
        not NameMatchesAnyFilter(git_path, exclude_dirs):
      files_found += [
        os.path.join(root, name)
        for name in files
        if NameMatchesAnyFilter(name, include) and \
            not NameMatchesAnyFilter(name, exclude)
      ]
  return files_found
