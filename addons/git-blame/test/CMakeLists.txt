include(ECMMarkAsTest)

add_executable(test_gitblameparsing "")
target_include_directories(test_gitblameparsing PRIVATE ${CMAKE_CURRENT_LIST_DIR}/..)

find_package(Qt6Test ${QT_MIN_VERSION} QUIET REQUIRED)
target_link_libraries(
  test_gitblameparsing
  PRIVATE
    kateprivate
    KF6::TextEditor
    KF6::I18n
    Qt::Test
    $<TARGET_OBJECTS:kategitblameplugin>
)

target_sources(
  test_gitblameparsing
  PRIVATE
    gitblametest.cpp
    testfiles.qrc
)

if(ENABLE_PCH)
    # plugin objects were compiled with katepch, MSVC needs its PCH object
    target_link_libraries(test_gitblameparsing PRIVATE katepch)
    target_precompile_headers(test_gitblameparsing REUSE_FROM katepch_tests)
endif()

add_test(NAME kateapp-test_gitblameparsing COMMAND test_gitblameparsing ${OFFSCREEN_QPA})
ecm_mark_as_test(test_gitblameparsing)
