#
# $Id: Makefile.am,v 1.2 2002/02/19 20:39:34 troth Exp $
#

MAINTAINERCLEANFILES = Makefile.in stamp-vti

EXTRA_DIST           = README regress.py.in

SUBDIRS              = modules test_opcodes

check-local: regression

clean-local:
	rm -f *.py[co] *.err *.out
	rm -f $(srcdir)/modules/*.py[co]
	rm -f $(srcdir)/test_*/*.py[co]

regression:
if COND_HAS_PYTHON
	python regress.py 2> regress.err | tee regress.out
else
	@echo "  Configure could not find python on your system so regression"
	@echo "  tests can not be automated."
endif
