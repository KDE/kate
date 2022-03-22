include(ECMMarkAsTest)

add_executable(tabswitcher_test "")
target_include_directories(tabswitcher_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

find_package(Qt${QT_MAJOR_VERSION}Test ${QT_MIN_VERSION} QUIET REQUIRED)
target_link_libraries(
  tabswitcher_test 
  PRIVATE
    KF5::TextEditor
    Qt::Test
)

target_sources(
  tabswitcher_test 
  PRIVATE 
    tabswitchertest.cpp 
    ../tabswitcherfilesmodel.cpp
)

add_test(NAME plugin-tabswitcher_test COMMAND tabswitcher_test)
ecm_mark_as_test(tabswitcher_test)
