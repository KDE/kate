include(ECMMarkAsTest)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Project Plugin
set(ProjectPluginSrc test1.cpp ${CMAKE_CURRENT_SOURCE_DIR}/../fileutil.cpp)
add_executable(projectplugin_test ${ProjectPluginSrc})
add_test(plugin-project_test projectplugin_test)
target_link_libraries(projectplugin_test kdeinit_kate Qt5::Test)
ecm_mark_as_test(projectplugin_test)
