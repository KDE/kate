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
  ${CMAKE_SOURCE_DIR}/part/kte5 # KDE5 remove
  ${CMAKE_BINARY_DIR}/part
  ${KDE4_KIO_INCLUDES}
)

set (KATE_TEST_LINK_LIBS ${KDE4_KDECORE_LIBS} ${QT_QTSCRIPT_LIBRARY} ${QT_QTTEST_LIBRARY} ${KDE4_KCMUTILS_LIBS} ${KDE_4_4_LIBS_NEEDED} ktexteditor katepartinterfaces)

# buffer test
kde4_add_unit_test(katetextbuffertest katetextbuffertest.cpp katetextbuffertest.h)
target_link_libraries (katetextbuffertest ${KATE_TEST_LINK_LIBS})

########### range test ###############

kde4_add_unit_test(range_test TESTNAME kate-range_test range_test.cpp)

target_link_libraries( range_test
  ${KDE4_KDEUI_LIBS}
  ${QT_QTTEST_LIBRARY}
  ${KATE_TEST_LINK_LIBS}
  katepartinterfaces
)

########### next target ###############

if(NOT MINGW)

########### undomanager test ###############

kde4_add_unit_test(undomanager_test TESTNAME kate-undomanager_test undomanager_test.cpp)

target_link_libraries( undomanager_test ${KATE_TEST_LINK_LIBS}
)

########### plaintextsearch test ###############

kde4_add_unit_test(plaintextsearch_test TESTNAME kate-plaintextsearch_test plaintextsearch_test.cpp)

target_link_libraries( plaintextsearch_test ${KATE_TEST_LINK_LIBS}
)

########### regexpsearch test ###############

kde4_add_unit_test(regexpsearch_test TESTNAME kate-regexpsearch_test regexpsearch_test.cpp)

target_link_libraries( regexpsearch_test ${KATE_TEST_LINK_LIBS}
)

########### scriptdocument test ###############

kde4_add_unit_test(scriptdocument_test TESTNAME kate-scriptdocument_test scriptdocument_test.cpp)

target_link_libraries( scriptdocument_test ${KATE_TEST_LINK_LIBS}
)

########### completion test ###############

set(completion_test_SRCS
  completion_test.cpp
  codecompletiontestmodel.cpp
)

kde4_add_unit_test(completion_test TESTNAME kate-completion_test ${completion_test_SRCS})

target_link_libraries( completion_test ${KATE_TEST_LINK_LIBS}
)
########### word completion test ###############

kde4_add_unit_test(wordcompletion_test TESTNAME kate-wordcompletion_test wordcompletiontest.cpp)

target_link_libraries( wordcompletion_test ${KATE_TEST_LINK_LIBS}
)

########### searchbar test ###############

kde4_add_unit_test(searchbar_test TESTNAME kate-searchbar_test searchbar_test.cpp)

target_link_libraries( searchbar_test
  ${KDE4_KDEUI_LIBS}
  ${QT_QTTEST_LIBRARY}
  ${KATE_TEST_LINK_LIBS}
  katepartinterfaces
)

########### vimode test ###############

kde4_add_unit_test(vimode_test TESTNAME kate-vimode_test vimode_test.cpp)



target_link_libraries( vimode_test
  ${KDE4_KDEUI_LIBS}
  ${QT_QTTEST_LIBRARY}
  ${KATE_TEST_LINK_LIBS}
  katepartinterfaces
)

########### cursor test ###############

kde4_add_unit_test(movingcursor_test TESTNAME kate-movingcursor_test movingcursor_test.cpp)

target_link_libraries( movingcursor_test
  ${KDE4_KDEUI_LIBS}
  ${QT_QTTEST_LIBRARY}
  ${KATE_TEST_LINK_LIBS}
  katepartinterfaces
)

########### moving range and feedback test ###############

kde4_add_unit_test(movingrange_test TESTNAME kate-movingrange_test movingrange_test.cpp)

target_link_libraries( movingrange_test
  ${KDE4_KDEUI_LIBS}
  ${QT_QTTEST_LIBRARY}
  ${KATE_TEST_LINK_LIBS}
  katepartinterfaces
)

########### document test ###############

kde4_add_unit_test(katedocument_test TESTNAME kate-katedocument_test katedocument_test.cpp)

target_link_libraries( katedocument_test
  ${KDE4_KDEUI_LIBS}
  ${QT_QTTEST_LIBRARY}
  ${KATE_TEST_LINK_LIBS}
  katepartinterfaces
)

########### view test ###############

kde4_add_unit_test(kateview_test TESTNAME kate-kateview_test kateview_test.cpp)

target_link_libraries( kateview_test
  ${KDE4_KDEUI_LIBS}
  ${QT_QTTEST_LIBRARY}
  ${KATE_TEST_LINK_LIBS}
  katepartinterfaces
)

########### revision test ###############

kde4_add_unit_test(revision_test TESTNAME kate-revision_test revision_test.cpp)

target_link_libraries( revision_test
  ${KDE4_KDEUI_LIBS}
  ${QT_QTTEST_LIBRARY}
  ${KATE_TEST_LINK_LIBS}
  katepartinterfaces
)

