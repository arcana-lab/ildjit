
MAINTAINERCLEANFILES = Makefile.in

man_MANS=iljit.1 ildjit.1
info_TEXINFOS = ildjit.texi
iljit_TEXINFOS = gpl.texinfo

EXTRA_DIST=		\
	$(man_MANS)	\
	ildjit.texi	\
	texinfo.tex	\
	gpl.texinfo	\
	doxy.config

if API_DOC
install-data-local: install-html-local

install-html-local: doxygen-docs
	$(INSTALL) -d $(docdir)
	cp -r html $(docdir)

doxygen-docs: doxy.config
	doxygen $<
endif

clean-local:
	rm -rf html
