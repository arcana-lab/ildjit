AC_INIT(optimizer-ddg, 0.3.0, simo.xan@gmail.com)
AM_INIT_AUTOMAKE
AM_PROG_AR
AM_CONFIG_HEADER(src/config.h)
AC_SUBST(VERSION)
ISODATE=`date +%Y-%m-%d`
AC_SUBST(ISODATE)
if test "${prefix}" == "NONE" ; then
	prefix=/usr/local
fi
AC_ARG_ENABLE(debug, [  --enable-debug    Enable debug compilation])
AC_ARG_ENABLE(optdebug, [  --enable-optdebug    Enable optimised debug compilation])
AC_ARG_ENABLE(printdebug, [  --enable-printdebug    Enable the print debug and the debug compilation])
AC_ARG_ENABLE(smalllib_debug, [  --enable-smalllib-debug   Enable debug compilation in small lib])
AC_ARG_ENABLE(memuse_debug, [  --enable-memuse-debug   Enable memory usage debug compilation])
AC_ARG_ENABLE(profile, [  --enable-profile    Enable the compilation for the automatically profiler tools])
AC_ARG_ENABLE(callgrind, [  --enable-callgrind    "Enable compilation for analysis with valgrind's callgrind tool"])
AM_CONDITIONAL(DEBUG, test "$enable_debug" = "yes")
AM_CONDITIONAL(OPTDEBUG, test "$enable_optdebug" = "yes")
AM_CONDITIONAL(PRINTDEBUG, test "$enable_printdebug" = "yes")
AM_CONDITIONAL(SMALLLIB_DEBUG, test "$enable_smalllib_debug" = "yes")
AM_CONDITIONAL(MEMUSE_DEBUG, test "$enable_memuse_debug" = "yes")
AM_CONDITIONAL(PROFILE, test "$enable_profile" = "yes")
AM_CONDITIONAL(CALLGRIND, test "$enable_callgrind" = "yes")
AC_DEFINE_UNQUOTED(PREFIX,		"${prefix}",				[Prefix directory])
AC_CANONICAL_HOST

##dnl Checks for programs.
AC_PROG_INSTALL
AC_PROG_CC
AC_PROG_LIBTOOL

##############################################################################################################################
#						Initialize compiler default options
##############################################################################################################################
AM_INIT_AUTOMAKE(-Wall -Werror)
CFLAGS="$CFLAGS -Wall"
AC_SUBST(CFLAGS)

##############################################################################################################################
#						Checks for libraries.
##############################################################################################################################
PKG_CHECK_MODULES(COMPILERMEMORYMANAGER, libcompilermemorymanager >= 1.0.0)
AC_SUBST(COMPILERMEMORYMANAGER_CFLAGS)
AC_SUBST(COMPILERMEMORYMANAGER_LIBS)

PKG_CHECK_MODULES(ILJITIROPTIMIZER, libiljitiroptimizer >= 1.0.0)
AC_SUBST(ILJITIROPTIMIZER_CFLAGS)
AC_SUBST(ILJITIROPTIMIZER_LIBS)

PKG_CHECK_MODULES(ILJITIR, libirmanager >= 1.0.0)
AC_SUBST(ILJITIR_CFLAGS)
AC_SUBST(ILJITIR_LIBS)

PKG_CHECK_MODULES(ILJITU, libiljitu >= 1.0.0)
AC_SUBST(ILJITU_CFLAGS)
AC_SUBST(ILJITU_LIBS)

PKG_CHECK_MODULES(XAN, libxan >= 1.0.0)
AC_SUBST(XAN_CFLAGS)
AC_SUBST(XAN_LIBS)

PKG_CHECK_MODULES(CHIARA, libchiara >= 0.1.0)
AC_SUBST(CHIARA_CFLAGS)
AC_SUBST(CHIARA_LIBS)

##############################################################################################################################
# 						Checks for header files.
##############################################################################################################################
AC_HEADER_STDC
AC_CHECK_HEADERS(stdio.h assert.h errno.h)

AC_OUTPUT(
	Makefile
	src/Makefile
)
