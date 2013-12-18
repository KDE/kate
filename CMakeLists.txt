# Kate project
project (kate)

set(KATE_VERSION_MAJOR 5)
set(KATE_VERSION_MINOR 0)
set(KATE_VERSION_MICRO 0)
set(KATE_VERSION ${KATE_VERSION_MAJOR}.${KATE_VERSION_MINOR}.${KATE_VERSION_MICRO})

# minimal cmake req
cmake_minimum_required (VERSION 2.8.6 FATAL_ERROR)

# minimal qt req
set (QT_MIN_VERSION "5.2.0")

include (CheckFunctionExists)

# KDE reqs
find_package(ECM 0.0.8 REQUIRED NO_MODULE)

include(CMakePackageConfigHelpers)
include(FeatureSummary)
include(WriteBasicConfigVersionFile)

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})
include(ECMMarkNonGuiExecutable)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)

include(ECMOptionalAddSubdirectory)
include(ECMInstallIcons)
include(ECMSetupVersion)

find_package(Qt5Core REQUIRED NO_MODULE)
find_package(Qt5DBus REQUIRED NO_MODULE)
find_package(Qt5Quick REQUIRED NO_MODULE)
find_package(Qt5Widgets REQUIRED NO_MODULE)
find_package(Qt5Script REQUIRED NO_MODULE)

# Load the frameworks we need
find_package(KF5 CONFIG REQUIRED
    IdleTime ItemModels WidgetsAddons WindowSystem Codecs Archive CoreAddons Solid ThreadWeaver
    Config Auth JS Wallet DBusAddons Sonnet
    I18n GuiAddons Service ConfigWidgets ItemViews Notifications IconThemes Style Completion JobWidgets TextWidgets GlobalAccel XmlGui Crash
    Bookmarks UnitConversion KIO KCMUtils Parts NotifyConfig PrintUtils Su DocTools NewStuff )

find_package(KF5Declarative REQUIRED NO_MODULE)
find_package(KF5KIO REQUIRED NO_MODULE)

find_package(KF5Init)

# needed for set_package_properties macro
find_package(KF5KDE4Support REQUIRED NO_MODULE)
find_package(Qt5Transitional MODULE)

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
add_definitions (-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS -DKTEXTEDITOR_NO_DEPRECATED)

# global include directories
include_directories (${CMAKE_CURRENT_BINARY_DIR})

# use our own copies of some stuff
set (KDE4_KTEXTEDITOR_LIBS "ktexteditor")

# add more includes to let the local ktexteditor stuff be found
include_directories (BEFORE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/includes)

# ktexteditor interfaces, copy from kdelibs/interfaces/ktexteditor
ecm_optional_add_subdirectory (ktexteditor)

# kate part
ecm_optional_add_subdirectory (part)

# kwrite
ecm_optional_add_subdirectory (kwrite)

# kate application
ecm_optional_add_subdirectory (kate)

# addons, e.g. ktexteditor plugins, kate plugins, plasma applets, ...
#macro_optional_add_subdirectory (addons)

# docs
ecm_optional_add_subdirectory (doc)

# tests
#macro_optional_add_subdirectory (tests)
