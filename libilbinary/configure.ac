AC_INIT([libilbinary],[2.0.0],[simo.xan@gmail.com])
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
AM_CONDITIONAL(DEBUG, test "$enable_debug" = "yes")
AM_CONDITIONAL(PRINTDEBUG, test "$enable_printdebug" = "yes")
AM_CONDITIONAL(PROFILE, test "$enable_profile" = "yes")

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
AC_PROG_INSTALL
AC_PROG_CC
AC_PROG_LIBTOOL
AC_CHECK_TOOL(PKGVAR, pkg-config)
if test -z "$PKGVAR" ; then
	echo "*** Error: No suitable pkg-config program found. ***"
	echo "    Please install the 'pkg-config' package."
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


##############################################################################################################################
# 						Checks for header files.
##############################################################################################################################
AC_HEADER_STDC
AC_CHECK_HEADERS(
	assert.h  	\
	errno.h		\
	unistd.h 	\
	stdio.h		\
	stdlib.h	\
	string.h	\
)

##############################################################################################################################
#						Output
##############################################################################################################################
AC_CONFIG_FILES([Makefile 
	src/Makefile
	include/Makefile
	pkgconfig/Makefile
	pkgconfig/libilbinary.pc
])
AC_OUTPUT
