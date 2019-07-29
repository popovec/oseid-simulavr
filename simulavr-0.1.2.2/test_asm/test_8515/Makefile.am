#
# $Id: Makefile.am,v 1.5 2004/04/17 00:03:51 troth Exp $
#
# simulavr - A simulator for the Atmel AVR family of microcontrollers.
# Copyright (C) 2001, 2002, 2003, 2004  Theodore A. Roth
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

## FIXME: This build file is crap! Need to move all the .asm files to .S and
## handle these in a sane way.

MAINTAINERCLEANFILES = Makefile.in
CLEANFILES           = *.elf *.lst *.bin *.hex $(TEST_PRG)

TEST_ASM_SRC         = \
	test_blink.asm \
	test_cntr.asm \
	test_eeprom.asm \
	test_port.asm \
	test_stack.asm \
	test_toie0.asm \
	test_toie0_2.asm \
	test_wdr.asm \
	test_wdr2.asm

EXTRA_DIST           = $(TEST_ASM_SRC) 8515def.inc

# If we're using gnumake, we can automate creation of the .lst,
# .bin and .hex files. Otherwise, the user will have to do it
# manually.
@ifGNUmake@ TEST_ELF = $(TEST_ASM_SRC:.asm=.elf)
@ifGNUmake@ TEST_LST = $(TEST_ASM_SRC:.asm=.lst)
@ifGNUmake@ TEST_BIN = $(TEST_ASM_SRC:.asm=.bin)
@ifGNUmake@ TEST_HEX = $(TEST_ASM_SRC:.asm=.hex)
@ifGNUmake@ TEST_PRG = $(TEST_ASM_SRC:.asm=)

AVR_TARGET           = -mmcu=at90s8515

all-local: $(TEST_ELF) $(TEST_LST) $(TEST_BIN) $(TEST_HEX)
	for prg in $(TEST_PRG); do \
		$(AVR_LD) -m avr2 -o $$prg $${prg}.elf || exit 1; \
	done

clean-local:
	rm -rf $(TEST_PRG)

include $(top_srcdir)/Makefile_AVR_Rules

## This is how I download to the stk200 using uisp:
#download: dl.hex
#	uisp -dprog=stk200 -dlpt=/dev/parport0 --erase
#	uisp -dprog=stk200 -dlpt=/dev/parport0 --upload if=dl.hex

