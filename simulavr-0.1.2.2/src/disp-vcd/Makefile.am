#
# $Id: Makefile.am,v 1.3 2003/11/22 20:12:27 troth Exp $
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

bin_PROGRAMS = simulavr-vcd
simulavr_vcd_LDADD = ../getopt/libgnugetopt.a
simulavr_vcd_SOURCES = disp.c \
                       vcd.c \
                       vcd.h \
                       config_parser.y \
                       config_scanner.l

AM_CFLAGS = @ENABLE_WARNINGS@ \
            -I$(top_srcdir)/src \
            -I$(top_srcdir)/src/getopt

# yacc and yacc flags
AM_AM_YFLAGS = -d -p config_

# lex flags
AM_AM_LFLAGS = -Pconfig_ -olex.yy.c
