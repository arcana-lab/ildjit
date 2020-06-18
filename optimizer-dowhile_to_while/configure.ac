AC_INIT(optimizer-dowhile_to_while, 0.1.0, simo.xan@gmail.com)
AM_INIT_AUTOMAKE(optimizer-dowhile_to_while, 0.1.0)
AC_CONFIG_MACRO_DIR([m4])
AM_CONFIG_HEADER(src/config.h)
AC_SUBST(VERSION)
ISODATE=`date +%Y-%m-%d`
AC_SUBST(ISODATE)
if test "${prefix}" == "NONE" ; then
	prefix=/usr/local
fi
AC_ARG_ENABLE(debug, AS_HELP_STRING([--enable-debug], [Enable debug compilation]))
AC_ARG_ENABLE(printdebug, AS_HELP_STRING([--enable-printdebug], [Enable the print debug and the debug compilation]))
AC_ARG_ENABLE(superdebug, AS_HELP_STRING([--enable-superdebug], [Enable the super debug, print debug and debug compilation (prints lots of stuff to the console)]))
AC_ARG_ENABLE(profile, AS_HELP_STRING([--enable-profile], [Enable the compilation for the automatically profiler tools]))
AM_CONDITIONAL(DEBUG, test "$enable_debug" = "yes")
AM_CONDITIONAL(PRINTDEBUG, test "$enable_printdebug" = "yes")
AM_CONDITIONAL(SUPERDEBUG, test "$enable_superdebug" = "yes")
AM_CONDITIONAL(PROFILE, test "$enable_profile" = "yes")
AC_DEFINE_UNQUOTED(PREFIX,		"${prefix}",				[Prefix directory])
AC_CANONICAL_HOST

##############################################################################################################################
#						Initialize compiler default options
##############################################################################################################################
AM_INIT_AUTOMAKE(-Wall -Werror)
CFLAGS="$CFLAGS -Wall"
AC_SUBST(CFLAGS)

##############################################################################################################################
#						Checks for programs.
##############################################################################################################################
AC_PROG_INSTALL
AC_PROG_CC
AC_PROG_LIBTOOL

##############################################################################################################################
#						Checks for libraries.
##############################################################################################################################
PKG_CHECK_MODULES(COMPILERMEMORYMANAGER, libcompilermemorymanager >= 0.0.3)
AC_SUBST(COMPILERMEMORYMANAGER_CFLAGS)
AC_SUBST(COMPILERMEMORYMANAGER_LIBS)

PKG_CHECK_MODULES(ILJITIROPTIMIZER, libiljitiroptimizer >= 0.0.4)
AC_SUBST(ILJITIROPTIMIZER_CFLAGS)
AC_SUBST(ILJITIROPTIMIZER_LIBS)

PKG_CHECK_MODULES(ILJITIR, libirmanager >= 0.0.4)
AC_SUBST(ILJITIR_CFLAGS)
AC_SUBST(ILJITIR_LIBS)

PKG_CHECK_MODULES(ILJITU, libiljitu >= 0.0.4)
AC_SUBST(ILJITU_CFLAGS)
AC_SUBST(ILJITU_LIBS)

PKG_CHECK_MODULES(XAN, libxan >= 0.0.3)
AC_SUBST(XAN_CFLAGS)
AC_SUBST(XAN_LIBS)

##############################################################################################################################
# 						Checks for header files.
##############################################################################################################################
AC_HEADER_STDC
AC_CHECK_HEADERS(stdio.h assert.h errno.h)

AC_OUTPUT(
	Makefile
	src/Makefile
)
