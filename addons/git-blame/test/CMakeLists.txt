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
)

target_sources(
  test_gitblameparsing
  PRIVATE
    ../gitblameparser.cpp
    gitblametest.cpp
    testfiles.qrc
)

if(ENABLE_PCH)
    target_precompile_headers(test_gitblameparsing REUSE_FROM katepch_tests)
endif()

add_test(NAME kateapp-test_gitblameparsing COMMAND test_gitblameparsing ${OFFSCREEN_QPA})
ecm_mark_as_test(test_gitblameparsing)
