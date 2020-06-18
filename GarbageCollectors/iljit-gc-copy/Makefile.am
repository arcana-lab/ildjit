
# Main automake file for iljit-gc-copy garbage collector

# Load doxygen targets
include $(top_srcdir)/config/doxygen.am

# Tell automake where aclocal macro are
ACLOCAL_AMFLAGS = -I config

# Search othere automake files here
SUBDIRS = lib test doc

# Include doxygen configuration file
EXTRA_DIST = config/doxygen.conf

# Add doxygen generated files to clean files target
MOSTLYCLEANFILES = $(DX_CLEANFILES) $(DX_DOCDIR)

# Where to put extra documentation
docdir = $(datadir)/doc/$(PACKAGE_NAME)
doc_DATA = AUTHORS COPYING NEWS README

# Add specific rules for install doxygen rules here because doxygen must be
# launched from project root
install-data-local: doxygen-doc
	@$(INSTALL) -d $(docdir)/html; \
	for file in $(DX_DOCDIR)/html/*; do \
	  $(INSTALL_DATA) $$file $(docdir)/html; \
	done;
