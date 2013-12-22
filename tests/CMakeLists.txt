set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

include_directories(
  ${CMAKE_SOURCE_DIR}/part
  ${CMAKE_SOURCE_DIR}/part/buffer
  ${CMAKE_SOURCE_DIR}/part/completion
  ${CMAKE_SOURCE_DIR}/part/dialogs
  ${CMAKE_SOURCE_DIR}/part/document
  ${CMAKE_SOURCE_DIR}/part/script
  ${CMAKE_SOURCE_DIR}/part/mode
  ${CMAKE_SOURCE_DIR}/part/render
  ${CMAKE_SOURCE_DIR}/part/search
  ${CMAKE_SOURCE_DIR}/part/syntax
  ${CMAKE_SOURCE_DIR}/part/undo
  ${CMAKE_SOURCE_DIR}/part/utils
  ${CMAKE_SOURCE_DIR}/part/vimode
  ${CMAKE_SOURCE_DIR}/part/view
  ${CMAKE_BINARY_DIR}/part
)

add_definitions(-DTEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data/")
add_definitions(-DJS_DATA_DIR="${CMAKE_SOURCE_DIR}/part/script/data/")

set (KATE_TEST_LINK_LIBS ktexteditor katepartinterfaces
  KF5::I18n
  KF5::KIOFileWidgets
  KF5::IconThemes
  KF5::WindowSystem
  KF5::GuiAddons)

include(ECMMarkAsTest)

# encoding tests
add_subdirectory (encoding)

macro(kate_unit_tests)
  foreach(_testname ${ARGN})
    add_executable(${_testname} ${_testname}.cpp)
    add_test(kate-${_testname} ${_testname})
    target_link_libraries(${_testname} ${KATE_TEST_LINK_LIBS} Qt5::Test)
    ecm_mark_as_test(${_testname})
  endforeach()
endmacro()

macro(kate_unit_test testname)
  add_executable(${testname} ${testname}.cpp ${ARGN})
  add_test(kate-${testname} ${testname})
  target_link_libraries(${testname} ${KATE_TEST_LINK_LIBS} Qt5::Test)
  ecm_mark_as_test(${testname})
endmacro()

kate_unit_tests(
  katetextbuffertest
  range_test
  undomanager_test
  plaintextsearch_test
  regexpsearch_test
  scriptdocument_test
  vimode_test
  wordcompletiontest
  searchbar_test
  movingcursor_test
  messagetest
  kte_documentcursor
  bug313769
  katedocument_test
  movingrange_test
  kateview_test
  revision_test
  modificationsystem_test
  templatehandler_test
  katefoldingtest
  bug286887
)

kate_unit_test(completion_test codecompletiontestmodel.cpp codecompletiontestmodels.cpp)
kate_unit_test(indenttest script_test_base.cpp testutils.cpp)
kate_unit_test(commands_test script_test_base.cpp testutils.cpp)
kate_unit_test(scripting_test script_test_base.cpp testutils.cpp)
kate_unit_test(bug313759 testutils.cpp)
kate_unit_test(bug317111 testutils.cpp)
