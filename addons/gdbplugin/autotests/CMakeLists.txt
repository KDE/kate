include(ECMMarkAsTest)

add_executable(test_breakpointview "")
target_include_directories(test_breakpointview PRIVATE ${CMAKE_CURRENT_LIST_DIR}/..)

find_package(Qt6Test ${QT_MIN_VERSION} QUIET REQUIRED)
target_link_libraries(
  test_breakpointview
  PRIVATE
    kateprivate
    KF6::TextEditor
    KF6::I18n
    Qt::Test
     $<TARGET_OBJECTS:kategdbplugin>
)

if(ENABLE_PCH)
  target_link_libraries(test_breakpointview PRIVATE katepch)
  target_precompile_headers(test_breakpointview REUSE_FROM katepch_tests)
endif()

target_sources(
  test_breakpointview
  PRIVATE
    test_breakpointview.cpp
)

add_test(NAME kateapp-test_breakpointview COMMAND test_breakpointview ${OFFSCREEN_QPA})
ecm_mark_as_test(test_breakpointview)

add_executable(test_localsview "")
target_include_directories(test_localsview PRIVATE ${CMAKE_CURRENT_LIST_DIR}/..)
target_link_libraries(
  test_localsview
  PRIVATE
    kateprivate
    KF6::TextEditor
    KF6::I18n
    Qt::Test
     $<TARGET_OBJECTS:kategdbplugin>
)

if(ENABLE_PCH)
  target_link_libraries(test_localsview PRIVATE katepch)
  target_precompile_headers(test_localsview REUSE_FROM katepch_tests)
endif()

target_sources(
  test_localsview
  PRIVATE
    test_localsview.cpp
)

add_test(NAME kateapp-test_localsview COMMAND test_localsview ${OFFSCREEN_QPA})
ecm_mark_as_test(test_localsview)
