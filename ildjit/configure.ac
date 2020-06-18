AC_INIT([ildjit],[2.0.0],[simo.xan@gmail.com])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE
AM_PROG_AR
AC_CONFIG_HEADERS([src/config.h])
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
AC_ARG_ENABLE(multiapp, [  --enable-multiapp    Enable support for multi application environment])
AC_ARG_ENABLE(duma, [  --enable-duma    Enable the library to detect buffer overruns and under-runs for debugging purpose])
AC_ARG_ENABLE(windows, [  --enable-windows    Enable the compilation to Windows OS])

AM_CONDITIONAL(DEBUG, test "$enable_debug" = "yes")
AM_CONDITIONAL(PRINTDEBUG, test "$enable_printdebug" = "yes")
AM_CONDITIONAL(PROFILE, test "$enable_profile" = "yes")
AM_CONDITIONAL(MULTIAPP, test "$enable_multiapp" = "yes")
AM_CONDITIONAL(DUMA, test "$enable_duma" = "yes")
AM_CONDITIONAL(WINDOWS, test "$enable_windows" = "yes")

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
#						Checks for programs.
##############################################################################################################################
AC_PROG_LIBTOOL
AC_PROG_CC

##############################################################################################################################
##						Checks for libraries.
##############################################################################################################################
PKG_CHECK_MODULES(PLATFORM, libplatform >= 2.0.0)
AC_SUBST(PLATFORM_CFLAGS)
AC_SUBST(PLATFORM_LIBS)

PKG_CHECK_MODULES(XAN, libxan >= 2.0.0)
AC_SUBST(XAN_CFLAGS)
AC_SUBST(XAN_LIBS)

PKG_CHECK_MODULES(COMPILERMEMORYMANAGER, libcompilermemorymanager >= 2.0.0)
AC_SUBST(COMPILERMEMORYMANAGER_CFLAGS)
AC_SUBST(COMPILERMEMORYMANAGER_LIBS)

PKG_CHECK_MODULES(ILJIT_ILJITU, libiljitu >= 2.0.0)
AC_SUBST(ILJIT_ILJITU_CFLAGS)
AC_SUBST(ILJIT_ILJITU_LIBS)

PKG_CHECK_MODULES(LIBPLUGINMANAGER, libpluginmanager >= 2.0.0)
AC_SUBST(LIBPLUGINMANAGER_CFLAGS)
AC_SUBST(LIBPLUGINMANAGER_LIBS)

PKG_CHECK_MODULES(ILJIT_ILJITSM, libiljitsm >= 2.0.0)
AC_SUBST(ILJIT_ILJITSM_CFLAGS)
AC_SUBST(ILJIT_ILJITSM_LIBS)

PKG_CHECK_MODULES(ILJIT_ILJITMM, libiljitmm >= 2.0.0)
AC_SUBST(ILJIT_ILJITMM_CFLAGS)
AC_SUBST(ILJIT_ILJITMM_LIBS)

PKG_CHECK_MODULES(ILJITIR, libirmanager >= 2.0.0)
AC_SUBST(ILJITIR_CFLAGS)
AC_SUBST(ILJITIR_LIBS)

PKG_CHECK_MODULES(ILBINARY, libilbinary >= 2.0.0)
AC_SUBST(ILBINARY_CFLAGS)
AC_SUBST(ILBINARY_LIBS)

PKG_CHECK_MODULES(ILJITIROPTIMIZER, libiljitiroptimizer >= 2.0.0)
AC_SUBST(ILJITIROPTIMIZER_CFLAGS)
AC_SUBST(ILJITIROPTIMIZER_LIBS)

PKG_CHECK_MODULES(IRVIRTUALMACHINE, libirvirtualmachine >= 2.0.0)
AC_SUBST(IRVIRTUALMACHINE_CFLAGS)
AC_SUBST(IRVIRTUALMACHINE_LIBS)

PKG_CHECK_MODULES(CLIMANAGER, libclimanager >= 2.0.0)
AC_SUBST(CLIMANAGER_CFLAGS)
AC_SUBST(CLIMANAGER_LIBS)

PKG_CHECK_MODULES(DLA, libdla >= 2.0.0)
AC_SUBST(DLA_CFLAGS)
AC_SUBST(DLA_LIBS)

PKG_CHECK_MODULES(MANFRED, libmanfred >= 2.0.0)
AC_SUBST(MANFRED_CFLAGS)
AC_SUBST(MANFRED_LIBS)

PKG_CHECK_MODULES(ILJIT, libcore >= 2.0.0)
AC_SUBST(ILJIT_CFLAGS)
AC_SUBST(ILJIT_LIBS)

#AC_CHECK_LIB(duma, malloc, , echo "ERROR: You have to install the electric-fence library"; exit 1)


##############################################################################################################################
# 						Checks for header files.
##############################################################################################################################
AC_HEADER_STDC
AC_CHECK_HEADERS(
	alloca.h 	\
	assert.h  	\
	dirent.h	\
	errno.h		\
	fcntl.h 	\
	locale.h 	\
	memory.h 	\
	unistd.h 	\
	string.h 	\
	stdio.h		\
	stdlib.h	\
	string.h	\
	sys/param.h 	\
	sys/time.h 	\
	time.h 		\
)
AC_HEADER_MAJOR

##############################################################################################################################
#						Output
##############################################################################################################################
AC_CONFIG_FILES([Makefile 
	src/Makefile
	doc/Makefile
])
AC_OUTPUT
