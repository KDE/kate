find_package(KF${KF_MAJOR_VERSION}GuiAddons ${KF5_DEP_VERSION} QUIET)
set_package_properties(KF${KF_MAJOR_VERSION}GuiAddons PROPERTIES PURPOSE "Required to build the katefiletree addon")

if(NOT KF5GuiAddons_FOUND)
  return()
endif()

kate_add_plugin(katefiletreeplugin)

target_compile_definitions(katefiletreeplugin PRIVATE TRANSLATION_DOMAIN="katefiletree")

target_link_libraries(
  katefiletreeplugin
  PUBLIC
    kateprivate
    KF${KF_MAJOR_VERSION}::I18n
    KF${KF_MAJOR_VERSION}::TextEditor
    KF${KF_MAJOR_VERSION}::GuiAddons
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


if(BUILD_TESTING)
  add_subdirectory(autotests)
endif()
