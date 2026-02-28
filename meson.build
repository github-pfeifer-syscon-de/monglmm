project('monglmm', 'c', 'cpp'
    , default_options: ['c_std=gnu11'   # for libg15.c is ancient and refuses to usleep
                       , 'cpp_std=c++20'
                       , 'buildtype=debugoptimized'
                       , 'warning_level=3']
    , license : 'GPL-3.0'
    , meson_version: '>=1.3.0'
    , version: '0.1')

fs = import('fs')

pkgdatadir = join_paths(get_option('prefix'), get_option('datadir'), meson.project_name())
pkglocaledir = join_paths(get_option('prefix'), get_option('datadir'), 'locale')
pkgsrcdir = fs.relative_to( meson.global_source_root(), meson.current_build_dir()) + '/src'
add_project_arguments('-DPACKAGE_DATA_DIR="' + pkgdatadir + '"', language: ['cpp','c'])
add_project_arguments('-DPACKAGE="' + meson.project_name() + '"', language: ['cpp','c'])
# this is to recreate the conventions from autotools
add_project_arguments('-DPACKAGE_SRC_DIR="' + pkgsrcdir + '"', language: ['cpp','c'])
#add_project_arguments('-DGETTEXT_PACKAGE="' + meson.project_name() + '"', language: ['c'])
#add_project_arguments('-DPACKAGE_LOCALE_DIR="' + pkglocaledir + '"', language: ['cpp','c'])

disable_flags = [
    '-Wno-unused-parameter' # limit the compiler noisiness
]
cc = meson.get_compiler('c')
foreach cflag : disable_flags
    if cc.has_argument(cflag)
        add_project_arguments(cflag, language: ['c', 'cpp'])
    endif
endforeach

genericglm_deps = dependency('genericglm', version :'>= 0.3.2')
use_gles = cc.has_define('USE_GLES'
                         , dependencies: genericglm_deps
                         , prefix : '#include "genericglm_config.h"')
#                 -> has_define = false
#   #undef UNDEF  -> has_define = false
#   #define DEF   -> has_define = true
#   #define ONE 1 -> has_define = true
#  get_define retives the value, but not found,#undef,#define as empty are indistinguishable

conf = configuration_data()
conf.set_quoted(meson.project_name().to_upper() + '_VERSION', meson.project_version())     # Surround the version in quotes to make it a C string
conf.set       ('LIBGTOP', get_option('glibtop'))
conf.set       ('LIBG15', get_option('libg15'))
conf.set       ('RASPI', get_option('raspi'))
conf.set       ('LMSENSORS', get_option('lmsensors'))
conf_file = configure_file(output : meson.project_name() + '_config.h'
               , configuration : conf)
configuration_inc = include_directories(fs.parent(conf_file))

libusb_deps     = declare_dependency()
if get_option('libg15')
    libusb_deps = dependency('libusb-1.0')
endif
lmsensors_deps  = declare_dependency()
if get_option('lmsensors')
    lmsensors_deps = cc.find_library('sensors')
endif
libgtop_deps    = declare_dependency()
if get_option('glibtop')
   libgtop_deps = dependency('libgtop-2.0')
endif
raspi_deps      = declare_dependency()
if get_option('raspi')
    raspi_deps  = [ # not sure this will work?, but has limited support anyway
                  dependency('/opt/vc/lib/pkgconfig/vcsm.pc')
                , dependency('/opt/vc/lib/pkgconfig/bcm_host.pc')
                ]
endif

thread_deps     = dependency('threads')
genericimg_deps = dependency('genericimg', version :'>= 0.4.0')
deps = [
      genericimg_deps
    , genericglm_deps
    , libusb_deps
    , lmsensors_deps
    , thread_deps
    , raspi_deps
    , libgtop_deps
    ]
subdir('src')
subdir('test')
subdir('res')
sources += app_resources

project_targt = executable(meson.project_name()
    , sources
    , dependencies: deps
    , include_directories : configuration_inc
    , install: true
    , win_subsystem: 'windows'  # disable this to see console output for windows
    )