########### line modification test ###############

kde4_add_unit_test(modificationsystem_test TESTNAME kate-modificationsystem_test modificationsystem_test.cpp)

target_link_libraries( modificationsystem_test ${KATE_TEST_LINK_LIBS}
  ${KDE4_KDEUI_LIBS}
  ${QT_QTTEST_LIBRARY}
  ${KATE_TEST_LINK_LIBS}
  katepartinterfaces
)

########### templatehandler test ###############

kde4_add_unit_test(templatehandler_test TESTNAME kate-templatehandler_test templatehandler_test.cpp)

target_link_libraries( templatehandler_test
  ${KDE4_KDEUI_LIBS}
  ${QT_QTTEST_LIBRARY}
  ${KATE_TEST_LINK_LIBS}
  katepartinterfaces
)

########### text folding test ###############

kde4_add_unit_test(katecodefoldingtest TESTNAME codefoldingtest katefoldingtest.cpp)

target_link_libraries( katecodefoldingtest
  ${KDE4_KDEUI_LIBS}
  ${QT_QTTEST_LIBRARY}
  ${KATE_TEST_LINK_LIBS}
  katepartinterfaces
)

########### indenting test ################

set(indenttest_SRCS
  indenttest.cpp
  script_test_base.cpp
  testutils.cpp
)

kde4_add_unit_test(indenttest TESTNAME kate-indenttest ${indenttest_SRCS})

target_link_libraries(indenttest ${KATE_TEST_LINK_LIBS})

########### commands test ################

set(commands_test_SRCS
  commands_test.cpp
  script_test_base.cpp
  testutils.cpp
)

kde4_add_unit_test(commands_test TESTNAME kate-commands_test ${commands_test_SRCS})

target_link_libraries(commands_test ${KATE_TEST_LINK_LIBS})

########### scripting test ################

set(scripting_test_SRCS
  scripting_test.cpp
  script_test_base.cpp
  testutils.cpp
)

kde4_add_unit_test(scripting_test TESTNAME kate-scripting_test ${scripting_test_SRCS})

target_link_libraries(scripting_test ${KATE_TEST_LINK_LIBS})

########### syntax highlighting test ################

set(syntax_highlighting_test_SRCS
  syntax_highlighting_test.cpp
  script_test_base.cpp
  testutils.cpp
)

kde4_add_unit_test(syntax_highlighting_test TESTNAME kate-syntax_highlighting_test ${syntax_highlighting_test_SRCS})

target_link_libraries(syntax_highlighting_test ${KATE_TEST_LINK_LIBS})

########### bug 286887 test ###############

kde4_add_unit_test(bug286887_test TESTNAME kate-bug286887_test bug286887.cpp)
set_tests_properties(kate-bug286887_test PROPERTIES TIMEOUT 4)

target_link_libraries( bug286887_test
  ${KDE4_KDEUI_LIBS}
  ${QT_QTTEST_LIBRARY}
  ${KATE_TEST_LINK_LIBS}
  katepartinterfaces
)

########### bug 313759 test ###############

set(bug313759_SRCS
  bug313759.cpp
  testutils.cpp
)

kde4_add_unit_test(bug313759_test TESTNAME kate-bug313759_test ${bug313759_SRCS})

target_link_libraries(bug313759_test
  ${KDE4_KDEUI_LIBS}
  ${QT_QTTEST_LIBRARY}
  ${KATE_TEST_LINK_LIBS}
  katepartinterfaces
)

########### bug 313769 test ###############

kde4_add_unit_test(bug313769_test TESTNAME kate-bug313769_test bug313769.cpp)

target_link_libraries( bug313769_test
 ${KDE4_KDEUI_LIBS}
 ${QT_QTTEST_LIBRARY}
 ${KATE_TEST_LINK_LIBS}
 katepartinterfaces
)

########### bug 317111 test ###############

set(bug317111_SRCS
  bug317111.cpp
  testutils.cpp
)

kde4_add_unit_test(bug317111_test TESTNAME kate-bug317111_test ${bug317111_SRCS})

target_link_libraries( bug317111_test
  ${KDE4_KDEUI_LIBS}
  ${QT_QTTEST_LIBRARY}
  ${KATE_TEST_LINK_LIBS}
  katepartinterfaces
)

########### KTextEditor::DocumentCursor test ###############

kde4_add_unit_test(kte_documentcursor_test TESTNAME kte-documentcursor_test kte_documentcursor.cpp)

target_link_libraries( kte_documentcursor_test
  ${KDE4_KDEUI_LIBS}
  ${QT_QTTEST_LIBRARY}
  ${KATE_TEST_LINK_LIBS}
  katepartinterfaces
)

########### KTextEditor::MessageInterface test ###############

kde4_add_unit_test(kte_messageinterface_test TESTNAME kte-messageinterface messagetest.cpp)

target_link_libraries( kte_messageinterface_test
  ${KDE4_KDEUI_LIBS}
  ${QT_QTTEST_LIBRARY}
  ${KATE_TEST_LINK_LIBS}
  katepartinterfaces
)

endif(NOT MINGW)

# encoding tets
add_subdirectory (encoding)
