#
# $Id: Makefile.am,v 1.27 2004/04/16 04:33:51 troth Exp $
#
# simulavr - A simulator for the Atmel AVR family of microcontrollers.
# Copyright (C) 2001, 2002, 2003  Theodore A. Roth
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

MAINTAINERCLEANFILES = Makefile.in

SUBDIRS              = getopt @ac_curses_disp_dirs@ disp-vcd
DIST_SUBDIRS         = getopt disp disp-vcd

AM_CFLAGS            = @ENABLE_WARNINGS@ \
                       -I$(top_srcdir)/src/getopt

bin_PROGRAMS         = simulavr
simulavr_LDADD       = getopt/libgnugetopt.a
simulavr_SOURCES     = \
	adc.c              \
	adc.h              \
	avrclass.c         \
	avrclass.h         \
	avrcore.c          \
	avrcore.h          \
	avrerror.c         \
	avrerror.h         \
	avrmalloc.c        \
	avrmalloc.h        \
	callback.c         \
	callback.h         \
	decoder.c          \
	decoder.h          \
	device.c           \
	devsupp.c          \
	devsupp.h          \
	display.c          \
	display.h          \
	eeprom.c           \
	eeprom.h           \
	flash.c            \
	flash.h            \
	gdb.h              \
	gdbserver.c        \
	intvects.c         \
	intvects.h         \
	main.c             \
	memory.c           \
	memory.h           \
	op_names.c         \
	op_names.h         \
	ports.c            \
	ports.h            \
	register.c         \
	register.h         \
	sig.c              \
	sig.h              \
	spi.c              \
	spi.h              \
	sram.c             \
	sram.h             \
	stack.c            \
	stack.h            \
	storage.c          \
	storage.h          \
	timers.c           \
	timers.h           \
	uart.c             \
	uart.h             \
	usb.c              \
	usb.h              \
	utils.c            \
	utils.h            \
	vdevs.h            \
	defn/90s1200.h     \
	defn/90s2313.h     \
	defn/90s4414.h     \
	defn/90s8515.h     \
	defn/mega8.h       \
	defn/mega16.h      \
	defn/mega103.h     \
	defn/mega128.h     \
	defn/43usb320.h    \
	defn/43usb325.h    \
	defn/43usb326.h    \
	defn/43usb351.h    \
	defn/43usb353.h    \
	defn/43usb355.h

links:
	@for i in `./simulavr -L`; do ln -s simulavr $$i; done

