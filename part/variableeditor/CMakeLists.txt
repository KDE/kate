project(editor)

cmake_minimum_required(VERSION 2.6)

# enable warnings
add_definitions( -Wall )

find_package(Qt4 REQUIRED)
INCLUDE( ${QT_USE_FILE} )

find_package(KDE4 REQUIRED)


set(editor
  main.cpp
  variablelistview.cpp
  variableitem.cpp
  variableeditor.cpp
)

# another list, this time it includes all header files that should be treated with moc
set(editor_MOC_HDRS
  variablelistview.h
  variableeditor.h
)

# and finally this will run moc:
QT4_WRAP_CPP( editor_MOC_SRCS ${editor_MOC_HDRS} )

# we need this to be able to include headers produced by moc/uic in our code
include_directories( ${CMAKE_BINARY_DIR} )

add_executable( editor ${editor} ${editor_MOC_SRCS} )

target_link_libraries( editor ${QT_LIBRARIES} kdeui )

# kate: indent-width 2; replace-tabs on;
