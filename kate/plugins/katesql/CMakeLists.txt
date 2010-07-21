if(NOT KDE4_FOUND)
  find_package(KDE4 REQUIRED)
  include_directories( ${KDE4_INCLUDES} ${QT_INCLUDES} )
endif(NOT KDE4_FOUND)

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

set(katesql_SRCS
  katesqlplugin.cpp
  katesqlview.cpp
  connectionmodel.cpp
  sqlmanager.cpp
  dataoutputmodel.cpp
  dataoutputview.cpp
  dataoutputwidget.cpp
  textoutputwidget.cpp
  schemawidget.cpp
  schemabrowserwidget.cpp
  connectionwizard.cpp
  katesqlconfigpage.cpp
  exportwizard.cpp
)

kde4_add_plugin(katesqlplugin ${katesql_SRCS})

add_subdirectory(pics)

target_link_libraries(katesqlplugin ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${QT_QTSQL_LIBRARY} kateinterfaces ${KDE4_KTEXTEDITOR_LIBS} )

install(TARGETS katesqlplugin DESTINATION ${PLUGIN_INSTALL_DIR} )

########### install files ###############

install( FILES ui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kate/plugins/katesql )
install( FILES katesql.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )
