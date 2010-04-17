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
  ${KDE4_KIO_INCLUDES}
)

set (KATE_TEST_LINK_LIBS ${KDE4_KDECORE_LIBS} ${QT_QTSCRIPT_LIBRARY} ${QT_QTTEST_LIBRARY} kutils ktexteditor katepartinterfaces)

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


########### next target ###############

set(cursorrange_regression_SRCS
  cursorrange_regression.cpp
  rangeexpectation.cpp
  cursorexpectation.cpp
)

kde4_add_unit_test(cursorrange_regression ${cursorrange_regression_SRCS})

target_link_libraries(cursorrange_regression ${KATE_TEST_LINK_LIBS} )

########### next target ###############

if(NOT MINGW)

# This can be used for finding data files in the source dir, without installing them
#add_definitions( -DKDESRCDIR=\\"${CMAKE_CURRENT_SOURCE_DIR}\\" )

#include_directories( ${KDE4_KDECORE_INCLUDES} )

set(testkateregression_SRCS
  test_regression.cpp
)

kde4_add_unit_test(testkateregression NOGUI ${testkateregression_SRCS})

target_link_libraries(testkateregression ${KATE_TEST_LINK_LIBS})

#add_dependencies(check testkateregression)

add_custom_target(check ${CMAKE_CURRENT_BINARY_DIR}/testkateregression DEPENDS testkateregression WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} )

########### undomanager test ###############

kde4_add_unit_test(undomanager_test undomanager_test.cpp)

target_link_libraries( undomanager_test ${KATE_TEST_LINK_LIBS}
)

########### plaintextsearch test ###############

kde4_add_unit_test(plaintextsearch_test plaintextsearch_test.cpp)

target_link_libraries( plaintextsearch_test ${KATE_TEST_LINK_LIBS}
)

########### regexpsearch test ###############

kde4_add_unit_test(regexpsearch_test regexpsearch_test.cpp)

target_link_libraries( regexpsearch_test ${KATE_TEST_LINK_LIBS}
)

########### scriptdocument test ###############

kde4_add_unit_test(scriptdocument_test scriptdocument_test.cpp)

target_link_libraries( scriptdocument_test ${KATE_TEST_LINK_LIBS}
)

########### completion test ###############

set(completion_test_SRCS
  completion_test.cpp
  codecompletiontestmodel.cpp
)

kde4_add_unit_test(completion_test ${completion_test_SRCS})

target_link_libraries( completion_test ${KATE_TEST_LINK_LIBS}
)

########### searchbar test ###############

kde4_add_unit_test(searchbar_test searchbar_test.cpp)

target_link_libraries( searchbar_test
  ${KDE4_KDECORE_LIBS}
  ${KDE4_KDEUI_LIBS}
  ${QT_QTTEST_LIBRARY}
  katepartinterfaces
)

endif(NOT MINGW)

# encoding tets
add_subdirectory (encoding)
