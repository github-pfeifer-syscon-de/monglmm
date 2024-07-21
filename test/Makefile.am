
%.cpp:
	ln -sf ../src/$@

%.hpp:
	ln -sf ../src/$@

SOURCE_CPP = Process.cpp NameValue.cpp Monitor.cpp

SOURCE_HPP = Process.hpp NameValue.hpp Monitor.hpp

SOURCE = $(SOURCE_CPP) $(SOURCE_HPP)

include_source: $(SOURCE)

TESTS: include_source

TESTS = simple_test funky_test

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

check_PROGRAMS = funky_test simple_test

simple_test_LDADD = \
	$(GENERICGLM_LIBS) \
	$(GENERICIMG_LIBS)

simple_test_SOURCES = \
    $(SOURCE) \
    Simple.cpp

funky_test_SOURCES = \
    funky_main.cpp
