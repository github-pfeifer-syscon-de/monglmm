
noinst_LIBRARIES = libglm.a


AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)."\" \
	-DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\" \
	$(GTKMM_CFLAGS) \
	$(GENERICGLM_CFLAGS) \
	$(GENERICIMG_CFLAGS) \
	$(EPOXY_CFLAGS)

libglm_a_CFLAGS= \
	-Wall

libglm_a_CXXFLAGS = \
	-Wall


libglm_a_SOURCES =  \
	active_ptr.cpp \
	active_ptr.hpp \
	Geom2.cpp \
	Geom2.hpp \
	Diagram2.cpp \
	Diagram2.hpp \
	Font2.cpp \
	Font2.hpp \
	Glyph2.cpp \
	Glyph2.hpp \
	Text2.cpp \
	Text2.hpp \
	NamedTreeNode2.cpp \
	NamedTreeNode2.hpp
