project(katesqlplugin)

add_definitions(-DTRANSLATION_DOMAIN=\"katesql\")
include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

set(katesql_SRCS
  katesqlplugin.cpp
  katesqlview.cpp
  connectionmodel.cpp
  sqlmanager.cpp
  cachedsqlquerymodel.cpp
  dataoutputmodel.cpp
  dataoutputview.cpp
  dataoutputwidget.cpp
  textoutputwidget.cpp
  schemawidget.cpp
  schemabrowserwidget.cpp
  connectionwizard.cpp
  katesqlconfigpage.cpp
  exportwizard.cpp
  outputstylewidget.cpp
  outputwidget.cpp
)

# resource for ui file and stuff
qt5_add_resources(katesql_SRCS plugin.qrc)

add_library (katesqlplugin MODULE ${katesql_SRCS})

kcoreaddons_desktop_to_json (katesqlplugin katesql.desktop)

target_link_libraries(katesqlplugin
    KF5::TextEditor
    KF5::Parts KF5::I18n KF5::Wallet
    Qt5::Sql KF5::ItemViews KF5::IconThemes)

install(TARGETS katesqlplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor )
