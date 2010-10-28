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
  ${CMAKE_CURRENT_SOURCE_DIR}/../smart
  ${CMAKE_CURRENT_SOURCE_DIR}/../syntax
  ${CMAKE_CURRENT_SOURCE_DIR}/../undo
  ${CMAKE_CURRENT_SOURCE_DIR}/../utils
  ${CMAKE_CURRENT_SOURCE_DIR}/../vimode
  ${CMAKE_CURRENT_SOURCE_DIR}/../view
  ${CMAKE_CURRENT_BINARY_DIR}/..
  ${KDE4_KIO_INCLUDES}
)

set (KATE_TEST_LINK_LIBS ${KDE4_KDECORE_LIBS} ${QT_QTSCRIPT_LIBRARY} ${QT_QTTEST_LIBRARY} ${KDE4_KCMUTILS_LIBS} ${KDE_4_4_LIBS_NEEDED} ktexteditor katepartinterfaces)

# buffer test
kde4_add_unit_test(katetextbuffertest katetextbuffertest.cpp katetextbuffertest.h)
target_link_libraries (katetextbuffertest ${KATE_TEST_LINK_LIBS})

########### next target ###############

set(katetest_SRCS
katetest.cpp
arbitraryhighlighttest.cpp
codecompletiontestmodel.cpp
annotationmodeltest.cpp
)


kde4_add_executable(katetest TEST ${katetest_SRCS})

target_link_libraries(katetest ${KATE_TEST_LINK_LIBS} )


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

########### revision test ###############

kde4_add_unit_test(revision_test TESTNAME kate-revision_test revision_test.cpp)

target_link_libraries( revision_test
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
  testutils.cpp
)

kde4_add_unit_test(indenttest TESTNAME kate-indenttest ${indenttest_SRCS})

target_link_libraries(indenttest ${KATE_TEST_LINK_LIBS})

########### bug 213964 test ###############

kde4_add_unit_test(bug213964_test TESTNAME kate-bug213964_test bug213964.cpp)

target_link_libraries( bug213964_test
  ${KDE4_KDEUI_LIBS}
  ${QT_QTTEST_LIBRARY}
  ${KATE_TEST_LINK_LIBS}
  katepartinterfaces
)

endif(NOT MINGW)

# encoding tets
add_subdirectory (encoding)
