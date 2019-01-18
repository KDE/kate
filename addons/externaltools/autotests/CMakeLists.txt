include(ECMMarkAsTest)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Project Plugin
add_executable(toolrunner_test toolrunnertest.cpp)
add_test(plugin-toolrunner_test toolrunner_test)
target_link_libraries(toolrunner_test kdeinit_kate Qt5::Test)
ecm_mark_as_test(toolrunner_test)
