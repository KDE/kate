project(katesessionapplet)

add_definitions(-DTRANSLATION_DOMAIN=\"plasma_applet_org.kde.plasma.katesessions\")

plasma_install_package(applet org.kde.plasma.katesessions)
add_subdirectory(engine)
