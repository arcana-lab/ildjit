all: Doxyfile
	doxygen Doxyfile > /dev/null
	mkdir ildjit-doc-`cat VERSION`
	cp -r Documentation/html ildjit-doc-`cat VERSION`
	tar cfz ildjit-doc.tar.gz ildjit-doc-`cat VERSION`
	rm -r ildjit-doc-`cat VERSION`

clean:
	rm -rf ildjit-doc*
	rm -rf Documentation/*
	rm -f log
