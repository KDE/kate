if(BUILD_kate)
  add_subdirectory(kate)
endif()

remove_definitions(-DQT_NO_CAST_FROM_ASCII)

set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

include_directories(
  ${CMAKE_SOURCE_DIR}/ktexteditor
  ${CMAKE_SOURCE_DIR}/ktexteditor/buffer
  ${CMAKE_SOURCE_DIR}/ktexteditor/completion
  ${CMAKE_SOURCE_DIR}/ktexteditor/dialogs
  ${CMAKE_SOURCE_DIR}/ktexteditor/document
  ${CMAKE_SOURCE_DIR}/ktexteditor/script
  ${CMAKE_SOURCE_DIR}/ktexteditor/mode
  ${CMAKE_SOURCE_DIR}/ktexteditor/render
  ${CMAKE_SOURCE_DIR}/ktexteditor/search
  ${CMAKE_SOURCE_DIR}/ktexteditor/syntax
  ${CMAKE_SOURCE_DIR}/ktexteditor/undo
  ${CMAKE_SOURCE_DIR}/ktexteditor/utils
  ${CMAKE_SOURCE_DIR}/ktexteditor/vimode
  ${CMAKE_SOURCE_DIR}/ktexteditor/view
  ${CMAKE_BINARY_DIR}/ktexteditor
)

add_definitions(-DTEST_DATA_DIR="${CMAKE_SOURCE_DIR}/autotests/input/")
add_definitions(-DJS_DATA_DIR="${CMAKE_SOURCE_DIR}/ktexteditor/script/data/")

set (KATE_TEST_LINK_LIBS ktexteditor
  KF5::I18n
  KF5::KIOFileWidgets
  KF5::IconThemes
  KF5::WindowSystem
  KF5::GuiAddons
  Qt5::Script)

include(ECMMarkAsTest)

# test executable for encoding
add_executable(kateencodingtest src/kateencodingtest.cpp)
target_link_libraries(kateencodingtest ${KATE_TEST_LINK_LIBS})
ecm_mark_as_test(kateencodingtest)

# test macro for encoding tests
MACRO(KATE_ENCODING_TEST _encoding _testname)
  ADD_TEST (${_testname}_create kateencodingtest ${_encoding} ${CMAKE_SOURCE_DIR}/autotests/input/encoding/${_testname} ${CMAKE_CURRENT_BINARY_DIR}/${_testname} )
  ADD_TEST (${_testname}_diff ${CMAKE_COMMAND} -E compare_files ${CMAKE_SOURCE_DIR}/autotests/input/encoding/${_testname} ${CMAKE_CURRENT_BINARY_DIR}/${_testname} )
ENDMACRO(KATE_ENCODING_TEST)

# add tests

# this file is utf-8, simple
KATE_ENCODING_TEST ("utf-8" "utf8.txt")

# this file is latin15, but fallback should work!
KATE_ENCODING_TEST ("utf-8" "latin15.txt")

# this file is utf32, little endian, but fallback should work!
KATE_ENCODING_TEST ("utf-8" "utf32.txt")

# this file is utf16, little endian, but fallback should work!
KATE_ENCODING_TEST ("utf-8" "utf16.txt")

# this file is utf32, big endian, but fallback should work!
KATE_ENCODING_TEST ("utf-8" "utf32be.txt")

# this file is utf16, big endian, but fallback should work!
KATE_ENCODING_TEST ("utf-8" "utf16be.txt")

# cyrillic utf-8
KATE_ENCODING_TEST ("utf-8" "cyrillic_utf8.txt")

# cyrillic cp1251
KATE_ENCODING_TEST ("utf-8" "cp1251.txt")

# cyrillic koi8-r
KATE_ENCODING_TEST ("utf-8" "koi8-r.txt")

# one character latin-15 test, segfaulted
KATE_ENCODING_TEST ("utf-8" "one-char-latin-15.txt")

macro(kate_unit_tests)
  foreach(_testname ${ARGN})
    add_executable(${_testname} src/${_testname}.cpp)
    add_test(kate-${_testname} ${_testname})
    target_link_libraries(${_testname} ${KATE_TEST_LINK_LIBS} Qt5::Test)
    ecm_mark_as_test(${_testname})
  endforeach()
endmacro()

macro(kate_unit_test testname)
  add_executable(${testname} src/${testname}.cpp ${ARGN})
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
  katewildcardmatcher_test
)

kate_unit_test(completion_test src/codecompletiontestmodel.cpp src/codecompletiontestmodels.cpp)
kate_unit_test(indenttest src/script_test_base.cpp src/testutils.cpp)
kate_unit_test(commands_test src/script_test_base.cpp src/testutils.cpp)
kate_unit_test(scripting_test src/script_test_base.cpp src/testutils.cpp)
kate_unit_test(bug313759 src/testutils.cpp)
kate_unit_test(bug317111 src/testutils.cpp)
