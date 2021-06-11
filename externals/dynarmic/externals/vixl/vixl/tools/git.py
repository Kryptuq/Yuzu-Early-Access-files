# Copyright 2014, VIXL authors
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
# THIS SOFTWARE IS PROVIDED BY ARM LIMITED AND CONTRIBUTORS "AS IS" AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL ARM LIMITED BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
# OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
# EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import re
import util
import os.path
from pipes import quote

def is_git_repository_root(path):
  command = 'git -C ' + quote(path) + ' rev-parse --show-toplevel'
  status, toplevel = util.getstatusoutput(command)
  if status != 0: return False
  return os.path.samefile(toplevel, path)

def get_tracked_files():
  command = 'git ls-tree HEAD -r --full-tree --name-only'

  status, tracked = util.getstatusoutput(command)
  if status != 0: util.abort('Failed to list tracked files.')

  return tracked


# Get untracked files in src/, test/, and tools/.
def get_untracked_files():
  status, output = util.getstatusoutput('git status -s')
  if status != 0: util.abort('Failed to get git status.')

  untracked_regexp = re.compile('\?\?.*(src/|test/|tools/).*(.cc$|.h$)')
  files_in_watched_folder = lambda n: untracked_regexp.search(n) != None
  untracked_files = filter(files_in_watched_folder, output.split('\n'))

  return untracked_files
