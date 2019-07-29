#
# $Id: Makefile.am,v 1.5 2003/09/10 04:59:36 troth Exp $
#
# simulavr - A simulator for the Atmel AVR family of microcontrollers.
# Copyright (C) 2001, 2002, 2003,  Theodore A. Roth
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
CLEANFILES           = *.elf *.lst *.bin *.hex

noinst_PROGRAMS      = demo demo_kr deep_frame timer
demo_SOURCES         = demo.c common.h
demo_kr_SOURCES      = demo_kr.c common.h
deep_frame_SOURCES   = deep_frame.c common.h
timer_SOURCES        = timer.c common.h

AVR_TARGET           = -mmcu=atmega128

all-local: \
	demo.lst demo.hex demo.bin \
	demo_kr.lst demo_kr.hex demo_kr.bin \
	deep_frame.lst deep_frame.hex deep_frame.bin \
	timer.lst timer.hex timer.bin


demo.elf: demo
	@cp $< $@

demo.lst demo.hex demo.bin : demo.elf

demo_kr.elf: demo_kr
	@cp $< $@

demo_kr.lst demo_kr.hex demo_kr.bin : demo_kr.elf

deep_frame.elf: deep_frame
	@cp $< $@

deep_frame.lst deep_frame.hex deep_frame.bin : deep_frame.elf

timer.elf: timer
	@cp $< $@

timer.lst timer.hex timer.bin : timer.elf


include $(top_srcdir)/Makefile_AVR_Rules
