PKGCONFIG = pkg-config
GLIB_COMPILE_RESOURCES = $(shell $(PKGCONFIG) --variable=glib_compile_resources gio-2.0)

MONGLMM_RESOURCE_XML=monglapp.gresource.xml
MONGLMM_RESOURCES=$(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=. --generate-dependencies $(MONGLMM_RESOURCE_XML))
resources_c=../src/resources.c

all:$(resources_c)

$(resources_c): $(MONGLMM_RESOURCE_XML) $(MONGLMM_RESOURCES)
	$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(dir $<) --generate-source $<

# The desktop files
desktopdir = $(datadir)/applications
desktop_DATA=monglmm.desktop

# application icon
icon_basename=monglmm
pixmapdir=$(datadir)/icons/hicolor/64x64/apps
pixmap_DATA=$(icon_basename).png
appicondir=$(datadir)/icons/hicolor/scalable/apps
appicon_DATA=$(icon_basename).svg


EXTRA_DIST = $(MONGLMM_RESOURCES) $(MONGLMM_RESOURCE_XML) $(pixmap_DATA) $(appicon_DATA) $(desktop_DATA)

clean-local:
	-rm -rf $(resources_c)
	-rm -rf $(desktop_DATA)
