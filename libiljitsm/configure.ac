AC_INIT(libiljitsm, 2.0.0, simo.xan@gmail.com)
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE
AM_PROG_AR
AC_CONFIG_MACRO_DIR([m4])
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
case $host_os in
  *cygwin*)
    AM_CONDITIONAL(CYGWIN,true)
    ;;
  *solaris*)
    AM_CONDITIONAL(CYGWIN,false)
    ;;  
    *)
    AM_CONDITIONAL(CYGWIN,false)
    ;;
esac
##############################################################################################################################
#						Initialize compiler default options
##############################################################################################################################
AM_INIT_AUTOMAKE(-Wall -Werror)
CFLAGS="$CFLAGS -Wall"
AC_SUBST(CFLAGS)

##############################################################################################################################
# 							Checks for programs.
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
#							Checks for libraries.
##############################################################################################################################
PKG_CHECK_MODULES(PLATFORM, libplatform >= 2.0.0)
AC_SUBST(PLATFORM_CFLAGS)
AC_SUBST(PLATFORM_LIBS)

PKG_CHECK_MODULES(COMPILERMEMORYMANAGER, libcompilermemorymanager >= 2.0.0)
AC_SUBST(COMPILERMEMORYMANAGER_CFLAGS)
AC_SUBST(COMPILERMEMORYMANAGER_LIBS)

PKG_CHECK_MODULES(ILJITU, libiljitu >= 2.0.0)
AC_SUBST(ILJITU_CFLAGS)
AC_SUBST(ILJITU_LIBS)

PKG_CHECK_MODULES(XAN, libxan >= 2.0.0)
AC_SUBST(XAN_CFLAGS)
AC_SUBST(XAN_LIBS)


##############################################################################################################################
# 						Checks for header files.
##############################################################################################################################
AC_HEADER_STDC
AC_CHECK_HEADERS(		\
	sys/stat.h		\
	stdio.h			\
	stdlib.h		\
	assert.h		\
	errno.h			\
	jitsystem.h		\
	iljit-utils.h		\
)

AC_OUTPUT(
	Makefile 
	src/Makefile
	include/Makefile
	pkgconfig/libiljitsm.pc
	pkgconfig/Makefile
)
