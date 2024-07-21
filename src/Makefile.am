## Process this file with automake to produce Makefile.in



AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)."\" \
	-DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\" \
	$(GTHREAD_CFLAGS) \
	$(GENERICGLM_CFLAGS) \
	$(GENERICIMG_CFLAGS) \
	$(EPOXY_CFLAGS) \
	$(LIBGTOP_CFLAGS) \
	$(LIBUSB_CFLAGS) \
	$(VCSM_CFLAGS) \
	$(BCM_HOST_CFLAGS) \
	$(LMSENSORS_CFLAGS)

AM_CFLAGS= \
	-Wall

AM_CXXFLAGS = \
	-Wall

bin_PROGRAMS = monglmm

monglmm_SOURCES =  \
	CpuMonitor.hpp \
	CpuMonitor.cpp \
	DiskMonitor.hpp \
	DiskMonitor.cpp \
	Filesyses.hpp \
	Filesyses.cpp \
	GpuMonitor.hpp \
	GpuMonitor.cpp \
	GraphShaderContext.hpp \
	GraphShaderContext.cpp \
	HistMonitor.hpp \
	HistMonitor.cpp \
	MonglApp.hpp \
	MonglApp.cpp \
	MonglAppWindow.hpp \
	MonglAppWindow.cpp \
	MonglView.hpp \
	MonglView.cpp \
	Monitor.hpp \
	Monitor.cpp \
	NetMonitor.hpp \
	NetMonitor.cpp \
	Page.hpp \
	Page.cpp \
	Processes.hpp \
	Processes.cpp \
	ProcessesBase.hpp \
	ProcessesBase.cpp \
	resources.c \
	InfoPage.cpp \
	InfoPage.hpp \
	Process.cpp \
	Process.hpp \
	ClkMonitor.cpp \
	ClkMonitor.hpp \
	Infos.hpp \
	Infos.cpp \
	DiskInfo.cpp \
	DiskInfo.hpp \
	DiskInfos.cpp \
	DiskInfos.hpp \
	MemMonitor.cpp \
	MemMonitor.hpp \
	DiagramMonitor.hpp \
	DiagramMonitor.cpp \
	TempMonitor.cpp \
	TempMonitor.hpp \
	libgtop_helper.h \
	Sensors.cpp \
	Sensors.hpp \
	Sensor.cpp \
	Sensor.hpp \
	NameValue.cpp \
	NameValue.hpp \
	FileByLine.cpp \
	FileByLine.hpp \
	GpuCounter.cpp \
	GpuCounter.hpp \
	GpuExtAmdCounter.cpp \
	GpuExtAmdCounter.hpp \
	NetConnection.cpp \
	NetConnection.hpp \
	NetNode.cpp \
	NetNode.hpp \
	NetworkProperties.cpp \
	NetworkProperties.hpp \
	ProcessProperties.cpp \
	ProcessProperties.hpp \
	BaseNetInfo.cpp \
	BaseNetInfo.hpp \
	NetInfo.cpp \
	NetInfo.hpp

if LIBG15
monglmm_SOURCES += \
	G15Worker.cpp \
	G15Worker.hpp \
	libg15.c \
	libg15.h
endif

if RASPI
monglmm_SOURCES +=  \
	RaspiSensors.cpp \
	RaspiSensors.hpp \
	RaspiSensor.cpp \
	RaspiSensor.hpp
endif

if LMSENSORS
monglmm_SOURCES +=  \
	LmSensors.cpp \
	LmSensors.hpp \
	LmSensor.cpp \
	LmSensor.hpp
endif

if OPENGLES
##
else
monglmm_SOURCES +=  \
	GpuGLquery.cpp \
	GpuGLquery.hpp
endif

monglmm_LDFLAGS =

monglmm_LDADD = \
	$(MONGLMM_LIBS) \
	$(GENERICGLM_LIBS) \
	$(GENERICIMG_LIBS) \
	$(GTHREAD_LIBS) \
	$(EPOXY_LIBS) \
	$(LIBGTOP_LIBS) \
	$(LIBUSB_LIBS) \
	$(VCSM_LIBS) \
	$(BCM_HOST_LIBS) \
	$(LMSENSORS_LIBS)


# Remove ui directory on uninstall
uninstall-local:
	-rm -r $(pkgdatadir)

