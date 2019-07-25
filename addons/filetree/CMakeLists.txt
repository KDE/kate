add_library(katefiletree STATIC "")
set_target_properties(katefiletree PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

# PUBLIC so that katefiletreeplugin inherits these usage requirements.
target_compile_definitions(katefiletree PUBLIC TRANSLATION_DOMAIN="katefiletree")

target_link_libraries(
  katefiletree 
  PUBLIC
    KF5::TextEditor
    KF5::I18n
    KF5::GuiAddons
    KF5::IconThemes
)

target_sources(
  katefiletree 
  PRIVATE
    katefiletree.cpp 
    katefiletreemodel.cpp 
    katefiletreeproxymodel.cpp
    plugin.qrc
)

add_library(katefiletreeplugin MODULE "")
target_link_libraries(katefiletreeplugin PRIVATE katefiletree)

target_sources(
  katefiletreeplugin 
  PRIVATE
    katefiletreeplugin.cpp 
    katefiletreeconfigpage.cpp 
    katefiletreepluginsettings.cpp
    plugin.qrc
)

kcoreaddons_desktop_to_json(katefiletreeplugin katefiletreeplugin.desktop)
install(TARGETS katefiletreeplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)

if(BUILD_TESTING)
  add_subdirectory(autotests)
endif()
