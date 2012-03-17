#
# Copyright 2012 by Alex Turbov <i.zaufi@gmail.com>
#

#---------------------------------------------------------
# Section: Init cmake
#---------------------------------------------------------
project(KateCloseExceptPlugin)

set(VERSION_MAJOR 0)
set(VERSION_MINOR 2)
set(VERSION_PATCH 0)
set(VERSION_STRING ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/modules" ${CMAKE_MODULE_PATH})

#---------------------------------------------------------
# Section: Include aux cmake functions
#---------------------------------------------------------
# include(CTest)
# include(AddBoostTests)

#---------------------------------------------------------
# Section: Find used stuff
#---------------------------------------------------------

# Find KDE
find_package(KDE4 REQUIRED)
include(KDE4Defaults)

#---------------------------------------------------------
# Section: Configure building process
#---------------------------------------------------------
# NOTE Order is important!
include_directories(${PROJECT_SOURCE_DIR} ${PROJECT_BINARY_DIR})
include_directories(${KDE4_INCLUDES} ${QT_INCLUDES})

# Add Qt/KDE defines
add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
# Use Kate plugins debug area as default
# add_definitions(-DDEBUG_AREA=13040)

#---------------------------------------------------------
# Section: Define some helpful targets (using included)
#          functions.
# ALERT It should be at the bottom!
#       To make sure all vars r defined!
#---------------------------------------------------------

#include(DefineDoxyDocsTargetIfPossible)
set(NAMING_STYLE Camel)
include(DefineSkeletonGenerationTargetsIfPossible)

#---------------------------------------------------------
# Section: Dive into subdirs after main configuration
#---------------------------------------------------------
add_subdirectory(src)
