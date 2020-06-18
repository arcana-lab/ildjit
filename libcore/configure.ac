AC_INIT([libcore],[2.0.0],[simo.xan@gmail.com])
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
AC_ARG_ENABLE(libjit, [  --enable-libjit    Enable the Libjit backend])
AC_ARG_ENABLE(windows, [  --enable-windows    Enable the compilation to Windows OS])

AC_ARG_WITH(liblockos, [ --with-liblockos Use lib_lock_os as locking library])
AC_ARG_WITH(liblockthin, [ --with-liblockthin Use lib_lock_thin as locking library])
AM_CONDITIONAL(DEBUG, test "$enable_debug" = "yes")
AM_CONDITIONAL(PRINTDEBUG, test "$enable_printdebug" = "yes")
AM_CONDITIONAL(PROFILE, test "$enable_profile" = "yes")
AM_CONDITIONAL(WINDOWS, test "$enable_windows" = "yes")
AM_CONDITIONAL(USE_LIB_LOCK_OS, test "$with_liblockos" = "yes")
AM_CONDITIONAL(USE_LIB_LOCK_THIN, test "$with_liblockthin" = "yes")

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
AC_PROG_YACC
AC_PROG_LEX
AC_PROG_INSTALL
AC_PROG_CC
AC_PROG_LIBTOOL
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
AC_PATH_PROG(GPERF, gperf)
if test -z "$GPERF" ; then
	echo "*** Error: No suitable gperf found. ***"
	echo "    Please install the 'gperf' package."
	exit 1
fi
AC_CHECK_TOOL(PKGVAR, pkg-config)
if test -z "$PKGVAR" ; then
	echo "*** Error: No suitable pkg-config program found. ***"
	echo "    Please install the 'pkg-config' package."
	exit 1 ;
fi
AC_CHECK_TOOL(MAKEINFO, makeinfo)
if test -z "$MAKEINFO" ; then
	echo "*** Error: No suitable makeinfo program found. ***"
	echo "    Please install the 'texinfo' package."
	exit 1 ;
fi

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

PKG_CHECK_MODULES(ILJIT_ILBINARY, libilbinary >= 2.0.0)
AC_SUBST(ILJIT_ILBINARY_CFLAGS)
AC_SUBST(ILJIT_ILBINARY_LIBS)

PKG_CHECK_MODULES(ILJITIR, libirmanager >= 2.0.0)
AC_SUBST(ILJITIR_CFLAGS)
AC_SUBST(ILJITIR_LIBS)

PKG_CHECK_MODULES(ILJITIROPTIMIZER, libiljitiroptimizer >= 2.0.0)
AC_SUBST(ILJITIROPTIMIZER_CFLAGS)
AC_SUBST(ILJITIROPTIMIZER_LIBS)

PKG_CHECK_MODULES(IRVIRTUALMACHINE, libirvirtualmachine >= 2.0.0)
AC_SUBST(IRVIRTUALMACHINE_CFLAGS)
AC_SUBST(IRVIRTUALMACHINE_LIBS)

if test "$enable_libjit" = "yes" ; then
	PKG_CHECK_MODULES(IRVIRTUALMACHINE_BACKEND, libirvirtualmachine_libjit >= 0.8.0)
else
	PKG_CHECK_MODULES(IRVIRTUALMACHINE_BACKEND, libirvirtualmachine_llvm >= 2.0.0)
fi
AC_SUBST(IRVIRTUALMACHINE_BACKEND_CFLAGS)
AC_SUBST(IRVIRTUALMACHINE_BACKEND_LIBS)

PKG_CHECK_MODULES(CLIMANAGER, libclimanager >= 2.0.0)
AC_SUBST(CLIMANAGER_CFLAGS)
AC_SUBST(CLIMANAGER_LIBS)

PKG_CHECK_MODULES(DLA, libdla >= 2.0.0)
AC_SUBST(DLA_CFLAGS)
AC_SUBST(DLA_LIBS)

PKG_CHECK_MODULES(MANFRED, libmanfred >= 2.0.0)
AC_SUBST(MANFRED_CFLAGS)
AC_SUBST(MANFRED_LIBS)

AC_CHECK_LIB(rt, clock_gettime, rt_libs="yes", rt_libs="no")
#AC_CHECK_LIB(duma, malloc, , echo "ERROR: You have to install the electric-fence library"; exit 1)

if test "$rt_libs" == "no" ; then
	AC_MSG_RESULT([Building the profile with the gettimeofday function])
	AC_DEFINE(PROFILE_GETTIMEOFDAY, 1, [Define to use the gettimeofday function to make the profile])
else 
	AC_MSG_RESULT([Building the profile with the clock_gettime function])
	AC_DEFINE(PROFILE_CLOCKGETTIME, 1, [Define to use the gettimeofday function to make the profile])
fi
AM_CONDITIONAL(CLOCKGETTIME, test "$rt_libs" = "yes")


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

##############################################################################################################################
#						Output
##############################################################################################################################
AC_CONFIG_FILES([Makefile 
	src/Makefile
	include/Makefile
	pkgconfig/Makefile
	pkgconfig/libcore.pc
])
AC_OUTPUT
