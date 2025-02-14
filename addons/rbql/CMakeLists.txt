kate_add_plugin(rbqlplugin)
target_compile_definitions(rbqlplugin PRIVATE TRANSLATION_DOMAIN="rbqlplugin")
target_link_libraries(rbqlplugin PRIVATE KF6::TextEditor KF6::I18n Qt::Qml Qt::Concurrent)

target_sources(
  rbqlplugin
  PRIVATE
    RBQLPlugin.cpp
    RBQLWidget.cpp
    plugin.qrc
)

if (ENABLE_PCH)
    target_precompile_headers(rbqlplugin REUSE_FROM katepch)
endif()
