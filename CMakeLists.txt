# Kate project
project (kate)

# minimal requirements
cmake_minimum_required (VERSION 2.8.12 FATAL_ERROR)
set (QT_MIN_VERSION "5.2.0")

# set the kate version, used for part / kate / kwrite
set (KATE_VERSION_MAJOR 5)
set (KATE_VERSION_MINOR 0)
set (KATE_VERSION_MICRO 0)
set (KATE_VERSION ${KATE_VERSION_MAJOR}.${KATE_VERSION_MINOR}.${KATE_VERSION_MICRO})

# setup library versions corresponding
set (GENERIC_LIB_SOVERSION ${KATE_VERSION_MAJOR})
set (GENERIC_LIB_VERSION ${KATE_VERSION})

# we need some parts of the ECM CMake helpers
find_package (ECM 0.0.9 REQUIRED NO_MODULE)
set (CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
include(ECMOptionalAddSubdirectory)
include(ECMInstallIcons)
include(ECMSetupVersion)
include(ECMMarkNonGuiExecutable)
include(ECMGenerateHeaders)

include(CMakePackageConfigHelpers)
include(FeatureSummary)
include(WriteBasicConfigVersionFile)
include (CheckFunctionExists)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED Core DBus Widgets Script Test)

# Load the frameworks we need
find_package(KF5 CONFIG REQUIRED COMPONENTS
  Archive
  Config
  DocTools
  GuiAddons
  I18n
  Init
  JobWidgets
  KCMUtils
  KIO
  NewStuff
  Parts
  PrintUtils
  Sonnet
  WindowSystem
  XmlGui
  KDE4Support # TODO: waiting for Title case include files
)

# config.h
if(NOT APPLE)
  check_function_exists (fdatasync HAVE_FDATASYNC)
  check_function_exists (ctermid HAVE_CTERMID)
endif(NOT APPLE)

configure_file (config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

# enforce build of tests
set (KDE4_BUILD_TESTS "ON" CACHE "BOOL" "Enable building of tests" FORCE)

# KDE/QT definitions we want
add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
add_definitions (-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)

# Temporary solution until the split frameworks provide forward headers
get_target_property (KDE4Support_INCLUDES KF5::KDE4Support INTERFACE_INCLUDE_DIRECTORIES)
include_directories(${KDE4Support_INCLUDES})

# add more includes to let the local ktexteditor stuff be found first in any case
include_directories (BEFORE ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/ktexteditor)

# ktexteditor interfaces, copy from kdelibs/interfaces/ktexteditor
ecm_optional_add_subdirectory (ktexteditor)

# kate part
ecm_optional_add_subdirectory (part)

# kwrite
ecm_optional_add_subdirectory (kwrite)

# kate application
ecm_optional_add_subdirectory (kate)

# addons, e.g. ktexteditor plugins, kate plugins, plasma applets, ...
ecm_optional_add_subdirectory (addons)

# docs
ecm_optional_add_subdirectory (doc)

# tests
ecm_optional_add_subdirectory (tests)
