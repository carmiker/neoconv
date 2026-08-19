AC_PREREQ([2.69])
AC_INIT([neoconv],[0.1.0])
AC_CONFIG_AUX_DIR([.])
AC_CONFIG_SRCDIR([src/neoconv.c])
AM_INIT_AUTOMAKE([foreign -Wall subdir-objects])

AC_PROG_CC
AM_PROG_AR
LT_INIT
PKG_PROG_PKG_CONFIG

AC_CHECK_HEADERS([ctype.h dirent.h fnmatch.h stdarg.h stddef.h stdint.h \
	stdio.h stdlib.h string.h sys/stat.h sys/types.h])

PKG_CHECK_MODULES([MINIZ],[miniz],[HAVE_MINIZ=1],[HAVE_MINIZ=0])
AM_CONDITIONAL([HAVE_MINIZ],[test "x$HAVE_MINIZ" = "x1"])

AC_CONFIG_FILES([libneoconv.pc Makefile])
AC_OUTPUT
