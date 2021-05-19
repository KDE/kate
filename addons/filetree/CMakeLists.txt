find_package(KF5GuiAddons ${KF5_DEP_VERSION} QUIET)
set_package_properties(KF5GuiAddons PROPERTIES PURPOSE "Required to build the katefiletree addon")

if(NOT KF5GuiAddons_FOUND)
  return()
endif()

kcoreaddons_add_plugin(katefiletreeplugin INSTALL_NAMESPACE "ktexteditor")

target_compile_definitions(katefiletreeplugin PRIVATE TRANSLATION_DOMAIN="katefiletree")

target_link_libraries(
  katefiletreeplugin
  PUBLIC
    KF5::I18n
    KF5::TextEditor
    KF5::GuiAddons
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
