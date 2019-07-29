#! /usr/bin/env python
###############################################################################
#
# simulavr - A simulator for the Atmel AVR family of microcontrollers.
# Copyright (C) 2001, 2002  Theodore A. Roth
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
# $Id: avr_target.py,v 1.1 2002/02/05 01:04:20 troth Exp $
#

import sys, array, signal
import gdb_rsp

class AvrTarget(gdb_rsp.GdbRemoteSerialProtocol):
	offset_flash = 0x0
	offset_sram  = 0x00800000
	
	def __init__(self, host='localhost', port=1212, ofile=None):
		gdb_rsp.GdbRemoteSerialProtocol.__init__(self,host,port,ofile)

	def read_flash(self, addr, _len):
		return self.read_mem( addr+self.offset_flash, _len )

	def write_flash(self, addr, _len, buf):
		self.write_mem( addr+self.offset_flash, _len, buf )

	def read_sram(self, addr, _len):
		return self.read_mem( addr+self.offset_sram, _len )

	def write_sram(self, addr, _len, buf):
		self.write_mem( addr+self.offset_sram, _len, buf )

	def load_binary(self, file):
		f = open(file)
		bin = array.array('B', f.read())
		self.write_flash(0x0, len(bin), bin)
		f.close()

	def reset(self):
		self.cont_with_signal(signal.SIGHUP)

if __name__ == '__main__':
	# Open a connection to the target
	target = AvrTarget(ofile=sys.stderr)

	demo = '/home/troth/develop/avr/sav/build-sim-debug/test_c/demo.bin'
	target.load_binary(demo)

	target.close()
