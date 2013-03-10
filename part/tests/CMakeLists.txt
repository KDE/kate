set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${CMAKE_CURRENT_SOURCE_DIR}/../buffer
  ${CMAKE_CURRENT_SOURCE_DIR}/../completion
  ${CMAKE_CURRENT_SOURCE_DIR}/../dialogs
  ${CMAKE_CURRENT_SOURCE_DIR}/../document
  ${CMAKE_CURRENT_SOURCE_DIR}/../script
  ${CMAKE_CURRENT_SOURCE_DIR}/../mode
  ${CMAKE_CURRENT_SOURCE_DIR}/../render
  ${CMAKE_CURRENT_SOURCE_DIR}/../search
  ${CMAKE_CURRENT_SOURCE_DIR}/../syntax
  ${CMAKE_CURRENT_SOURCE_DIR}/../undo
  ${CMAKE_CURRENT_SOURCE_DIR}/../utils
  ${CMAKE_CURRENT_SOURCE_DIR}/../vimode
  ${CMAKE_CURRENT_SOURCE_DIR}/../view
  ${CMAKE_CURRENT_SOURCE_DIR}/../kte5 # KDE5 remove
  ${CMAKE_CURRENT_BINARY_DIR}/..
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

########### folding test ###############

kde4_add_unit_test(katefolding_test TESTNAME kate-katefolding_test katefolding_test.cpp)

target_link_libraries( katefolding_test
  ${KDE4_KDEUI_LIBS}
  ${QT_QTTEST_LIBRARY}
  ${KATE_TEST_LINK_LIBS}
  katepartinterfaces
)

########### folded selection test ###############

kde4_add_unit_test(katefoldedselection_test TESTNAME kate-katefoldedselection_test foldedselection_test.cpp)

target_link_libraries( katefoldedselection_test
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

########### bug 213964 test ###############

kde4_add_unit_test(bug213964_test TESTNAME kate-bug213964_test bug213964.cpp)

target_link_libraries( bug213964_test
  ${KDE4_KDEUI_LIBS}
  ${QT_QTTEST_LIBRARY}
  ${KATE_TEST_LINK_LIBS}
  katepartinterfaces
)

########### bug 294241 test ###############

kde4_add_unit_test(bug294241_test TESTNAME kate-bug294241_test bug294241.cpp)

target_link_libraries( bug294241_test
  ${KDE4_KDEUI_LIBS}
  ${QT_QTTEST_LIBRARY}
  ${KATE_TEST_LINK_LIBS}
  katepartinterfaces
)

########### bug 294756 test ###############

kde4_add_unit_test(bug294756_test TESTNAME kate-bug294756_test bug294756.cpp)

target_link_libraries( bug294756_test
  ${KDE4_KDEUI_LIBS}
  ${QT_QTTEST_LIBRARY}
  ${KATE_TEST_LINK_LIBS}
  katepartinterfaces
)

########### bug 294750 test ###############

kde4_add_unit_test(bug294750_test TESTNAME kate-bug294750_test bug294750.cpp)

target_link_libraries( bug294750_test
  ${KDE4_KDEUI_LIBS}
  ${QT_QTTEST_LIBRARY}
  ${KATE_TEST_LINK_LIBS}
  katepartinterfaces
)

########### bug 286887 test ###############

kde4_add_unit_test(bug286887_test TESTNAME kate-bug286887_test bug286887.cpp)
set_tests_properties(kate-bug286887_test PROPERTIES TIMEOUT 4)

target_link_libraries( bug286887_test
  ${KDE4_KDEUI_LIBS}
  ${QT_QTTEST_LIBRARY}
  ${KATE_TEST_LINK_LIBS}
  katepartinterfaces
)

########### bug 309093 test ###############

kde4_add_unit_test(bug309093_test TESTNAME kate-bug309093_test bug309093.cpp)

target_link_libraries( bug309093_test
  ${KDE4_KDEUI_LIBS}
  ${QT_QTTEST_LIBRARY}
  ${KATE_TEST_LINK_LIBS}
  katepartinterfaces
)

########### bug 309093 test ###############

kde4_add_unit_test(bug313769_test TESTNAME kate-bug313769_test bug313769.cpp)

target_link_libraries( bug313769_test
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

endif(NOT MINGW)

# encoding tets
add_subdirectory (encoding)
