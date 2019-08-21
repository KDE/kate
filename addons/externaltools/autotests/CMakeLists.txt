include(ECMMarkAsTest)

find_package(KF5Config QUIET) # KConfig, KConfigGroup
if(NOT KF5Config_FOUND)
  return()
endif()

find_package(KF5CoreAddons QUIET) # KShell
if(NOT KF5CoreAddons_FOUND)
  return()
endif()

# Project Plugin
add_executable(externaltools_test
    externaltooltest.cpp
    ../kateexternaltool.cpp
    ../katetoolrunner.cpp
)
add_test(plugin-externaltools_test externaltools_test)
target_link_libraries(externaltools_test kdeinit_kate Qt5::Test KF5::ConfigCore KF5::CoreAddons)
ecm_mark_as_test(externaltools_test)
