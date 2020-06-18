
MAINTAINERCLEANFILES = aclocal.m4 configure Makefile.in

SUBDIRS=src include pkgconfig
EXTRA_DIST=INSTALL

all-local:
	if test ! -e $(prefix)/lib/iljit/optimizers ; then	\
		mkdir $(prefix)/lib/iljit/optimizers ;		\
	fi ;

maintainer-clean-local:
	rm -rf build-aux
