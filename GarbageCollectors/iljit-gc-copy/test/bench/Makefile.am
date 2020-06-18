
# Automake file for benchmark running

# benchmarks is a shell script to run a set of benchmarks
noinst_SCRIPTS = benchmarks

# Benchmarks suite
BENCHMARKS_SUITE = benchmarks

# A list of .NET tests to run
BENCHMARKS = CustomTest.exe GCBench.exe JGFCreateBench.exe
HEAP_SIZES = 4096 51200 4096

# Include benchmarks in distribution
EXTRA_DIST = CustomTest GCBench JGFCreateBench benchmarks.in

# We need to remove version control directory
dist-hook:
	rm -rf `find $(distdir) -name .svn`

# Files to clean
CLEANFILES = $(noinst_SCRIPTS) $(BENCHMARKS)

# The following rules builds test environment
.PHONY: $(BENCHMARKS) build-test-environment

all-local: $(BENCHMARKS_SUITE) $(BENCHMARKS)

$(BENCHMARKS):
	$(CSANT) -b `basename $@ .exe`

# Builds benchmarks script starting by benchmarks.in
do_subst = sed -e 's,[@]ILJIT[@],$(ILJIT),g' \
               -e 's,[@]BENCHMARKS[@],$(BENCHMARKS),g' \
	       -e 's,[@]HEAP_SIZES[@],$(HEAP_SIZES),g'

benchmarks: benchmarks.in
	$(do_subst) < $(srcdir)/$< > $@
	chmod +x $@

# Run benchmarsk after garbage collector install
installcheck-local: $(BENCHMARKS_SUITE)
	@$(SHELL) $(BENCHMARKS_SUITE)
