#!/usr/bin/perl

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

use v5.10.1;
no warnings 'experimental::smartmatch';

# Assembler header file.
my $hfile = "src/aarch64/assembler-aarch64.h";

# Extra pseudo instructions added to AArch64.
my @extras = qw/bind debug dci dc32 dc64 place/;

my %inst = ();  # Global hash of instructions.

# Set record separator to one or more consecutive new lines. This causes $_ to
# be assigned a 'paragraph' of text for each iteration of the while loop.
$/ = '';

open(IN, "<$hfile") or die("Can't open header file $hfile.\n");
while(<IN>)
{
  # Find a function formatted like an instruction.
  if(my($t) = /^  ((?:void|inline void) [a-z][a-z0-9]{0,8}_?)\(/mgp)
  {
    # Everything before the function match, ie. the comments.
    my $before = ${^PREMATCH};

    # Everything after the function match, ie. arguments to the function, if
    # any, and the closing parenthesis and semi-colon.
    my $after = ${^POSTMATCH};

    # Extract the instruction.
    my($i) = $t =~ /(?:void|inline void) ([a-z][a-z0-9]{0,8})/;

    # Extract the comment from before the function. Drop comment characters
    # and format the architecture version suffix, if present.
    my $d = $before;
    $d =~ s|^  // ||;   # Delete comment chars from first line.
    $d =~ s|\n  //||g;  # Delete comment chars from subsequent lines.
    $d =~ s|\n$||;      # Delete trailing new line.
    $d =~ s|\[Armv(.+)\]|_\(Armv$1\)_|gi;

    # Drop any templating that may have prefixed the prototype, and has now
    # found its way into the description.
    $d =~ s|\n  template <.*>$||g;

    # Extract and tidy up the function prototype.
    my($p) = $after =~ /(.*?\))/ms;
    $p =~ s/\n/\n  /g;
    $p = "$t(".$p;

    # Establish the type of the instruction.
    my $type = 'integer';
    ($p =~ /VRegister/) and $type = 'float';
    ($i ~~ @extras) and $type = 'pseudo';

    # Special case to distinguish dc() the data constant placing function from
    # dc() the data cache maintenance instruction.
    if (($i eq 'dc') and ($p =~ /\(T data\)/)) {
      $type = 'pseudo';
    }

    # Push the results into a hash keyed by prototype string.
    $inst{$p}->{'type'} = $type;
    $inst{$p}->{'mnemonic'} = $i;
    $inst{$p}->{'description'} = $d;
  }
}
close(IN);

print <<HEADER;
VIXL Supported Instruction List
===============================

This is a list of the AArch64 instructions supported by the VIXL assembler,
disassembler and simulator. The simulator may not support all floating point
operations to the precision required by AArch64 - please check the simulator
source code for details.

HEADER

print describe_insts('AArch64 integer instructions', 'integer');
print describe_insts('AArch64 floating point and NEON instructions', 'float');
print describe_insts('Additional or pseudo instructions', 'pseudo');

# Sort instructions by mnemonic and then description.
sub inst_sort
{
  $inst{$a}->{'mnemonic'} cmp $inst{$b}->{'mnemonic'} ||
  $inst{$a}->{'description'} cmp $inst{$b}->{'description'} ||
  $a cmp $b;
}

# Return a Markdown formatted list of instructions of a particular type.
sub describe_insts
{
  my($title, $type) = @_;
  my $result = '';
  $result .= "$title\n";
  $result .= '-' x length($title);
  $result .= "\n\n";

  foreach my $i (sort inst_sort keys(%inst))
  {
    next if($inst{$i}->{'type'} ne $type);
    $result .= sprintf("### %s ###\n\n%s\n\n",
                       uc($inst{$i}->{'mnemonic'}),
                       $inst{$i}->{'description'});
    $result .= "    $i\n\n\n";
  }
  $result .= "\n";
  return $result
}


