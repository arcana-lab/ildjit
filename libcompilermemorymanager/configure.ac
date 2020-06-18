AC_INIT(libcompilermemorymanager, 2.0.0, simo.xan@gmail.com)
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
AC_ARG_ENABLE(debug, [  --enable-debug          Enable debug compilation])
AC_ARG_ENABLE(printdebug, [  --enable-printdebug    Enable the print debug and the debug compilation])
AC_ARG_ENABLE(profile, [  --enable-profile    Enable the compilation for the automatically profiler tools])
AC_ARG_ENABLE(internalgc, [  --enable-internalgc    Enable the Bohem garbage collector for the internal modules of ILDJIT compiler])
AC_ARG_ENABLE(multiapp, [  --enable-multiapp    Enable support for multiple application environment])
AM_CONDITIONAL(DEBUG, test "$enable_debug" = "yes")
AM_CONDITIONAL(PRINTDEBUG, test "$enable_printdebug" = "yes")
AM_CONDITIONAL(PROFILE, test "$enable_profile" = "yes")
AM_CONDITIONAL(INTERNAL_GC, test "$enable_internalgc" = "yes")
AM_CONDITIONAL(MULTIAPP, test "$enable_multiapp" = "yes")
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
#						Checks for programs.
##############################################################################################################################
AC_PROG_INSTALL
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_MAKE_SET
if ! test -z "$MAKE" ; then
	AC_ERROR([Error: make program not exist, you have to install it])
fi
AC_CHECK_TOOL(BZIP2, bzip2, no)
if test "$BZIP2" = "no" ; then
	AC_ERROR([Error: you have to install bzip2 program])
fi
AC_CHECK_TOOL(PKGVAR, pkg-config)
if test -z "$PKGVAR" ; then
	echo "ERROR: I can't find the pkg-config program, then you have to install it" ;
	exit 1 ;
fi

##############################################################################################################################
#						Checks for libraries.
##############################################################################################################################
PKG_CHECK_MODULES(PLATFORM, libplatform >= 2.0.0)
AC_SUBST(PLATFORM_CFLAGS)
AC_SUBST(PLATFORM_LIBS)

PKG_CHECK_MODULES(XAN, libxan >= 2.0.0)
AC_SUBST(XAN_CFLAGS)
AC_SUBST(XAN_LIBS)


##############################################################################################################################
##						Checks for headers.
##############################################################################################################################
AC_HEADER_STDC
AC_CHECK_HEADERS(
        stdio.h         \
        stdlib.h        \
        string.h        \
        assert.h        \
)

AC_HEADER_MAJOR
AC_FUNC_ALLOCA
AC_STRUCT_TM
AC_STRUCT_ST_BLOCKS

##############################################################################################################################
#						Run other configure
##############################################################################################################################
AC_CONFIG_SUBDIRS([src/libgc])
if test "$enable_internalgc" == "yes" ; then
	AC_SUBST(GC_LIBS)
	GC_LIBS='libgc/.libs/libgc.a'
	AC_SUBST(GC_CFLAGS)
	GC_CFLAGS='-I./libgc/include'
	AC_DEFINE(GC_LINUX_THREADS, 1, [Define to use libgc linux thread support])
	AC_DEFINE(_REENTRANT, 1, [Define for re-entrant thread support])
	AC_DEFINE(THREAD_LOCAL_ALLOC, 1, [Define for thread local allocation])
fi

AC_OUTPUT(
	Makefile 
	src/Makefile
	doc/Makefile
	include/Makefile
	pkgconfig/libcompilermemorymanager.pc
	pkgconfig/Makefile
)
