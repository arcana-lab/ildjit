AC_INIT(libdla, 2.0.0, simo.xan@gmail.com)
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
AC_ARG_ENABLE(morpheus, [  --enable-morpheus    Enable extra code for the Morpheus project])
AM_CONDITIONAL(DEBUG, test "$enable_debug" = "yes")
AM_CONDITIONAL(PRINTDEBUG, test "$enable_printdebug" = "yes")
AM_CONDITIONAL(PROFILE, test "$enable_profile" = "yes")
AM_CONDITIONAL(MORPHEUS, test "$enable_morpheus" = "yes")
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


##############################################################################################################################
##						Checks for headers.
##############################################################################################################################
AC_HEADER_STDC
AC_CHECK_HEADERS(unistd.h sys/param.h sys/time.h time.h sys/mkdev.h sys/sysmacros.h string.h memory.h fcntl.h dirent.h sys/ndir.h ndir.h alloca.h locale.h )

AC_OUTPUT(
	Makefile 
	src/Makefile
	doc/Makefile
	include/Makefile
	pkgconfig/libdla.pc
	pkgconfig/Makefile
)
