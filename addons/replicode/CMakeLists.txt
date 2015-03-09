project(katereplicodeplugin)

add_definitions(-DTRANSLATION_DOMAIN=\"kate-replicode-plugin\")

# Set source variables
set(katereplicode_SRCS
    replicodeplugin.cpp
    replicodeconfigpage.cpp
    replicodeconfig.cpp
    replicodesettings.cpp
    replicodeview.cpp
)

# Generate files
ki18n_wrap_ui(ui_SRCS config.ui)
#qt5_wrap_ui(ui_SRCS config.ui)
add_library(katereplicodeplugin MODULE ${katereplicode_SRCS} ${ui_SRCS})

# Link it all together
target_link_libraries(katereplicodeplugin
    KF5::TextEditor
    KF5::IconThemes
    KF5::I18n
)

kcoreaddons_desktop_to_json(katereplicodeplugin katereplicodeplugin.desktop)

# Install
install(TARGETS katereplicodeplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)
install(FILES ui.rc DESTINATION ${KXMLGUI_INSTALL_DIR}/katereplicodeplugin)

