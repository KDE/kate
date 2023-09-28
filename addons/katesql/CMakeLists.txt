find_package(KF6Wallet ${KF5_DEP_VERSION} QUIET)
set_package_properties(KF6Wallet PROPERTIES PURPOSE "Required to build the katesql addon")

find_package(Qt6Sql ${QT_MIN_VERSION} QUIET)
set_package_properties(Qt6Sql PROPERTIES PURPOSE "Required to build the katesql addon")

if(NOT KF5Wallet_FOUND OR NOT Qt6Sql_FOUND)
  return()
endif()

kate_add_plugin(katesqlplugin)
target_compile_definitions(katesqlplugin PRIVATE TRANSLATION_DOMAIN="katesql")

target_link_libraries(
  katesqlplugin 
  PRIVATE
    KF6::I18n
    KF6::TextEditor
    KF6::Wallet
    KF6::KIOWidgets
    Qt::Sql
)

target_sources(
  katesqlplugin
  PRIVATE
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

if (BUILD_PCH)
    target_precompile_headers(katesqlplugin REUSE_FROM katepch)
endif()
