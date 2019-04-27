include(ECMMarkAsTest)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

set(Src tabswitchertest.cpp ../tabswitcherfilesmodel.cpp)
add_executable(tabswitcher_test ${Src})
add_test(NAME plugin-tabswitcher_test COMMAND tabswitcher)
target_link_libraries(tabswitcher_test kdeinit_kate Qt5::Test)
ecm_mark_as_test(tabswitcher_test)
