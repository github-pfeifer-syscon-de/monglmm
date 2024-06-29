dnl Process this file with autoconf to produce a configure script.

AC_CONFIG_MACRO_DIR([m4])

AC_INIT([monglmm],[0.1],[monglmm@pfeifer-syscon.de])

AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.11])

AM_SILENT_RULES([yes])

AX_CHECK_ENABLE_DEBUG([info], [DEBUG], [RELEASE], [IS-RELEASE])
AC_PROG_CXX
AC_PROG_CC
dnl the requirements on c++20 are growing over time
dnl a older compiler, c++17 might work as well with some adaptions.
AX_CXX_COMPILE_STDCXX([20], [noext], [optional])

LT_INIT

PKG_CHECK_MODULES([GTHREAD], [gthread-2.0])

PKG_CHECK_MODULES([GENERICGLM], [genericglm >= 0.3.2])
PKG_CHECK_MODULES([GENERICIMG], [genericimg >= 0.3.1])

dnl don't repeat previous dependencies
dnl PKG_CHECK_MODULES([GLIBMM], [glibmm-2.4])
dnl PKG_CHECK_MODULES([GTKMM], [gtkmm-3.0])
dnl PKG_CHECK_MODULES(LIBFONTCONFIG, [fontconfig])
dnl PKG_CHECK_MODULES(GLU, [glu])

AC_ARG_WITH([glibtop], AS_HELP_STRING([--with-glibtop], [Build for glibtop (limited suport) otherwise proc/sys will be used, linux only]))

AS_IF([test "x$with_glibtop" = "xyes"], [
    PKG_CHECK_MODULES(LIBGTOP, [libgtop-2.0], [AC_DEFINE([LIBGTOP], [1], [Using libgtop])])
])

AC_ARG_WITH([libg15], AS_HELP_STRING([--with-libg15], [Build with libg15 support]))

AS_IF([test "x$with_libg15" = "xyes"], [
   PKG_CHECK_MODULES(LIBUSB, [libusb-1.0], [AC_DEFINE([LIBG15], [1], [Using libg15])])
])

AM_CONDITIONAL([LIBG15], [test "x$with_libg15" = "xyes"])

AC_ARG_WITH([raspi], AS_HELP_STRING([--with-raspi], [Build with raspi core voltage info (function is (c) broadcom) beware: breaks --with-gles]))

AM_CONDITIONAL([RASPI], [test "x$with_raspi" = "xyes"])

AS_IF([test "x$with_raspi" = "xyes"],
   [
    AC_DEFINE([RASPI], [1], [Using raspi])
    PKG_CHECK_MODULES(VCSM, [/opt/vc/lib/pkgconfig/vcsm.pc])
    PKG_CHECK_MODULES(BCM_HOST, [/opt/vc/lib/pkgconfig/bcm_host.pc])
   ]
)

AC_ARG_WITH([lmsensors], AS_HELP_STRING([--with-lmsensors], [Build with lmsensors support]))

AM_CONDITIONAL([LMSENSORS], [test "x$with_lmsensors" = "xyes"])

AS_IF([test "x$with_lmsensors" = "xyes"],
   [
    AC_DEFINE([LMSENSORS], [1], [Using lmsensor ])
    AC_SEARCH_LIBS([sensors_init], [sensors])
   ]
)

AC_ARG_WITH([gles], AS_HELP_STRING([--with-gles], [use glesv3 (e.g. Raspi) otherweise fullgl, requires same use as with GenericGlm]))

AM_CONDITIONAL([OPENGLES], [test "x$with_gles" = "xyes"])

if test "x$with_gles" = "xyes" ; then
dnl    if  "x$with_raspi" = "xyes" ; then
dnl       #prefere raspi gles? working but no benefit as the AMD perfmon stuff stays locked out
dnl       git this a try ? https://forums.raspberrypi.com/viewtopic.php?t=305952
dnl       PKG_CHECK_MODULES(EPOXY,[/opt/vc/lib/pkgconfig/brcmglesv2.pc], [AC_DEFINE([USE_GLES], [1], [Using glesv3 (e.g. Raspi)])])
dnl    else
       PKG_CHECK_MODULES(EPOXY,[glesv2], [AC_DEFINE([USE_GLES], [1], [Using glesv3 (e.g. Raspi)])])
dnl    fi
dnl GL is added only for the AMD perf monitor functions, if you are looking for a leaner install disable it in GpuMonitor
        AC_SEARCH_LIBS([glBeginPerfMonitorAMD], [GL])
else
    PKG_CHECK_MODULES(EPOXY,[epoxy])
fi

AX_CHECK_SED()

AC_CONFIG_FILES([
Makefile
test/Makefile
src/Makefile
res/Makefile
])
AC_OUTPUT
