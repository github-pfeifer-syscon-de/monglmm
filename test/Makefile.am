
TESTS = funky_test simple_test

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)."\" \
	-DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\" \
	$(GENERICGLM_CFLAGS) \
	$(GENERICIMG_CFLAGS)

AM_CFLAGS= \
	-Wall

AM_CXXFLAGS = \
	-Wall

check_PROGRAMS = funky_test simple_test

simple_test_LDADD = \
	$(GENERICGLM_LIBS) \
	$(GENERICIMG_LIBS)

simple_test_SOURCES = \
    Simple.cpp

funky_test_SOURCES = \
    funky_main.cpp
