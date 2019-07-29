#! /usr/bin/env python
###############################################################################
#
# simulavr - A simulator for the Atmel AVR family of microcontrollers.
# Copyright (C) 2004  Theodore A. Roth
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
###############################################################################
#
# $Id: io_gen.py,v 1.1 2004/01/29 05:35:33 troth Exp $
#

#
# This script is for generating the .io_reg field initializer of the
# DevSuppDefn for a specfic device. The input should be one of the io*.h files
# provided by avr-libc in the include/avr/ directory.
#
# Usage: io_gen.py <io_header>
#
# This best way to use this script is while using vim to edit the
# devsupp.c. You can run the script from vim and have the output of the script
# inserted directly into the devsupp.c file with a command like this:
#
#   :r!../misc/io_gen.py ~/dev/tools/avr-libc-cvs/include/avr/io1200.h
#

import sys, re

base_regx = r'[#]define\s*?(\S+)\s*?%s\s*[(]\s*?(\S+?)\s*?[)]'

re_io8 = re.compile (base_regx % ('_SFR_IO8'))
re_mem8 = re.compile (base_regx % ('_SFR_MEM8'))

# Open the input file.

f = open (sys.argv[1]).read ()

register = {}

# Find all the _SFR_IO8 defs.

for name, addr_str in re_io8.findall (f):
    addr = int (addr_str, 0) + 0x20
    register[addr] = name

# Find all the _SFR_MEM8 defs.

for name, addr_str in re_mem8.findall (f):
    addr = int (addr_str, 0)
    register[addr] = name

# Print the field initializer to stdout.

print '    .io_reg = {'
addrs = register.keys ()
addrs.sort ()
for addr in addrs:
    print '        { .addr = 0x%02x, .name = "%s", },' % (addr, register[addr])
print '        /* May need to add SREG, SPL, SPH, and eeprom registers. */'
print '        IO_REG_DEFN_TERMINATOR'
print '    }'
