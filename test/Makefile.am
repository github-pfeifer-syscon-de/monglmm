
TESTS = funky_test

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)."\" \
	-DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\" \
	-I../glmLib

AM_CFLAGS= \
	-Wall

AM_CXXFLAGS = \
	-Wall

check_PROGRAMS = funky_test

funky_test_SOURCES = \
    main.cpp

funky_test_LDADD = \
    ../glmLib/libglm.a
