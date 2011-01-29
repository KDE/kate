# Kate project
project (kate)

# shared libs
set (LIBRARY_TYPE SHARED)

# search packages used by KDE
find_package(KDE4 REQUIRED)
include (KDE4Defaults)
include (MacroLibrary)

# config.h
if(NOT APPLE)
  check_function_exists (fdatasync HAVE_FDATASYNC)
endif(NOT APPLE)

configure_file (config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

# enforce build of tests
set (KDE4_BUILD_TESTS "ON" CACHE "BOOL" "Enable building of tests" FORCE)

# add some definitions needed
add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories (${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDES})

# some stuff to have it compile for KDE 4.4
set (KDE_4_4_LIBS_NEEDED ${KDE4_KUTILS_LIBS})

if (BUILD_KTEXTEDITOR)
  # use our own copies of some stuff
  set (KDE4_KTEXTEDITOR_LIBS "ktexteditor")

  # add more includes to let the local ktexteditor stuff be found
  include_directories (BEFORE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/includes)

  # ktexteditor interfaces, copy from kdelibs/interfaces/ktexteditor
  macro_optional_add_subdirectory (ktexteditor)
endif (BUILD_KTEXTEDITOR)

# kate part, copy from kdelibs/kate
macro_optional_add_subdirectory (part)

# kate, copy from kdesdk/kate
macro_optional_add_subdirectory (kate)

# kwrite, copy from kdebase/apps/kwrite
macro_optional_add_subdirectory (kwrite)

# testing area, just for experimenting with new stuff
macro_optional_add_subdirectory (playground)
