# 3.16 is required for the ECM version we require
cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

# KDE Applications version, managed by release script.
set(RELEASE_SERVICE_VERSION_MAJOR "24")
set(RELEASE_SERVICE_VERSION_MINOR "07")
set(RELEASE_SERVICE_VERSION_MICRO "70")
set(RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")

project(kate VERSION ${RELEASE_SERVICE_VERSION})

# KF6 only
set(QT_MIN_VERSION "6.5.0")
set(KF5_DEP_VERSION "6.0.0")
set(QT_MAJOR_VERSION "6")

# use C++20 like KF6 itself
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# We need some parts of the ECM CMake helpers.
find_package(ECM ${KF5_DEP_VERSION} QUIET REQUIRED NO_MODULE)

# We append to the module path so modules can be overridden from the command line.
list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEClangFormat)
include(KDEGitCommitHooks)

include(ECMOptionalAddSubdirectory)
include(ECMAddAppIcon)
include(ECMInstallIcons)
include(ECMDeprecationSettings)

include(FeatureSummary)
include(FindIntl)

# Required here so that the version comparison below works
find_package(Qt6Widgets ${QT_MIN_VERSION} CONFIG REQUIRED)
find_package(Qt6Concurrent ${QT_MIN_VERSION} CONFIG REQUIRED)

# Required here so that the ki18n_install/kdoctools_install calls work.
find_package(KF6 ${KF5_DEP_VERSION}
    REQUIRED COMPONENTS
        I18n
        CoreAddons
        GuiAddons
        Crash
        IconThemes
        TextEditor
    OPTIONAL_COMPONENTS
        DocTools
)

if (NOT WIN32 AND NOT HAIKU)
    find_package(Intl)
    set_package_properties(Intl PROPERTIES TYPE REQUIRED
        URL "http://gnuwin32.sourceforge.net/packages/libintl.htm"
        PURPOSE "Needed for building kate unless glibc is the system libc implementation"
    )
endif()

# define which offscreen platform plugin to use for tests
# windows has issues with offscreen tests in the CI
set(OFFSCREEN_QPA)
if(NOT WIN32)
    set(OFFSCREEN_QPA "-platform" "offscreen")
endif()

# deprecation options
ecm_set_disabled_deprecation_versions(
    QT 6.5
    KF 5.90
)

option(BUILD_PCH "Enable PCH support" ON)
if (BUILD_PCH)
    add_subdirectory(pch)
endif()

# addons for Kate
ecm_optional_add_subdirectory(addons)

# the Kate & KWrite applications
ecm_optional_add_subdirectory(apps)

# our documentation
ecm_optional_add_subdirectory(doc)

ki18n_install(po)
if(KF6DocTools_FOUND)
    kdoctools_install(po)
endif()

feature_summary(INCLUDE_QUIET_PACKAGES WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

# add clang-format target for all our real source files
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
