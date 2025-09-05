/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2001-2022 Christoph Cullmann <cullmann@kde.org>
   SPDX-FileCopyrightText: 2001 Joseph Wenninger <jowenn@kde.org>
   SPDX-FileCopyrightText: 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kateapp.h"

#include <KAboutData>
#include <KLocalizedString>

#ifdef WITH_DBUS
#include <KDBusService>
#endif

#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QStandardPaths>

int main(int argc, char **argv)
{
    /**
     * Do all needed pre-application init steps, shared between Kate and KWrite
     */
    KateApp::initPreApplicationCreation(false /* never detach */);

    /**
     * Create application first
     */
    QApplication app(argc, argv);

    /**
     * Enforce application name even if the executable is renamed
     * Connect application with translation catalogs, Kate & KWrite share the same one
     */
    QApplication::setApplicationName(QStringLiteral("kwrite"));
    KLocalizedString::setApplicationDomain(QByteArrayLiteral("kate"));

    /**
     * then use i18n and co
     */
    KAboutData aboutData(QStringLiteral("kwrite"),
                         i18n("KWrite"),
                         QStringLiteral(KATE_VERSION),
                         i18n("KWrite - Text Editor"),
                         KAboutLicense::LGPL_V2,
                         i18n("(c) 2000-2024 The Kate Authors"),
                         // use the other text field to get our mascot into the about dialog
                         QStringLiteral("<img height=\"362\" width=\"512\" src=\":/kate/mascot.png\"/>"),
                         QStringLiteral("https://kate-editor.org"));

    /**
     * right dbus prefix == org.kde.
     */
    aboutData.setOrganizationDomain(QByteArray("kde.org"));

    /**
     * desktop file association to make application icon work (e.g. in Wayland window decoration)
     */
    aboutData.setDesktopFileName(QStringLiteral("org.kde.kwrite"));

    /**
     * authors & co.
     * add yourself there, if you helped to work on Kate or KWrite
     */
    KateApp::initPostApplicationCreation(aboutData);

    /**
     * set proper KWrite icon for our about dialog
     */
    aboutData.setProgramLogo(QIcon(QStringLiteral(":/kwrite/kwrite.svg")));

    /**
     * bugzilla
     */
    aboutData.setProductName(QByteArray("kate/kwrite"));

    /**
     * set and register app about data
     */
    KAboutData::setApplicationData(aboutData);

    /**
     * set the program icon
     */
#ifndef Q_OS_MACOS // skip this on macOS to have proper mime-type icon visible
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/kwrite/kwrite.svg")));
#endif

    /**
     * Create command line parser and feed it with known options
     */
    QCommandLineParser parser;
    aboutData.setupCommandLine(&parser);

    // -e/--encoding option
    const QCommandLineOption useEncoding(QStringList{QStringLiteral("e"), QStringLiteral("encoding")},
                                         i18n("Set encoding for the file to open."),
                                         i18n("encoding"));
    parser.addOption(useEncoding);

    // -l/--line option
    const QCommandLineOption gotoLine(QStringList{QStringLiteral("l"), QStringLiteral("line")}, i18n("Navigate to this line."), i18n("line"));
    parser.addOption(gotoLine);

    // -c/--column option
    const QCommandLineOption gotoColumn(QStringList{QStringLiteral("c"), QStringLiteral("column")}, i18n("Navigate to this column."), i18n("column"));
    parser.addOption(gotoColumn);

    // -i/--stdin option
    const QCommandLineOption readStdIn(QStringList{QStringLiteral("i"), QStringLiteral("stdin")}, i18n("Read the contents of stdin."));
    parser.addOption(readStdIn);

    // --tempfile option
    const QCommandLineOption tempfile(QStringList{QStringLiteral("tempfile")}, i18n("The files/URLs opened by the application will be deleted after use"));
    parser.addOption(tempfile);

    // urls to open
    parser.addPositionalArgument(QStringLiteral("urls"), i18n("Documents to open."), i18n("[urls...]"));

    /**
     * do the command line parsing
     */
    parser.process(app);

    /**
     * handle standard options
     */
    aboutData.processCommandLine(&parser);

    /**
     * construct the real kate app object ;)
     * behaves like a singleton, one unique instance
     * we are passing our local command line parser to it
     */
    KateApp kateApp(parser,
                    KateApp::ApplicationKWrite,
                    QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/kwrite/sessions"));

    /**
     * init kate
     * if this returns false, we shall exit
     * else we may enter the main event loop
     */
    if (!kateApp.init()) {
        return 0;
    }

#ifdef WITH_DBUS
    /**
     * finally register this kwrite instance for dbus, don't die if no dbus is around!
     */
    const KDBusService dbusService(KDBusService::Multiple | KDBusService::NoExitOnFailure);
#endif

    /**
     * Run the event loop
     */
    return QApplication::exec();
}
