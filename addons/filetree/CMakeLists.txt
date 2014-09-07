
project(katefiletreeplugin)

set(katefiletree_PART_SRCS katefiletree.cpp katefiletreemodel.cpp katefiletreeproxymodel.cpp )
set(katefiletreeplugin_PART_SRCS katefiletreeplugin.cpp katefiletreeconfigpage.cpp katefiletreepluginsettings.cpp )

include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

add_library (katefiletree STATIC  ${katefiletree_PART_SRCS})
set_target_properties(katefiletree PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_link_libraries(katefiletree
PUBLIC
  KF5::TextEditor
  KF5::I18n
  KF5::GuiAddons
  KF5::IconThemes
)

add_library (katefiletreeplugin MODULE  ${katefiletreeplugin_PART_SRCS})
target_link_libraries(katefiletreeplugin katefiletree)

install(TARGETS katefiletreeplugin  DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor )

kcoreaddons_desktop_to_json (katefiletreeplugin katefiletreeplugin.desktop)
install( FILES katefiletreeplugin.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )

########### install files ###############

install( FILES ui.rc  DESTINATION  ${KXMLGUI_INSTALL_DIR}/katefiletree )

############# unit tests ################
ecm_optional_add_subdirectory (autotests)
