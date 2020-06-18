AC_INIT(libirvirtualmachine_libjit, 0.8.0, simo.xan@gmail.com)
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(src/config.h)
AC_SUBST(VERSION)
ISODATE=`date +%Y-%m-%d`
AC_SUBST(ISODATE)
AC_PREFIX_DEFAULT(/usr/local)
if test "${prefix}" == "NONE" ; then
	prefix=/usr/local
fi
AC_ARG_ENABLE(debug, [  --enable-debug    Enable debug compilation])
AC_ARG_ENABLE(printdebug, [  --enable-printdebug    Enable the print debug and the debug compilation])
AC_ARG_ENABLE(profile, [  --enable-profile    Enable the compilation for the automatically profiler tools])
AM_CONDITIONAL(DEBUG, test "$enable_debug" = "yes")
AM_CONDITIONAL(PRINTDEBUG, test "$enable_printdebug" = "yes")
AM_CONDITIONAL(PROFILE, test "$enable_profile" = "yes")
AC_DEFINE_UNQUOTED(PREFIX,		"${prefix}",				[Prefix directory])
AC_DEFINE_UNQUOTED(DATADIR,		"${prefix}/share",			[Data directory])
AC_CANONICAL_HOST

AC_ARG_ENABLE([api-doc],
              AS_HELP_STRING([--enable-api-doc], [generate api documentation]))
AM_CONDITIONAL(API_DOC, test "$enable_api_doc" = "yes")

##############################################################################################################################
#						Initialize compiler default options
##############################################################################################################################
AM_INIT_AUTOMAKE(-Wall -Werror)
CFLAGS="$CFLAGS -Wall"
AC_SUBST(CFLAGS)

##############################################################################################################################
##						Checks for programs.
##############################################################################################################################
AC_PROG_INSTALL
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_YACC
AC_CHECK_PROGS(XYACC,$YACC bison yacc,none)
if test "$XYACC" = "none" ; then
	echo "*** Error: No suitable bison/yacc found. ***"
	echo "    Please install the 'bison' package."
	exit 1
fi
AC_CHECK_PROGS(XLEX,$LEX flex lex,none)
if test "$XLEX" = "none" ; then
	echo "*** Error: No suitable flex found. ***"
	echo "    Please install the 'flex' package."
	exit 1
fi

##############################################################################################################################
##						Checks for libraries.
##############################################################################################################################
PKG_CHECK_MODULES(PLATFORM, libplatform >= 0.8.0)
AC_SUBST(PLATFORM_CFLAGS)
AC_SUBST(PLATFORM_LIBS)

PKG_CHECK_MODULES(XANLIB, libxan >= 0.8.0)
AC_SUBST(XANLIB_CFLAGS)
AC_SUBST(XANLIB_LIBS)

PKG_CHECK_MODULES(COMPILERMEMORYMANAGER, libcompilermemorymanager >= 0.8.0)
AC_SUBST(COMPILERMEMORYMANAGER_CFLAGS)
AC_SUBST(COMPILERMEMORYMANAGER_LIBS)

PKG_CHECK_MODULES(LIBILJITU, libiljitu >= 0.8.0)
AC_SUBST(LIBILJITU_CFLAGS)
AC_SUBST(LIBILJITU_LIBS)

PKG_CHECK_MODULES(LIBILJITIR, libiljitir >= 0.8.0)
AC_SUBST(LIBILJITIR_CFLAGS)
AC_SUBST(LIBILJITIR_LIBS)

PKG_CHECK_MODULES(ILJITIROPTIMIZER, libiljitiroptimizer >= 0.8.0)
AC_SUBST(ILJITIROPTIMIZER_CFLAGS)
AC_SUBST(ILJITIROPTIMIZER_LIBS)

PKG_CHECK_MODULES(ILJITIGC, libiljitiroptimizer >= 0.8.0)
AC_SUBST(ILJITGC_CFLAGS)
AC_SUBST(ILJITGC_LIBS)

PKG_CHECK_MODULES(IRVIRTUALMACHINE, libirvirtualmachine >= 0.8.0)
AC_SUBST(IRVIRTUALMACHINE_CFLAGS)
AC_SUBST(IRVIRTUALMACHINE_LIBS)


##############################################################################################################################
#						Run other configure
##############################################################################################################################
AC_CONFIG_SUBDIRS([src/libjit])
AC_SUBST(JIT_LIBS)
JIT_LIBS='-L$(top_srcdir)/src/libjit/jit/.libs -ljit'
AC_SUBST(JIT_CFLAGS)
JIT_CFLAGS='-I$(top_srcdir)/src/libjit/include ${CFLAGS} ${CXXFLAGS}'


##############################################################################################################################
##						Checks for headers.
##############################################################################################################################
AC_HEADER_STDC
AC_CHECK_HEADERS(unistd.h)

AC_OUTPUT(
	Makefile 
	src/Makefile
	doc/Makefile
	pkgconfig/libirvirtualmachine_libjit.pc
	pkgconfig/Makefile
)
