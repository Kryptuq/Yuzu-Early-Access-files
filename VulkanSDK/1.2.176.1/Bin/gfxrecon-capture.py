#!/usr/bin/env python3
#
# Copyright (c) 2020 LunarG, Inc.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
#
#
# Utility for capturing Vulkan API calls using gfxreconstruct


import argparse
import os
import shutil
import sys
import subprocess


######################
# Define a usage message.
# We use a custom usage message because Python's arg parser
# doesn't provide a very good usage message for positional arguments.
# (The default usage message for positional args is
# as an ellipse. We would rather have "<program> [<programArgs>]".)
def UsageMsg():
    msg = ('gfxrecon-capture.py [-h]' + os.linesep +
           '                           [-w dir]' + os.linesep +
           '                           [-o captureFile]' + os.linesep +
           '                           [-f captureFrames]' + os.linesep +
           '                           [--no-file-timestamp]' + os.linesep +
           '                           [--trigger {F1-F12,TAB,CTRL}]' + os.linesep +
           '                           [--compression-type {LZ4,ZLIB,ZSTD,NONE}]' + os.linesep +
           '                           [--file-flush]' + os.linesep +
           '                           [--log-level {debug,info,warn,error,fatal}]' + os.linesep +
           '                           [--log-file <file>]' + os.linesep)
    if sys.platform == 'win32':
        msg += '                           [--log-debugview]' + os.linesep
    msg += '                           [--memory-tracking-mode {page_guard,assisted,unassisted}]' + os.linesep
    msg += '                           <program> [<programArgs>]'
    return msg



######################
# SmartFormater is used by arg parser to assist in breaking arg help text into multiple lines.
# It is used whenever the help text starts with 'R|'.
class SmartFormatter(argparse.HelpFormatter):
    def _split_lines(self, text, width):
        if text.startswith('R|'):
            return text[2:].splitlines()
        return argparse.HelpFormatter._split_lines(self, text, width)


######################
# Print error message and exit with non-zero status
def PrintErrorAndExit(msg):
    print(os.path.basename(__file__) + ' error: ' + msg)
    sys.exit(1)


######################
# Set an environment variable to a given value or remove it from the environment if None
def SetEnvVar(name, value):
    if value is not None:
        os.environ[name] = value
    elif name in os.environ:
        del os.environ[name]


######################
# Parse arguments
def ParseArgs():

    triggerKeyChoices = ['F1','F2','F3','F4','F5','F6','F7','F8','F9','F10','F11','F12','TAB','CTRL']
    compressionTypeChoices = ['LZ4','ZLIB','ZSTD','NONE']
    logLevelChoices = ['debug','info','warn','error','fatal']
    memoryTrackingModeChoices = ['page_guard','assisted','unassisted']

    parser = argparse.ArgumentParser(prog=os.path.basename(sys.argv[0]), description='Create a capture of a Vulkan program.', usage=UsageMsg(), allow_abbrev=False, formatter_class=SmartFormatter)

    # Common optional args
    # All arguments default to None, indicating they should be unset in the capture environment
    parser.add_argument('-w', '--working-dir', dest='workingDir', metavar='<dir>', help='Set CWD to this directory before running the program')
    parser.add_argument('-o', '--capture-file', dest='captureFile', metavar='<captureFile>', default='gfxrecon_capture.gfxr', help='Name of the capture file, default is gfxrecon_capture.gfxr')
    parser.add_argument('-f', '--capture-frames', dest='captureFrames', metavar='<captureFrames>', help='List of frames to capture, default is all frames')
    parser.add_argument('--no-file-timestamp', dest='fileTimestamp', action='store_const', const='false', help='Do not add a timestamp to the capture file name')
    parser.add_argument('--trigger', dest='trigger', choices=triggerKeyChoices, help='Specify a hotkey to start/stop capture')
    parser.add_argument('--compression-type', dest='compressionType', choices=compressionTypeChoices, help='Specify the type of compression to use in the capture file, default is LZ4')
    parser.add_argument('--file-flush', dest='fileFlush', action='store_const', const='true', help='Flush output stream after each packet is written to capture file')
    parser.add_argument('--log-level', dest='logLevel', choices=logLevelChoices, help='Specify highest level message to log, default is info')
    parser.add_argument('--log-file', dest='logFile', metavar='<logFile>', help='Write log messages to a file at the specified path. Default is: Empty string (file logging disabled)')
    parser.add_argument('--log-debugview', dest='logDebugView', action='store_const', const='true', help='Log messages with OutputDebugStringA' if sys.platform=='win32' else argparse.SUPPRESS)
    parser.add_argument('--memory-tracking-mode', dest='memoryTrackingMode', choices=memoryTrackingModeChoices , help=
                        'R|Method to use to track changes to memory mapped objects:' + os.linesep +
                        '   page_guard: use pageguard to track changes (default)' + os.linesep +
                        '   assisted: application will call vkFlushMappedMemoryRanges' + os.linesep +
                        '      for memory to be written to the capture file' + os.linesep +
                        '   unassisted: all mapped memory will be written to the' + os.linesep +
                        '      capture file during VkQueueSubmit and VkUnmapMemory')

    # Required args
    parser.add_argument('programAndArgs', metavar='<program> [<program args>]', nargs=argparse.REMAINDER, help='Program to capture, optionally followed by program arguments')

    return parser


