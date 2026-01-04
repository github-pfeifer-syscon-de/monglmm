
TESTS = process_test

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)."\" \
	-DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\" \
	-I $(top_srcdir)/src \
	$(GENERICGLM_CFLAGS) \
	$(GENERICIMG_CFLAGS) \
	$(EPOXY_CFLAGS)

AM_CFLAGS= \
	-Wall -Wpedantic -Wconversion

AM_CXXFLAGS = \
	-Wall -Wpedantic -Wconversion

check_PROGRAMS = process_test

process_test_LDADD = \
	../src/Process.o \
	../src/NameValue.o \
	$(GENERICGLM_LIBS) \
	$(GENERICIMG_LIBS)

process_test_SOURCES = \
	process_test.cpp

# used to create logo
SvgTest_LDADD = \
	$(GENERICGLM_LIBS) \
	$(GENERICIMG_LIBS) \
	$(EPOXY_LIBS)

SvgTest_SOURCES = \
	SvgTest.cpp
