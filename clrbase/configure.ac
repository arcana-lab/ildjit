AC_INIT(clrbase, 2.0.0, simo.xan@gmail.com)
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
AC_PATH_PROG(GPERF, gperf)
if test -z "$GPERF" ; then
	echo "*** Error: No suitable gperf found. ***"
	echo "    Please install the 'gperf' package."
	exit 1
fi

##############################################################################################################################
##						Checks for libraries.
##############################################################################################################################
PKG_CHECK_MODULES(PLATFORM, libplatform >= 2.0.0)
AC_SUBST(PLATFORM_CFLAGS)
AC_SUBST(PLATFORM_LIBS)

PKG_CHECK_MODULES(XANLIB, libxan >= 2.0.0)
AC_SUBST(XANLIB_CFLAGS)
AC_SUBST(XANLIB_LIBS)

PKG_CHECK_MODULES(LIBILJITU, libiljitu >= 2.0.0)
AC_SUBST(LIBILJITU_CFLAGS)
AC_SUBST(LIBILJITU_LIBS)

PKG_CHECK_MODULES(LIBILJITIR, libirmanager >= 2.0.0)
AC_SUBST(LIBILJITIR_CFLAGS)
AC_SUBST(LIBILJITIR_LIBS)

PKG_CHECK_MODULES(ILJITIROPTIMIZER, libiljitiroptimizer >= 2.0.0)
AC_SUBST(ILJITIROPTIMIZER_CFLAGS)
AC_SUBST(ILJITIROPTIMIZER_LIBS)

PKG_CHECK_MODULES(IRVIRTUALMACHINE, libirvirtualmachine >= 2.0.0)
AC_SUBST(IRVIRTUALMACHINE_CFLAGS)
AC_SUBST(IRVIRTUALMACHINE_LIBS)

PKG_CHECK_MODULES(CLIMANAGER, libclimanager >= 2.0.0)
AC_SUBST(CLIMANAGER_CFLAGS)
AC_SUBST(CLIMANAGER_LIBS)

PKG_CHECK_MODULES(ILJIT, libcore >= 2.0.0)
AC_SUBST(ILJIT_CFLAGS)
AC_SUBST(ILJIT_LIBS)


##############################################################################################################################
# 						Checks for functions.
##############################################################################################################################
AC_CHECK_FUNCS(ioctl getsockopt setsockopt access)


##############################################################################################################################
##						Checks for headers.
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
	pthread.h	\
	stdlib.h	\
	string.h	\
	sys/mkdev.h	\
	sys/ndir.h 	\
	sys/param.h 	\
	sys/sysmacros.h \
	sys/time.h 	\
	time.h 		\
	sys/socket.h	\
	linux/irda.h	\
,[],[],
[[#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h> 
#endif
]]
)
AC_CHECK_HEADER(pthread.h, , echo "ERROR: The pthread.h header is not present. You have to install POSIX thread support in your system"; exit 1)

AC_HEADER_MAJOR
AC_STRUCT_TM
AC_STRUCT_ST_BLOCKS
AC_FUNC_CLOSEDIR_VOID

AC_OUTPUT(
	Makefile 
	src/Makefile
)
