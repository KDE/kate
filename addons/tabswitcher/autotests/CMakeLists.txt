include(ECMMarkAsTest)

add_executable(tabswitcher_test "")
target_include_directories(tabswitcher_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

find_package(Qt6Test ${QT_MIN_VERSION} QUIET REQUIRED)
target_link_libraries(
  tabswitcher_test 
  PRIVATE
    KF6::TextEditor
    kateprivate
    Qt::Test
)

target_sources(
  tabswitcher_test 
  PRIVATE 
    tabswitchertest.cpp 
    ../tabswitcherfilesmodel.cpp
)

if(BUILD_PCH)
    target_precompile_headers(tabswitcher_test REUSE_FROM katepch_tests)
endif()

add_test(NAME plugin-tabswitcher_test COMMAND tabswitcher_test ${OFFSCREEN_QPA})
ecm_mark_as_test(tabswitcher_test)
