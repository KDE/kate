set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/..
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

########### next target ###############

set(katetest_SRCS
katetest.cpp
arbitraryhighlighttest.cpp
codecompletiontestmodel.cpp
annotationmodeltest.cpp
)


kde4_add_executable(katetest TEST ${katetest_SRCS})

target_link_libraries(katetest  ${KDE4_KDECORE_LIBS} ktexteditor )


########### next target ###############

set(cursorrange_regression_SRCS
  cursorrange_regression.cpp
  rangeexpectation.cpp
  cursorexpectation.cpp
)

kde4_add_unit_test(cursorrange_regression ${cursorrange_regression_SRCS})

target_link_libraries(cursorrange_regression  ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY} ktexteditor )


########### next target ###############

if(NOT MINGW)

# This can be used for finding data files in the source dir, without installing them
#add_definitions( -DKDESRCDIR=\\"${CMAKE_CURRENT_SOURCE_DIR}\\" )

#include_directories( ${KDE4_KDECORE_INCLUDES} )

set(testkateregression_SRCS
  test_regression.cpp
  codecompletiontestmodel.cpp
  completiontest.cpp
)

kde4_add_unit_test(testkateregression NOGUI ${testkateregression_SRCS})

target_link_libraries(testkateregression ${KDE4_KDECORE_LIBS} ${QT_QTSCRIPT_LIBRARY} ${QT_QTTEST_LIBRARY} kutils ktexteditor kateparttestlib)

#add_dependencies(check testkateregression)

add_custom_target(check ${CMAKE_CURRENT_BINARY_DIR}/testkateregression DEPENDS testkateregression WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} )

########### undomanager test ###############

kde4_add_unit_test(undomanager_test undomanager_test.cpp)

target_link_libraries( undomanager_test
  ${KDE4_KDECORE_LIBS}
  ${KDE4_KDEUI_LIBS}
  ${QT_QTTEST_LIBRARY}
  kateparttestlib
)

########### plaintextsearch test ###############

kde4_add_unit_test(plaintextsearch_test plaintextsearch_test.cpp)

target_link_libraries( plaintextsearch_test
  ${KDE4_KDECORE_LIBS}
  ${KDE4_KDEUI_LIBS}
  ${QT_QTTEST_LIBRARY}
  kateparttestlib
)

endif(NOT MINGW)

########### install files ###############





