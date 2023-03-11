find_package(KF${KF_MAJOR_VERSION}Wallet ${KF5_DEP_VERSION} QUIET)
set_package_properties(KF${KF_MAJOR_VERSION}Wallet PROPERTIES PURPOSE "Required to build the katesql addon")

find_package(Qt${QT_MAJOR_VERSION}Sql ${QT_MIN_VERSION} QUIET)
set_package_properties(Qt${QT_MAJOR_VERSION}Sql PROPERTIES PURPOSE "Required to build the katesql addon")

if(NOT KF5Wallet_FOUND OR NOT Qt${QT_MAJOR_VERSION}Sql_FOUND)
  return()
endif()

kate_add_plugin(katesqlplugin)
target_compile_definitions(katesqlplugin PRIVATE TRANSLATION_DOMAIN="katesql")

target_link_libraries(
  katesqlplugin 
  PRIVATE
    KF${KF_MAJOR_VERSION}::I18n
    KF${KF_MAJOR_VERSION}::TextEditor
    KF${KF_MAJOR_VERSION}::Wallet
    KF${KF_MAJOR_VERSION}::KIOWidgets
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
