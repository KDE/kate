include(ECMMarkAsTest)

find_package(KF${KF_MAJOR_VERSION}Config ${KF5_DEP_VERSION} QUIET) # KConfig, KConfigGroup
if(NOT KF5Config_FOUND)
  return()
endif()

find_package(KF${KF_MAJOR_VERSION}CoreAddons ${KF5_DEP_VERSION} QUIET) # KShell
if(NOT KF5CoreAddons_FOUND)
  return()
endif()

# Project Plugin
add_executable(externaltools_test
    externaltooltest.cpp
    ../kateexternaltool.cpp
    ../katetoolrunner.cpp
)
add_test(NAME plugin-externaltools_test COMMAND externaltools_test)
target_link_libraries(externaltools_test PRIVATE kateprivate Qt::Test KF${KF_MAJOR_VERSION}::I18n KF${KF_MAJOR_VERSION}::ConfigCore KF${KF_MAJOR_VERSION}::CoreAddons KF${KF_MAJOR_VERSION}::TextEditor)
ecm_mark_as_test(externaltools_test)