######################
# PrintArgs - for debugging
def PrintArgs(args):
    print('working-dir', args.workingDir)
    print('capture-file', os.path.abspath(args.captureFile))
    print('capture-frames', args.captureFrames)
    print('no-file-timestamp', args.fileTimestamp)
    print('trigger', args.trigger)
    print('compression-type', args.compressionType)
    print('file-flush', args.fileFlush)
    print('log-level', args.logLevel)
    print('log-file', args.logFile)
    print('log-debugview', args.logDebugView)
    print('memory-tracking-mode', args.memoryTrackingMode)
    print('programAndArgs', args.programAndArgs)


######################
# Get the full path to the command to execute
def GetCommandPath(args):
    # Replace ~ or ~user with the user's home path before calling shutil.which()
    programName=os.path.expanduser(args.programAndArgs[0])
    return shutil.which(programName)

######################
# Do validation on arguments
def ValidateArgs(args):

    # Verify workingDir exists and is a directory.
    if args.workingDir is not None:
        if (not os.path.isdir(args.workingDir)):
            PrintErrorAndExit('Working directory ' + args.workingDir + ' does not exist')

    # Make sure program was specified. arg parser doesn't allow specifying programAndArgs
    # as required.
    if len(args.programAndArgs) == 0:
        print('usage: ' + UsageMsg())
        PrintErrorAndExit('<program> must be specified')

    # Verify programName exists and is executable.
    if GetCommandPath(args) is None:
        PrintErrorAndExit('Cannot find program ' + programName + ' to execute')

    # Verify captureFile directory exists and is a valid directory.
    captureFileDir = os.path.dirname(os.path.abspath(args.captureFile))
    if (not os.path.exists(captureFileDir)):
        PrintErrorAndExit('Capture file output directory ' + captureFileDir + ' does not exist')
    if (not os.path.isdir(captureFileDir)):
        PrintErrorAndExit('Capture file output directory ' + captureFileDir + ' is not a valid directory')


######################
# Set env variables for capture layer
def SetEnvVars(args):

    # Set VK_INSTANCE_LAYERS
    # If gfxr layer is not already in VK_INSTANCE_LAYER, append gfxr layer to VK_INSTANCE_LAYERS
    if os.getenv('VK_INSTANCE_LAYERS') is None:
        os.environ['VK_INSTANCE_LAYERS'] = 'VK_LAYER_LUNARG_gfxreconstruct'
    elif (not ('VK_LAYER_LUNARG_gfxreconstruct' in os.getenv('VK_INSTANCE_LAYERS'))):
        os.environ['VK_INSTANCE_LAYERS'] = os.environ['VK_INSTANCE_LAYERS'] + os.pathsep + 'VK_LAYER_LUNARG_gfxreconstruct'

    # Set GFXRECON_* capture options
    # The capture layer will validate these options and generate errors as needed
    SetEnvVar('GFXRECON_CAPTURE_FILE', os.path.abspath(args.captureFile))
    SetEnvVar('GFXRECON_CAPTURE_FRAMES', args.captureFrames)
    SetEnvVar('GFXRECON_CAPTURE_FILE_TIMESTAMP', args.fileTimestamp)
    SetEnvVar('GFXRECON_CAPTURE_TRIGGER', args.trigger)
    SetEnvVar('GFXRECON_CAPTURE_COMPRESSION_TYPE', args.compressionType)
    SetEnvVar('GFXRECON_CAPTURE_FILE_FLUSH', args.fileFlush)
    SetEnvVar('GFXRECON_LOG_LEVEL', args.logLevel)
    SetEnvVar('GFXRECON_LOG_FILE', args.logFile)
    SetEnvVar('GFXRECON_LOG_OUTPUT_TO_OS_DEBUG_STRING', args.logDebugView)
    SetEnvVar('GFXRECON_MEMORY_TRACKING_MODE', args.memoryTrackingMode)


######################
# PrintEnvVar - for debugging
# print a single env var
def PrintEnvVar(envVar):
    if envVar in os.environ:
        print(envVar, os.environ[envVar])
    else:
        print(envVar, 'None')


######################
# PrintLayerEnv - for debugging
# print all env vars used by layer that are set by this script
def PrintLayerEnv():
    PrintEnvVar('GFXRECON_CAPTURE_COMPRESSION_TYPE')
    PrintEnvVar('GFXRECON_CAPTURE_FILE')
    PrintEnvVar('GFXRECON_CAPTURE_FILE_FLUSH')
    PrintEnvVar('GFXRECON_CAPTURE_FILE_TIMESTAMP')
    PrintEnvVar('GFXRECON_CAPTURE_FRAMES')
    PrintEnvVar('GFXRECON_CAPTURE_TRIGGER')
    PrintEnvVar('GFXRECON_LOG_FILE')
    PrintEnvVar('GFXRECON_LOG_LEVEL')
    PrintEnvVar('GFXRECON_LOG_OUTPUT_TO_OS_DEBUG_STRING')
    PrintEnvVar('GFXRECON_MEMORY_TRACKING_MODE')
    PrintEnvVar('VK_INSTANCE_LAYERS')


if __name__ == '__main__':

    # We don't support running under Cygwin Python
    if sys.platform == 'cygwin':
        PrintErrorAndExit("Cygwin Python not supported")

    # Get and validate args
    parser = ParseArgs()
    args = parser.parse_args()
    #PrintArgs(args)   # For debugging
    ValidateArgs(args)

    # Set up environment
    SetEnvVars(args)
    #PrintLayerEnv()    # For debugging

    # If workingDir was specified, make it the cwd
    if args.workingDir is not None:
        os.chdir(args.workingDir)

    # Run the program and and exit with the exit status of the program
    print('Executing program', GetCommandPath(args))
    result = subprocess.run(args.programAndArgs)
    sys.exit(result.returncode)
