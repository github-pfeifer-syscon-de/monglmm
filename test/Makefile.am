
TESTS = process_test

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)."\" \
	-DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\" \
	-I ../src \
	$(GENERICGLM_CFLAGS) \
	$(GENERICIMG_CFLAGS)

AM_CFLAGS= \
	-Wall -Wpedantic

AM_CXXFLAGS = \
	-Wall -Wpedantic

check_PROGRAMS = process_test

process_test_LDADD = \
	../src/Process.o \
	../src/NameValue.o \
	../src/Monitor.o \
	$(GENERICGLM_LIBS) \
	$(GENERICIMG_LIBS)

process_test_SOURCES = \
    process_test.cpp
