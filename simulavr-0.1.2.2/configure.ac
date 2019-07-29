#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
#
# $Id: configure.ac,v 1.43 2004/09/15 15:17:40 troth Exp $
#
###############################################################################
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
###############################################################################

AC_PREREQ(2.57)
AC_INIT(simulavr, 0.1.2.2, simulavr-devel@nongnu.org)
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADER([src/config.h:src/config-h.in])

AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC
AC_PROG_LEX
AC_PROG_YACC
AC_PROG_LN_S
AC_PROG_RANLIB

AC_CHECK_PROG(AVR_AS, avr-as, avr-as)
if test -z "$AVR_AS"; then
  AC_MSG_ERROR([no acceptable avr assembler found in \$PATH])
fi
AC_CHECK_PROG(AVR_LD, avr-ld, avr-ld)
if test -z "$AVR_LD"; then
  AC_MSG_ERROR([no acceptable avr linker found in \$PATH])
fi
AC_CHECK_PROG(AVR_OBJCOPY, avr-objcopy, avr-objcopy)
if test -z "$AVR_OBJCOPY"; then
  AC_MSG_ERROR([no acceptable avr objcopy found in \$PATH])
fi
AC_CHECK_PROG(AVR_OBJDUMP, avr-objdump, avr-objdump)
if test -z "$AVR_OBJDUMP"; then
  AC_MSG_ERROR([no acceptable avr objdump found in \$PATH])
fi
AC_CHECK_PROG(AVR_NM, avr-nm, avr-nm)
if test -z "$AVR_NM"; then
  AC_MSG_ERROR([no acceptable avr nm found in \$PATH])
fi
AC_SUBST([AVR_AS])
AC_SUBST([AVR_LD])
AC_SUBST([AVR_OBJCOPY])
AC_SUBST([AVR_OBJDUMP])
AC_SUBST([AVR_NM])

AC_CHECK_PROG([AVR_CC], [avr-gcc], [avr-gcc])
if test -z "$AVR_CC"; then
  AC_MSG_ERROR([no acceptable cc found in \$PATH])
fi
AC_SUBST([AVR_CC])

dnl This macro searches for python version 2.1.1 or newer
AC_CACHE_CHECK(for python >= 2.1.1, _cv_python_211, [

changequote(<@, @>)

_cv_python_211='no' ;
if python -V 2>&1 | grep -q Python ; then
  cat <<EOF > py_ver.py
import sys
v = sys.version_info
ver = (int(v[0]) << 16) + (int(v[1]) << 8) + int(v[2])
if ver < 0x020101:
	print 'no'
else:
	print 'yes'
EOF
  _cv_python_211=`python py_ver.py`
  rm -f py_ver.py
fi

changequote([, ])
])

if test "x$_cv_python_211" = "xyes" ; then
  ac_regression_subdir="regress"
else
  AC_MSG_WARN([ ])
  AC_MSG_WARN([Python >= 2.1.1 not found.])
  AC_MSG_WARN([Regression tests will not be run.])
  AC_MSG_WARN([ ])
fi
AC_SUBST([ac_regression_subdir])
AM_CONDITIONAL(COND_HAS_PYTHON, test "x$_cv_python_211" = "xyes")

