include(ECMMarkAsTest)

add_executable(btbrowser_test "")
target_include_directories(btbrowser_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

find_package(Qt6Test ${QT_MIN_VERSION} QUIET REQUIRED)
target_link_libraries(
  btbrowser_test 
  PRIVATE 
    Qt::Widgets
    Qt::Test
)

target_sources(btbrowser_test PRIVATE
  btbrowsertest.cpp 
  ${CMAKE_CURRENT_SOURCE_DIR}/../btparser.cpp
)

if(BUILD_PCH)
    target_precompile_headers(btbrowser_test REUSE_FROM katepch_tests)
endif()

add_test(NAME plugin-btbrowser_test COMMAND btbrowser_test ${OFFSCREEN_QPA})
ecm_mark_as_test(btbrowser_test)
