#
# $Id: Makefile.am,v 1.13 2003/09/26 22:07:34 troth Exp $
#
###############################################################################
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
###############################################################################


MAINTAINERCLEANFILES = Makefile.in stamp-vti

CLEANFILES           = *.html *.pdf *.ma *.mas *.ps

EXTRA_DIST           = dox.css dox_html_header dox_html_footer \
                       main.dox

info_TEXINFOS        = simulavr.texi

SUFFIXES             = .pdf

DOC_INST_DIR         = @ac_doc_inst_dir@

all-local: html $(TARGET_PS) $(TARGET_PDF)

# Rule for generating postscript output.
ps: simulavr.ps $(TARGET_DOX_PS)

install-ps: ps $(INSTALL_DOX_PS)
	$(mkinstalldirs) $(DOC_INST_DIR)
	$(INSTALL_DATA) simulavr.ps $(DOC_INST_DIR)/simulavr.ps

# Rule for generating html output.
html: $(info_TEXINFOS) $(TARGET_DOX_HTML)
	mkdir -p html
	texi2html -split_node $(srcdir)/$(info_TEXINFOS)
	mv *.html html
	if test -f html/simulavr.html ; then \
	   cp html/simulavr.html html/index.html ; \
	else \
	   if test -f html/simulavr_toc.html ; then \
	      cp html/simulavr_toc.html html/index.html ; \
	   fi \
	fi

install-html: html $(INSTALL_DOX_HTML)
	$(mkinstalldirs) $(DOC_INST_DIR)/html
	@list='$(wildcard html/*.html)' ; \
	for file in $$list ; do \
	  echo " $(INSTALL_DATA) $$file $(DOC_INST_DIR)/$$file"; \
	  $(INSTALL_DATA) $$file $(DOC_INST_DIR)/$$file; \
	done

# General rule for installing documentation
install-data-local: install-html install-ps $(INSTALL_PDF)

uninstall-local:
	rm -rf $(DOC_INST_DIR)

clean-local:
	rm -rf html internals_html internals

#
# I'm not generating PDF files by default. pdftex seems to
# generate some warnings I don't like and can't figure out
# how to get rid of. The generated pdf file seems to be alright
# though, so you can still get a pdf file via `make pdf`.
#

# Rule for generating PDF output
pdf: simulavr.pdf $(TARGET_DOX_PDF)

simulavr.pdf: $(info_TEXINFOS)

.texi.pdf:
	texi2pdf $<

install-pdf: pdf $(INSTALL_DOX_PDF)
	$(mkinstalldirs) $(DOC_INST_DIR)
	$(INSTALL_DATA) simulavr.pdf $(DOC_INST_DIR)/simulavr.pdf

#
# Rules for generating documentation with doxygen
#

dox: internals_html internals

internals_html internals:
	doxygen doxygen.config

dox-html: internals_html

dox-pdf: internals
	${MAKE} -C internals pdf
	cp internals/refman.pdf simulavr-internals.pdf

dox-ps: internals
	${MAKE} -C internals ps
	cp internals/refman.ps simulavr-internals.ps

install-dox-html: internals_html
	$(mkinstalldirs) $(DOC_INST_DIR)/internals_html
	@list='$(wildcard internals_html/*.html internals_html/*.css)' ; \
	for file in $$list ; do \
	  echo " $(INSTALL_DATA) $$file $(DOC_INST_DIR)/$$file"; \
	  $(INSTALL_DATA) $$file $(DOC_INST_DIR)/$$file; \
	done

install-dox-pdf: dox-pdf
	$(mkinstalldirs) $(DOC_INST_DIR)
	$(INSTALL_DATA) internals/refman.pdf $(DOC_INST_DIR)/simulavr-internals.pdf

install-dox-ps: dox-ps
	$(mkinstalldirs) $(DOC_INST_DIR)
	$(INSTALL_DATA) internals/refman.ps $(DOC_INST_DIR)/simulavr-internals.ps

.PHONY: ps pdf dox dox-pdf install-pdf install-html install-ps install-dox-html \
	install-dox-ps install-dox-pdf
