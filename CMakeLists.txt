# 3.16 is required for the ECM version we require
cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

# KDE Applications version, managed by release script.
set(RELEASE_SERVICE_VERSION_MAJOR "22")
set(RELEASE_SERVICE_VERSION_MINOR "08")
set(RELEASE_SERVICE_VERSION_MICRO "2")
set(RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")

project(kate VERSION ${RELEASE_SERVICE_VERSION})

set(QT_MIN_VERSION "5.15.2")
set(KF5_DEP_VERSION "5.90.0")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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


ecm_set_disabled_deprecation_versions(
    QT 5.15
    KF 5.90
)

# forbid some old things
add_definitions(-DQT_NO_FOREACH)

# Required here so that the version comparison below works
find_package(Qt${QT_MAJOR_VERSION}Widgets ${QT_MIN_VERSION} CONFIG REQUIRED)

# Required here so that the ki18n_install/kdoctools_install calls work.
find_package(KF5 ${KF5_DEP_VERSION}
    REQUIRED COMPONENTS
        I18n
    OPTIONAL_COMPONENTS
        DocTools
)

# addons for Kate
ecm_optional_add_subdirectory(addons)

# the Kate & KWrite applications
ecm_optional_add_subdirectory(apps)

# our documentation
ecm_optional_add_subdirectory(doc)

ki18n_install(po)
if(KF5DocTools_FOUND)
    kdoctools_install(po)
endif()

feature_summary(INCLUDE_QUIET_PACKAGES WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

# add clang-format target for all our real source files
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
