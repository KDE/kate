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
   #bug313769
   #katedocument_test
   movingrange_test
   #kateview_test
   revision_test
   modificationsystem_test
   templatehandler_test
   #katefoldingtest
   bug286887
 )

# ########### completion test ###############
# 
# set(completion_test_SRCS
#   completion_test.cpp
#   codecompletiontestmodel.cpp
# )
# 
# kde4_add_unit_test(completion_test TESTNAME kate-completion_test ${completion_test_SRCS})
# 
# target_link_libraries( completion_test ${KATE_TEST_LINK_LIBS}
# )
# 
# 
# 
# ########### indenting test ################
# 
# set(indenttest_SRCS
#   indenttest.cpp
#   script_test_base.cpp
#   testutils.cpp
# )
# 
# kde4_add_unit_test(indenttest TESTNAME kate-indenttest ${indenttest_SRCS})
# 
# target_link_libraries(indenttest ${KATE_TEST_LINK_LIBS})
# 
# ########### commands test ################
# 
# set(commands_test_SRCS
#   commands_test.cpp
#   script_test_base.cpp
#   testutils.cpp
# )
# 
# kde4_add_unit_test(commands_test TESTNAME kate-commands_test ${commands_test_SRCS})
# 
# target_link_libraries(commands_test ${KATE_TEST_LINK_LIBS})
# 
# ########### scripting test ################
# 
# set(scripting_test_SRCS
#   scripting_test.cpp
#   script_test_base.cpp
#   testutils.cpp
# )
# 
# kde4_add_unit_test(scripting_test TESTNAME kate-scripting_test ${scripting_test_SRCS})
# 
# target_link_libraries(scripting_test ${KATE_TEST_LINK_LIBS})
# 
# ########### bug 313759 test ###############
# 
# set(bug313759_SRCS
#   bug313759.cpp
#   testutils.cpp
# )
# 
# kde4_add_unit_test(bug313759_test TESTNAME kate-bug313759_test ${bug313759_SRCS})
# 
# target_link_libraries(bug313759_test
#   ${KDE4_KDEUI_LIBS}
#   ${QT_QTTEST_LIBRARY}
#   ${KATE_TEST_LINK_LIBS}
#   katepartinterfaces
# )
# 
# 
# ########### bug 317111 test ###############
# 
# set(bug317111_SRCS
#   bug317111.cpp
#   testutils.cpp
# )
# 
# kde4_add_unit_test(bug317111_test TESTNAME kate-bug317111_test ${bug317111_SRCS})
# 
# target_link_libraries( bug317111_test
#   ${KDE4_KDEUI_LIBS}
#   ${QT_QTTEST_LIBRARY}
#   ${KATE_TEST_LINK_LIBS}
#   katepartinterfaces
# )
# 