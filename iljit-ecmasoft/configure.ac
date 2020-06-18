AC_INIT([iljit-ecmasoft-decoder], [2.0.0], [simo.xan@gmail.com])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE
AM_PROG_AR
AM_CONFIG_HEADER([src/config.h])
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
#						Checks for programs.
##############################################################################################################################
AC_PROG_INSTALL
AC_PROG_CC
AC_PROG_LIBTOOL
AC_CHECK_TOOL(PKGVAR, pkg-config)
if test -z "$PKGVAR" ; then
	echo "ERROR: I can't find the pkg-config program, then you have to install it" ;
	exit 1 ;
fi

##############################################################################################################################
##						Checks for libraries.
##############################################################################################################################
PKG_CHECK_MODULES(COMPILERMEMORYMANAGER, libcompilermemorymanager >= 2.0.0)
AC_SUBST(COMPILERMEMORYMANAGER_CFLAGS)
AC_SUBST(COMPILERMEMORYMANAGER_LIBS)

PKG_CHECK_MODULES(XAN, libxan >= 2.0.0)
AC_SUBST(XAN_CFLAGS)
AC_SUBST(XAN_LIBS)

PKG_CHECK_MODULES(ILJITES_ILJITU, libiljitu >= 2.0.0)
AC_SUBST(ILJITES_ILJITU_CFLAGS)
AC_SUBST(ILJITES_ILJITU_LIBS)

PKG_CHECK_MODULES(ILJITES_ILJITSM, libiljitsm >= 2.0.0)
AC_SUBST(ILJITES_ILJITSM_CFLAGS)
AC_SUBST(ILJITES_ILJITSM_LIBS)

PKG_CHECK_MODULES(ILJITES_ILJITMM, libiljitmm >= 2.0.0)
AC_SUBST(ILJITES_ILJITMM_CFLAGS)
AC_SUBST(ILJITES_ILJITMM_LIBS)

##############################################################################################################################
##						Checks for headers.
##############################################################################################################################
AC_HEADER_STDC
AC_CHECK_HEADERS(string.h)

AC_OUTPUT(
	Makefile 
	src/Makefile
)
