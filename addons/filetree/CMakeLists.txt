kate_add_plugin(katefiletreeplugin)

target_compile_definitions(katefiletreeplugin PRIVATE TRANSLATION_DOMAIN="katefiletree")

target_link_libraries(
  katefiletreeplugin
  PUBLIC
    kateprivate
    KF6::I18n
    KF6::TextEditor
    KF6::GuiAddons
)

target_sources(
  katefiletreeplugin
  PRIVATE
    katefiletree.cpp
    katefiletreeconfigpage.cpp
    katefiletreemodel.cpp
    katefiletreeplugin.cpp
    katefiletreepluginsettings.cpp
    katefiletreeproxymodel.cpp
    plugin.qrc
)

if (BUILD_PCH)
    target_precompile_headers(katefiletreeplugin REUSE_FROM katepch)
endif()

if(BUILD_TESTING)
  add_subdirectory(autotests)
endif()
