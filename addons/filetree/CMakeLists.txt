find_package(KF5GuiAddons QUIET)
set_package_properties(KF5GuiAddons PROPERTIES PURPOSE "Required to build the katefiletree addon")

if(NOT KF5GuiAddons_FOUND)
  return()
endif()

add_library(katefiletree MODULE "")
target_compile_definitions(katefiletree PRIVATE TRANSLATION_DOMAIN="katefiletree")

target_link_libraries(
  katefiletree
  PUBLIC
    KF5::TextEditor
    KF5::GuiAddons
)

target_sources(
  katefiletree
  PRIVATE
    katefiletree.cpp
    katefiletreeconfigpage.cpp
    katefiletreemodel.cpp
    katefiletreeplugin.cpp
    katefiletreepluginsettings.cpp
    katefiletreeproxymodel.cpp
    plugin.qrc
)

kcoreaddons_desktop_to_json(katefiletree katefiletreeplugin.desktop)
install(TARGETS katefiletree DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)

if(BUILD_TESTING)
  add_subdirectory(autotests)
endif()