dnl This macro searches for a GNU version of make.  If a match is found, the
dnl makefile variable `ifGNUmake' is set to the empty string, otherwise it is
dnl set to "#".  This is useful for  including a special features in a Makefile,
dnl which cannot be handled by other versions of make.  The variable
dnl _cv_gnu_make_command is set to the command to invoke GNU make if it exists,
dnl the empty string otherwise.
dnl
dnl Here is an example of its use:
dnl
dnl Makefile.in might contain:
dnl
dnl     # A failsafe way of putting a dependency rule into a makefile
dnl     $(DEPEND):
dnl             $(CC) -MM $(srcdir)/*.c > $(DEPEND)
dnl
dnl     @ifGNUmake@ ifeq ($(DEPEND),$(wildcard $(DEPEND)))
dnl     @ifGNUmake@ include $(DEPEND)
dnl     @ifGNUmake@ endif
dnl
dnl Then configure.in would normally contain:
dnl
dnl     CHECK_GNU_MAKE()
dnl     AC_OUTPUT(Makefile)
dnl
dnl Then perhaps to cause gnu make to override any other make, we could do
dnl something like this (note that GNU make always looks for GNUmakefile first):
dnl
dnl     if  ! test x$_cv_gnu_make_command = x ; then
dnl             mv Makefile GNUmakefile
dnl             echo .DEFAULT: > Makefile ;
dnl             echo \  $_cv_gnu_make_command \$@ >> Makefile;
dnl     fi
dnl
dnl Then, if any (well almost any) other make is called, and GNU make also exists,
dnl then the other make wraps the GNU make.
dnl
dnl @author John Darrington <j.darrington@elvis.murdoch.edu.au>
dnl @version $Id: configure.ac,v 1.43 2004/09/15 15:17:40 troth Exp $
dnl
AC_CACHE_CHECK([for GNU make], [_cv_gnu_make_command], [
  _cv_gnu_make_command='' ;
  dnl Search all the common names for GNU make
  for a in "$MAKE" make gmake gnumake ; do
    if  ( $a --version 2> /dev/null | grep  -q GNU  ) ;  then
      _cv_gnu_make_command=$a ;
      break;
    fi
    done ;
])
dnl
dnl If there was a GNU version, then set @ifGNUmake@ to the empty string,
dnl '#' otherwise
if test  "x$_cv_gnu_make_command" != "x"  ; then
  ifGNUmake=""
else
  ifGNUmake="#"
fi
AC_SUBST([ifGNUmake])

dnl This macro check to see if texinfo is installed.
dnl If texinfo is not found, documentation will not be built.
dnl
dnl This should check for all of the programs which are needed to produce
dnl all of the various documentation formats.
dnl
AC_CHECK_PROG(has_makeinfo, makeinfo, "yes")
AC_CHECK_PROG(has_texi2dvi, texi2dvi, "yes")
AC_CHECK_PROG(has_texindex, texindex, "yes")
if test -n "$has_makeinfo" && test -n "$has_texi2dvi" && test -n "$has_texindex"; then
	ac_doc_subdir="doc"
else
	AC_MSG_WARN([ ])
	AC_MSG_WARN([You don not seem to have texinfo installed on your system.])
	AC_MSG_WARN([Documentation will not be build. If you wish to build the])
	AC_MSG_WARN([documentation files, you will have to install texinfo-4.0b])
	AC_MSG_WARN([or newer.])
	AC_MSG_WARN([ ])
fi
dnl Checks for some programs in the tetex package.
if test -n "$ac_doc_subdir"; then
	AC_CHECK_PROG(has_tex, tex, "yes")
	AC_CHECK_PROG(has_pdftex, pdftex, "yes")
	AC_CHECK_PROG(has_texi2html, texi2html, "yes")
	if test -z "$has_tex" || test -z "$has_pdftex" || test -z "$has_texi2html";
	   then
		ac_doc_subdir=""
		AC_MSG_WARN([ ])
		AC_MSG_WARN([You do not seem to have tetex installed on your system.])
		AC_MSG_WARN([Documentation will not be build. If you wish to build the])
		AC_MSG_WARN([documentation files, you will have to install tetex.])
		AC_MSG_WARN([ ])
	fi
fi
dnl Checks for programs in the tetex-dvips package.
if test -n "$ac_doc_subdir"; then
	AC_CHECK_PROG(has_dvips, dvips, "yes")
	if test -z "$has_dvips"; then
		ac_doc_subdir=""
		AC_MSG_WARN([ ])
		AC_MSG_WARN([You do not seem to have dvips installed on your system.])
		AC_MSG_WARN([Documentation will not be build. If you wish to build the])
		AC_MSG_WARN([documentation files, you will have to install dvips.])
		AC_MSG_WARN([ ])
	fi
fi
AC_SUBST([ac_doc_subdir])

# Checks for libraries.

dnl This macro defines a user switch to enable building of the curses
dnl display interface:
dnl
dnl    ./configure --enable-curses
dnl
dnl The default behavior is enabled.
dnl
dnl The dir_list argument is a space separated list of sources directories
dnl used to build the curses interface programs.
dnl
dnl If enabled, this macro will check to see if a usable curses library 
dnl can be found.
dnl
AC_DEFUN([TROTH_ENABLE_CURSES], [
dnl
AC_MSG_CHECKING([if user wants a curses display])
AC_ARG_ENABLE([curses],
[  --disable-curses        do not build with curses display (same as --enable-curses=no)], [
if test "$enable_curses" = "" -o "$enable_curses" = "yes"; then
	ac_curses_disp_dirs="$1"
	AC_MSG_RESULT(yes)
else
	AC_MSG_RESULT(no, curses display program will not be built)
fi
], [
ac_curses_disp_dirs="$1"
AC_MSG_RESULT(yes)
])
if test -n "$ac_curses_disp_dirs"; then
	AC_CHECK_LIB(ncurses, initscr,
		CursesLib=ncurses
		AC_DEFINE(HAS_NCURSES, 1, System has ncurses library))
dnl if we can't find ncurses, try curses
	if test -z "$CursesLib"; then
		AC_CHECK_LIB(curses, initscr, CursesLib=curses)
	fi
dnl Make sure that we found a curses library.
	if test -n "$CursesLib"; then
		AC_CHECK_LIB($CursesLib, resizeterm,
			AC_DEFINE(HAS_CURSES_RESIZETERM, 1, Curses library has resizeterm function),
			AC_MSG_ERROR([Your curses library lacks the resizeterm function.
  You have two choices:
    1: upgrade your curses library.
    2: disable curses by configuring with the '--disable-curses' option.
		]))

		ac_lib_curses="-l$CursesLib"
		AC_DEFINE(USE_CURSES, 1, Build support for curses display)
		AC_SUBST([ac_curses_disp_dirs])
		AC_SUBST([ac_lib_curses])
	else
		AC_MSG_WARN([ ])
		AC_MSG_WARN([No curses library found])
		AC_MSG_WARN([Curses display program will not be built])
		AC_MSG_WARN([ ])
	fi
fi
])
TROTH_ENABLE_CURSES(disp)

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([arpa/inet.h fcntl.h netinet/in.h stdlib.h string.h strings.h sys/ioctl.h sys/socket.h sys/time.h termios.h unistd.h inttypes.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_C_VOLATILE

dnl Available from the GNU Autoconf Macro Archive at:
dnl http://www.gnu.org/software/ac-archive/htmldoc/type_socklen_t.html
dnl
AC_CACHE_CHECK([for socklen_t], ac_cv_type_socklen_t,
[
  AC_TRY_COMPILE(
  [#include <sys/types.h>
   #include <sys/socket.h>],
  [socklen_t len = 42; return 0;],
  ac_cv_type_socklen_t=yes,
  ac_cv_type_socklen_t=no)
])
if test $ac_cv_type_socklen_t != yes; then
  AC_DEFINE([socklen_t], [int], [Define missing socklen_t.])
fi

# Checks for library functions.
AC_FUNC_FORK
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([atexit gettimeofday inet_ntoa memset putenv select socket strdup strerror strrchr strtol])

# Documentation checks

AC_ARG_ENABLE(versioned-doc,
[  --enable-versioned-doc  install docs in directory with version name (default)],
[case "${enableval}" in
  yes) versioned_doc=yes ;;
  no)  versioned_doc=no ;;
  *)   AC_MSG_ERROR(bad value ${enableval} for versioned-doc option) ;;
 esac], [versioned_doc=yes])

if test "$versioned_doc" = "yes"; then
    ac_doc_inst_dir='$(DESTDIR)$(datadir)/doc/simulavr-$(VERSION)'
else
    ac_doc_inst_dir='$(DESTDIR)$(datadir)/doc/simulavr'
fi
AC_SUBST(ac_doc_inst_dir)

dnl This macro defines a user switch to enable building of the documentation
dnl in pdf format:
dnl
dnl    ./configure --enable-pdf
dnl                     
dnl The default behavior is disabled.
AC_MSG_CHECKING(if user wants to build the documentation in pdf format)
AC_ARG_ENABLE(pdf,
[  --enable-pdf            enable generation of the documentation in pdf format],
if test "$enable_pdf" = "yes"; then
   AC_MSG_RESULT(yes)
   TARGET_PDF=pdf
   INSTALL_PDF=install-pdf
else
   AC_MSG_RESULT(no)
fi
,
AC_MSG_RESULT(no)
)
AC_SUBST(TARGET_PDF)
AC_SUBST(INSTALL_PDF)

dnl This macro defines a user switch to enable building of the documentation
dnl in ps format (including doxygen):
dnl
dnl    ./configure --enable-ps
AC_MSG_CHECKING(if user wants to build the documentation in ps format)
AC_ARG_ENABLE(ps,
[  --enable-ps,            enable generation of the documentation in ps format],
if test "$enable_ps" = "yes"; then
   AC_MSG_RESULT(yes)
   TARGET_PS=ps
   INSTALL_PS=install-ps
else
   AC_MSG_RESULT(no)
fi
,
AC_MSG_RESULT(no)
)
AC_SUBST(TARGET_PS)
AC_SUBST(INSTALL_PS)

dnl This macro checks if doxygen is installed on the build system
dnl
dnl inherits --enable-ps and --enable-pdf options
dnl (thus, should be called after AVR_DOC_{PS,PDF})
dnl
AC_DEFUN([CHECK_DOXYGEN],[
AC_MSG_CHECKING(for doxygen)
dox_ver=`doxygen --version 2>/dev/null`
if test "x$dox_ver" = "x"; then
	AC_MSG_RESULT(no)
else
	# FIXME: should also check for dox_ver >= 1.3.3
	AC_MSG_RESULT(yes)
	TARGET_DOX_PS=${TARGET_PS:+dox-ps}
	TARGET_DOX_PDF=${TARGET_PDF:+dox-pdf}
	TARGET_DOX_HTML=dox-html
	INSTALL_DOX_PS=${TARGET_DOX_PS:+install-dox-ps}
	INSTALL_DOX_PDF=${TARGET_DOX_PDF:+install-dox-pdf}
	INSTALL_DOX_HTML=install-dox-html
fi
])
CHECK_DOXYGEN
AC_SUBST(TARGET_DOX_PS)
AC_SUBST(TARGET_DOX_PDF)
AC_SUBST(TARGET_DOX_HTML)
AC_SUBST(INSTALL_DOX_PS)
AC_SUBST(INSTALL_DOX_PDF)
AC_SUBST(INSTALL_DOX_HTML)

dnl This macro defines a user switch to disable building of the test
dnl programs as such:
dnl
dnl    ./configure --disable-tests
dnl
dnl The default behavior is enabled.
dnl
dnl The dir_list argument is a space separated list of test directories.
dnl
AC_DEFUN([TROTH_ENABLE_TESTS],[
AC_MSG_CHECKING(if avr test programs will be built)
AC_ARG_ENABLE(tests,
[  --disable-tests         do not build avr tests (same as --enable-tests=no)],
if test "$enable_tests" = "" -o "$enable_tests" = "yes"; then
    ac_test_dirs="$1"
    AC_MSG_RESULT(yes)
else
    ac_test_dirs=""
    AC_MSG_RESULT(no)
fi
,
ac_test_dirs="$1"
AC_MSG_RESULT(yes))
AC_SUBST([ac_test_dirs])

#if test -n "$ac_test_dirs"; then
#    TROTH_PROG_AVR_CC
#    TROTH_PROG_AVR_BINUTILS
#    TROTH_WITH_AVR_INCLUDES
#fi
])

dnl My checks for the avr cross compiler and friends.
TROTH_ENABLE_TESTS([test_c test_asm])

# If we are compiling with gcc, enable all warning and make warnings errors.
if test "$GCC" = yes; then
    ENABLE_WARNINGS="-Wall -Winline -Werror"
fi
AC_SUBST(ENABLE_WARNINGS,$ENABLE_WARNINGS)


AH_BOTTOM([
#if defined HAVE_INTTYPES_H
#include <inttypes.h>
#endif
])

AC_CONFIG_FILES([Makefile
                 doc/Makefile
                 doc/doxygen.config
                 regress/Makefile
                 regress/regress.py
                 regress/modules/Makefile
                 regress/test_opcodes/Makefile
                 src/Makefile
                 src/getopt/Makefile
                 src/disp-vcd/Makefile
                 src/disp/Makefile
                 test_asm/Makefile
                 test_asm/test_8515/Makefile
                 test_c/Makefile
                 simulavr.spec
                 simulavr.1
                 simulavr-disp.1])
AC_OUTPUT
