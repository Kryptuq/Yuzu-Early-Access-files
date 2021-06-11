#!/usr/bin/env python2.7

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


import sys
import time
import multiprocessing

output_redirected_to_file = not sys.stdout.isatty()

def ColourCode(colour):
  return '' if output_redirected_to_file else colour

COLOUR_GREEN =  ColourCode("\x1b[0;32m")
COLOUR_ORANGE = ColourCode("\x1b[0;33m")
COLOUR_RED =    ColourCode("\x1b[0;31m")
NO_COLOUR =     ColourCode("\x1b[0m")

# Indicates the 'type' of the last printed line.
LINE_TYPE_NONE = 0
# Any type below this one is overwritable.
LINE_TYPE_OVERWRITABLE = 1
LINE_TYPE_PROGRESS = 2
LINE_TYPE_LINTER = 3

__print_lock__ = multiprocessing.Lock()
__last_overwritable_line_length__ = multiprocessing.Value('i', 0)
__last_line_type__ = multiprocessing.Value('i', LINE_TYPE_NONE)


def EnsureNewLine():
  if __last_line_type__.value >= LINE_TYPE_OVERWRITABLE:
    sys.stdout.write('\n')
    __last_line_type__.value = LINE_TYPE_NONE


def PrintInternal(string):
  sys.stdout.write(string)
  spaces = __last_overwritable_line_length__.value - len(string)
  if spaces > 0:
    sys.stdout.write(' ' * spaces)


def Print(string, has_lock = False):
  if not has_lock: __print_lock__.acquire()

  if __last_line_type__.value != LINE_TYPE_NONE:
    sys.stdout.write('\n')

  PrintInternal(string)
  sys.stdout.write('\n')
  __last_overwritable_line_length__.value = 0
  __last_line_type__.value = LINE_TYPE_NONE

  if not has_lock: __print_lock__.release()


# Lines of a specific type only overwrite and can only be overwritten by lines
# of the same type.
def PrintOverwritableLine(string, has_lock = False, type = LINE_TYPE_NONE):
  if not has_lock: __print_lock__.acquire()

  if (__last_line_type__.value != type) and \
      (__last_line_type__.value >= LINE_TYPE_OVERWRITABLE):
    sys.stdout.write('\n')

  PrintInternal(string)
  if not output_redirected_to_file:
    sys.stdout.write('\r')
  else:
    sys.stdout.write('\n')
  sys.stdout.flush()

  __last_overwritable_line_length__.value = len(string)
  __last_line_type__.value = type

  if not has_lock: __print_lock__.release()


# Display the run progress:
# prefix [time| progress|+ passed|- failed]  name
def UpdateProgress(start_time, passed, failed, count, skipped, known_failures,
                   name, prefix = '', prevent_next_overwrite = False,
                   has_lock = False):
  minutes, seconds = divmod(time.time() - start_time, 60)
  progress = float(passed + failed + skipped) / max(1, count) * 100
  passed_colour = COLOUR_GREEN if passed != 0 else ''
  failed_colour = COLOUR_RED if failed != 0 else ''
  skipped_colour = COLOUR_ORANGE if (skipped + known_failures) != 0 else ''
  indicator = '[%02d:%02d| %3d%%|'
  indicator += passed_colour + '+ %d' + NO_COLOUR + '|'
  indicator += failed_colour + '- %d' + NO_COLOUR + '|'
  indicator += skipped_colour + '? %d' + NO_COLOUR + ']'

  progress_string = prefix
  progress_string += indicator % (minutes, seconds, progress, passed, failed,
                                  skipped + known_failures)
  progress_string += '  ' + name

  PrintOverwritableLine(progress_string, type = LINE_TYPE_PROGRESS,
                        has_lock = has_lock)
  if prevent_next_overwrite:
    sys.stdout.write('\n')
    __last_line_type__.value = LINE_TYPE_NONE
