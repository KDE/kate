include(ECMMarkAsTest)

add_executable(tabswitcher_test "")
target_include_directories(tabswitcher_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

find_package(Qt5Test QUIET REQUIRED)
target_link_libraries(
  tabswitcher_test 
  PRIVATE
    kdeinit_kate
    Qt5::Test
)

target_sources(
  tabswitcher_test 
  PRIVATE 
    tabswitchertest.cpp 
    ../tabswitcherfilesmodel.cpp
)

add_test(NAME plugin-tabswitcher_test COMMAND tabswitcher_test)
ecm_mark_as_test(tabswitcher_test)
