#
# $Id: Makefile.am,v 1.28 2004/01/18 22:11:11 troth Exp $
#
# simulavr - A simulator for the Atmel AVR family of microcontrollers.
# Copyright (C) 2001, 2002, 2003, 2004  Theodore A. Roth
# Copyright (C) 2020 Peter Popovec <popovec.peter@gmail.com>
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
# Makefile.am -- Process this file with automake to produce Makefile.in
#

EXTRA_DIST           = bootstrap Makefile_AVR_Rules README.gdb README.opcodes \
                       ProjSummary misc simulavr-oseid.1.in \
                       README.cygwin \
                       ChangeLog-2003 \
                       ChangeLog-2002 \
                       ChangeLog-2001

man_MANS             = simulavr-oseid.1

SUBDIRS              = src @ac_test_dirs@ @ac_doc_subdir@ @ac_regression_subdir@
DIST_SUBDIRS         = src test_c test_asm doc regress

MAINTAINERCLEANFILES = Makefile.in aclocal.m4 configure src/config-h.in \
                       src/stamp-h.in

install_aux_files    = AUTHORS ChangeLog COPYING INSTALL ProjSummary \
                       README README.gdb README.opcodes TODO \
                       ChangeLog-2003 \
                       ChangeLog-2002 \
                       ChangeLog-2001

DOC_INST_DIR         = @ac_doc_inst_dir@

install-data-local:
	$(mkinstalldirs) $(DOC_INST_DIR)
	for file in $(install_aux_files) ; do \
	  echo " $(INSTALL_DATA) $$file $(DOC_INST_DIR)/$$file"; \
	  $(INSTALL_DATA) $(srcdir)/$$file $(DOC_INST_DIR)/$$file; \
	done

uninstall-local:
	rm -rf $(DOC_INST_DIR)

pdf ps:
	(cd doc && $(MAKE) $(AM_MAKEFLAGS) $@)

install-pdf:
	(cd doc && $(MAKE) $(AM_MAKEFLAGS) install-pdf)

dist-hook: simulavr.spec
	cp simulavr.spec $(distdir)/simulavr.spec
	rm -rf $(distdir)/misc/CVS
	rm -rf $(distdir)/gdb-patches/CVS

