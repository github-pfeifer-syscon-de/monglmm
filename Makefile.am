## Process this file with automake to produce Makefile.in

SUBDIRS = res src

dist_doc_DATA = \
	README.md \
	COPYING \
	AUTHORS \
	ChangeLog \
	INSTALL \
	NEWS


# Remove doc directory on uninstall
uninstall-local:
	-rm -r $(docdir)
