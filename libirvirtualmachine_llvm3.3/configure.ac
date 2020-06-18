AC_INIT(libirvirtualmachine_llvm, 1.0.2, simo.xan@gmail.com)
AC_LANG([C++])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE
AM_PROG_AR
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
CXXFLAGS="$CXXFLAGS -Wall"
AC_SUBST(CFLAGS)
AC_SUBST(CXXFLAGS)

##############################################################################################################################
##						Checks for programs.
##############################################################################################################################
AC_PROG_INSTALL
AC_PROG_CXXCPP
AC_PROG_CXX
AC_PROG_RANLIB
AC_PROG_LIBTOOL
AC_CHECK_PROGS(XLLVM, llvm-config-3.3,none)
if test "$XLLVM" = "none" ; then
	echo "*** Error: No suitable llvm-config program found. ***"
	echo "    Please install the 'llvm 3.0 32 bits' package."
	exit 1
fi


##############################################################################################################################
##						Checks for libraries.
##############################################################################################################################
PKG_CHECK_MODULES(PLATFORM, libplatform >= 0.6.0)
AC_SUBST(PLATFORM_CFLAGS)
AC_SUBST(PLATFORM_LIBS)

PKG_CHECK_MODULES(XANLIB, libxan >= 0.4.0)
AC_SUBST(XANLIB_CFLAGS)
AC_SUBST(XANLIB_LIBS)

PKG_CHECK_MODULES(COMPILERMEMORYMANAGER, libcompilermemorymanager >= 0.6.0)
AC_SUBST(COMPILERMEMORYMANAGER_CFLAGS)
AC_SUBST(COMPILERMEMORYMANAGER_LIBS)

PKG_CHECK_MODULES(LIBILJITU, libiljitu >= 0.6.0)
AC_SUBST(LIBILJITU_CFLAGS)
AC_SUBST(LIBILJITU_LIBS)

PKG_CHECK_MODULES(LIBILJITIR, libirmanager >= 0.6.0)
AC_SUBST(LIBILJITIR_CFLAGS)
AC_SUBST(LIBILJITIR_LIBS)

PKG_CHECK_MODULES(ILJITIROPTIMIZER, libiljitiroptimizer >= 0.6.0)
AC_SUBST(ILJITIROPTIMIZER_CFLAGS)
AC_SUBST(ILJITIROPTIMIZER_LIBS)

PKG_CHECK_MODULES(ILJITIGC, libiljitgc >= 0.6.0)
AC_SUBST(ILJITGC_CFLAGS)
AC_SUBST(ILJITGC_LIBS)

PKG_CHECK_MODULES(IRVIRTUALMACHINE, libirvirtualmachine >= 0.6.0)
AC_SUBST(IRVIRTUALMACHINE_CFLAGS)
AC_SUBST(IRVIRTUALMACHINE_LIBS)


##############################################################################################################################
#						Run other configure
##############################################################################################################################
AC_SUBST(JIT_LIBS)
AC_SUBST(JIT_CFLAGS)
AC_SUBST(JIT_LDFLAGS)
JIT_CFLAGS=`$XLLVM --cxxflags | sed 's/ -O2//g' | sed 's/ -O3//g' `
JIT_LIBS=`$XLLVM --libs`
JIT_LDFLAGS=`$XLLVM --ldflags`


##############################################################################################################################
##						Checks for headers.
##############################################################################################################################
AC_HEADER_STDC
AC_CHECK_HEADERS(unistd.h)

AC_OUTPUT(
	Makefile 
	src/Makefile
	doc/Makefile
	pkgconfig/libirvirtualmachine_llvm.pc
	pkgconfig/Makefile
)
