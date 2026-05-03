find_package(Qt6Sql ${QT_MIN_VERSION} QUIET)
set_package_properties(Qt6Sql PROPERTIES PURPOSE "Required to build the katesql addon")

find_package(Qt6Keychain QUIET)
set_package_properties(Qt6Keychain PROPERTIES PURPOSE "Required to build the katesql addon")

if(NOT Qt6Sql_FOUND OR NOT Qt6Keychain_FOUND)
  return()
endif()

kate_add_plugin(katesqlplugin)
target_compile_definitions(katesqlplugin PRIVATE TRANSLATION_DOMAIN="katesql")

target_link_libraries(
  katesqlplugin
  PRIVATE
    KF6::I18n
    KF6::TextEditor
    KF6::KIOWidgets
    Qt::Sql
    qt6keychain
)

target_sources(
  katesqlplugin
  PRIVATE
    katesqlplugin.cpp
    katesqlview.cpp
    connectionmodel.cpp
    sqlmanager.cpp
    dataoutput/cachedsqlquerymodel.cpp
    dataoutput/dataoutputmodel.h
    dataoutput/dataoutputmodelbase.h
    dataoutput/dataoutputmodelinterface.h
    dataoutput/dataoutputeditablemodel.h
    dataoutput/dataoutputeditablerelationalmodel.h
    dataoutput/columndelegates/booleandelegate.cpp
    dataoutput/columndelegates/datedelegate.cpp
    dataoutput/columndelegates/datetimedelegate.cpp
    dataoutput/columndelegates/enumdelegate.cpp
    helpers/dataoutputstylehelper.cpp
    helpers/databaseconfigserializinghelper.cpp
    helpers/enumhelper.cpp
    helpers/foreignkeyhelper.cpp
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

if (ENABLE_PCH)
    target_precompile_headers(katesqlplugin REUSE_FROM katepch)
endif()
