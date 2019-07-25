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
  plugin.qrc
)

add_library(katesqlplugin MODULE ${katesql_SRCS})
target_compile_definitions(katesqlplugin PRIVATE TRANSLATION_DOMAIN="katesql")

target_link_libraries(
  katesqlplugin 
  PRIVATE
    Qt5::Sql   
    KF5::TextEditor
    KF5::Parts 
    KF5::I18n 
    KF5::Wallet
    KF5::ItemViews 
    KF5::IconThemes
)

kcoreaddons_desktop_to_json(katesqlplugin katesql.desktop)
install(TARGETS katesqlplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor )
