# Kate project
project (kate)

# shared libs
set (LIBRARY_TYPE SHARED)

# search packages used by KDE
find_package(KDE4 4.8.0 REQUIRED)
include (KDE4Defaults)
include (MacroLibrary)
include (CheckFunctionExists)

macro_optional_find_package(KActivities 6.1.0)
macro_log_feature(KActivities_FOUND "KActivities" "Activities interface library" "https://projects.kde.org/projects/kde/kdelibs/kactivities" FALSE "6.1.0" "Required for Activities integration.")

# no in source builds
macro_ensure_out_of_source_build("kate requires an out of source build. Please create a separate build directory and run 'cmake path_to_kate [options]' there.")

# config.h
if(NOT APPLE)
  check_function_exists (fdatasync HAVE_FDATASYNC)
  check_function_exists (ctermid HAVE_CTERMID)
endif(NOT APPLE)

configure_file (config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

# enforce build of tests
set (KDE4_BUILD_TESTS "ON" CACHE "BOOL" "Enable building of tests" FORCE)

# add some definitions needed
add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
add_definitions (-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS -DKTEXTEDITOR_NO_DEPRECATED)
include_directories (${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDES} ${KACTIVITIES_INCLUDE_DIRS})

# allow to build own ktexteditor
option(BUILD_KTEXTEDITOR "Build KTextEditor interfaces, instead of reusing the system version" OFF)
if (BUILD_KTEXTEDITOR)
  # use our own copies of some stuff
  set (KDE4_KTEXTEDITOR_LIBS "ktexteditor")

  # add more includes to let the local ktexteditor stuff be found
  include_directories (BEFORE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/includes)

  # ktexteditor interfaces, copy from kdelibs/interfaces/ktexteditor
  macro_optional_add_subdirectory (ktexteditor)
endif (BUILD_KTEXTEDITOR)

# kate part
macro_optional_add_subdirectory (part)

# kate application
macro_optional_add_subdirectory (kate)

# kwrite
macro_optional_add_subdirectory (kwrite)

# plugins
macro_optional_add_subdirectory (plugins)

# docs
macro_optional_add_subdirectory (doc)

# tests
macro_optional_add_subdirectory( tests )
