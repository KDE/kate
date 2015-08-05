# Kate project
project (kate)

# minimal requirements
cmake_minimum_required (VERSION 2.8.12 FATAL_ERROR)
set (QT_MIN_VERSION "5.2.0")
set(KF5_DEP_VERSION "5.6.0")

# KDE Application Version, managed by release script
set (KDE_APPLICATIONS_VERSION_MAJOR "15")
set (KDE_APPLICATIONS_VERSION_MINOR "07")
set (KDE_APPLICATIONS_VERSION_MICRO "90")
set (KDE_APPLICATIONS_VERSION "${KDE_APPLICATIONS_VERSION_MAJOR}.${KDE_APPLICATIONS_VERSION_MINOR}.${KDE_APPLICATIONS_VERSION_MICRO}")

# we need some parts of the ECM CMake helpers
find_package (ECM 1.1.0 REQUIRED NO_MODULE)
set (CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
include(ECMOptionalAddSubdirectory)
include(ECMInstallIcons)
include(ECMSetupVersion)
include(ECMMarkNonGuiExecutable)
include(ECMGenerateHeaders)
include(GenerateExportHeader)

include(CMakePackageConfigHelpers)
include(FeatureSummary)
include(WriteBasicConfigVersionFile)
include (CheckFunctionExists)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)

find_package(Qt5 ${QT_MIN_VERSION} CONFIG REQUIRED Core DBus Widgets Script Sql)

if(BUILD_TESTING)
  find_package(Qt5Test ${QT_MIN_VERSION} CONFIG REQUIRED)
endif()

# Load the frameworks we need
find_package(KF5 "${KF5_DEP_VERSION}" REQUIRED COMPONENTS
  Activities
  Config
  Crash
  DocTools
  GuiAddons
  IconThemes
  I18n
  Init
  JobWidgets
  KIO
  Parts
  TextEditor
  WindowSystem
  XmlGui
  IconThemes
)

# use sane compile flags
add_definitions(
  -DQT_USE_QSTRINGBUILDER
  -DQT_NO_CAST_TO_ASCII
  -DQT_NO_CAST_FROM_ASCII
  -DQT_STRICT_ITERATORS
  -DQT_NO_URL_CAST_FROM_STRING
  -DQT_NO_CAST_FROM_BYTEARRAY
  -DQT_NO_SIGNALS_SLOTS_KEYWORDS
  -DQT_USE_FAST_OPERATOR_PLUS
)

# config.h
check_function_exists (ctermid HAVE_CTERMID)
configure_file (config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

# let our config.h be found first in any case
include_directories (BEFORE ${CMAKE_CURRENT_BINARY_DIR})

# kwrite
ecm_optional_add_subdirectory (kwrite)

# kate application
ecm_optional_add_subdirectory (kate)

# addons, e.g. kate plugins, plasma applets, ...
ecm_optional_add_subdirectory (addons)

# docs
ecm_optional_add_subdirectory (doc)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
