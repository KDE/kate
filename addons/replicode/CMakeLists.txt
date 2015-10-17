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
ki18n_wrap_ui(katereplicode_SRCS config.ui)

# resource for ui file and stuff
qt5_add_resources(katereplicode_SRCS plugin.qrc)

add_library(katereplicodeplugin MODULE ${katereplicode_SRCS})

kcoreaddons_desktop_to_json(katereplicodeplugin katereplicodeplugin.desktop)

# Link it all together
target_link_libraries(katereplicodeplugin
    KF5::TextEditor
    KF5::IconThemes
    KF5::I18n
)

# Install
install(TARGETS katereplicodeplugin DESTINATION ${PLUGIN_INSTALL_DIR}/ktexteditor)
