project(demo)
find_package(KDE4 REQUIRED)
include_directories(
  ${KDE4_INCLUDES}
  .
  ../
  ../../kte5
)

set(demo_SRCS
  main.cpp
  ../katemessagewidget.cpp
  ../../kte5/messageinterface.cpp
)

kde4_add_executable(demo ${demo_SRCS} )
target_link_libraries(demo ${KDE4_KDEUI_LIBS} ${KDE4_KPARTS_LIBS} )
