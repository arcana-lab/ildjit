
# Autoconf file for iljit-gc-copy project

# Init auto configuration system
AC_INIT(iljit-gc-copy, 0.1, speziale.ettore@gmail.com)

# Define auxiliary scripts location
AC_CONFIG_AUX_DIR(build-aux)

# Enable debug compiling
AC_ARG_ENABLE(debug, AS_HELP_STRING(--enable-debug, enable debug compiling))
AM_CONDITIONAL(DEBUG, test "$enable_debug" = "yes")

# Require c compiler
AC_PROG_CC
# Use libtool to build library
AC_PROG_LIBTOOL

# Where to write autoconf output
AC_CONFIG_FILES([
                  Makefile lib/Makefile
                  test/Makefile test/unit/Makefile
		  test/bench/Makefile
		  doc/Makefile
	        ])

# Check for libc standard library
AC_HEADER_STDC

# We need ildjit compiler and libiljitu
PKG_CHECK_MODULES(ILJIT, iljit)
PKG_CHECK_MODULES(LIBILJITU, libiljitu)
# Generate right include and libs flags
AC_SUBST(ILJIT_CFLAGS)
AC_SUBST(LIBILJITU_CFLAGS)
AC_SUBST(LIBILJITU_LIBS)

# libxan required
PKG_CHECK_MODULES(XAN, libxan)
# Generate right include flags
AC_SUBST(XAN_CFLAGS)
AC_SUBST(XAN_LIBS)

# Require csharp benchmarks
CS_BENCHMARK

# Doxygen config
#DX_HTML_FEATURE(ON)
#DX_PDF_FEATURE(OFF)
#DX_PS_FEATURE(OFF)
#DX_INIT_DOXYGEN(iljit-gc-copy, config/doxygen.conf, doc/doxygen)

# Init automake
AM_INIT_AUTOMAKE(-Wall -Werror)

# Load check macros for testing
PKG_CHECK_MODULES(CHECK, check)
AC_SUBST(CHECK_CFLAGS)
AC_SUBST(CHECK_LIBS)

# Generate output
AC_OUTPUT
