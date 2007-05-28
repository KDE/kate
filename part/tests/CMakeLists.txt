include_directories( ${KDE4_KIO_INCLUDES} )

if(KDE4_BUILD_TESTS)

# parent included kio already
include_directories( ${CMAKE_SOURCE_DIR}/kdeprint ${CMAKE_SOURCE_DIR}/interfaces ${CMAKE_SOURCE_DIR}/interfaces/kregexpeditor )


########### next target ###############

set(katetest_SRCS
katetest.cpp
arbitraryhighlighttest.cpp
codecompletiontest.cpp
)

kde4_automoc(${katetest_SRCS})

kde4_add_executable(katetest ${katetest_SRCS})

target_link_libraries(katetest  ${KDE4_KDECORE_LIBS} ktexteditor )


########### next target ###############

set(kateregression_SRCS
kateregression.cpp
rangeexpectation.cpp
cursorexpectation.cpp
)

kde4_automoc(${kateregression_SRCS})

kde4_add_executable(kateregression ${kateregression_SRCS})

target_link_libraries(kateregression  ${KDE4_KDECORE_LIBS} ${QT_QTTEST_LIBRARY} ktexteditor )

endif(KDE4_BUILD_TESTS)

########### install files ###############





