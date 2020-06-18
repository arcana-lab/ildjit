
# Definiton of benchmarks macro
#
# This macro declare --enable-benchmarks and --disable-benchmarks configure
# command line arguments. If --enable-benchmarks is given on command line and
# cscc compiler, csant build tool and iljit compiler are found the automake
# conditional variable BENCHMARK_COND is enabled and CSANT and ILJIT variable
# are filled with the name of the right binary. By default benchmarks are
# disabled. For an example makefile that uses this features see
# ../test/bench/Makefile.am

# CS_BENCHMARKS:
AC_DEFUN([CS_BENCHMARK], [

# We need pnet compiler, csant tool and iljit compiler
AC_CHECK_PROG(have_cscc, cscc, yes)
AC_CHECK_PROG(have_csant, csant, yes)
AC_CHECK_PROG(have_iljit, iljit, yes)

# Check for benchmarks dependencies
if test "$have_cscc" = "yes" -a \
        "$have_csant" = "yes" -a \
	"$have_iljit" = "yes"; then
  AM_CONDITIONAL(BENCHMARK_COND, :)
  CSANT=csant
  ILJIT=iljit
  AC_SUBST(CSANT)
  AC_SUBST(ILJIT)
else
  AM_CONDITIONAL(BENCHMARK_COND, false)
  AC_MSG_ERROR(benchmarks dependencies not found)
fi

])
