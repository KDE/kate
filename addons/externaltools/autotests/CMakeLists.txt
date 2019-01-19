include(ECMMarkAsTest)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Project Plugin
add_executable(externaltools_test externaltooltest.cpp)
add_test(plugin-externaltools_test externaltools_test)
target_link_libraries(externaltools_test kdeinit_kate Qt5::Test)
ecm_mark_as_test(externaltools_test)
