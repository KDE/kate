project(katesessionapplet)

find_package(KF5 REQUIRED
    I18n WindowSystem Service Completion WidgetsAddons KIO CoreAddons Wallet ItemViews XmlGui
    ConfigWidgets IconThemes Solid DBusAddons Notifications Plasma Declarative Init KDELibs4Support)


plasma_install_package(applet org.kde.plasma.katesessions)
add_subdirectory(engine)
