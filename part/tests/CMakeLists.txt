set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
include_directories( ${KDE4_KIO_INCLUDES} )


# parent included kio already
include_directories( ${CMAKE_SOURCE_DIR}/kdeprint ${CMAKE_SOURCE_DIR}/interfaces ${CMAKE_SOURCE_DIR}/interfaces/kregexpeditor )


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

set(kateregression_SRCS
kateregression.cpp
rangeexpectation.cpp
cursorexpectation.cpp
)


kde4_add_executable(kateregression TEST ${kateregression_SRCS})

target_link_libraries(kateregression  ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY} ktexteditor )


########### install files ###############





