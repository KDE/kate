/*
    SPDX-FileCopyrightText: 2024 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "mainwindow_commands.h"

#include "katemainwindow.h"
#include "ktexteditor_utils.h"
#include <KLocalizedString>

#include <QIcon>

static QStringList commands()
{
    return {
        QStringLiteral("showtoolview"),
        QStringLiteral("hidetoolview"),
    };
};

MainWindowCommands::MainWindowCommands(KateMainWindow *window)
    : KTextEditor::Command(commands(), window)
    , m_mainWindow(window)
{
}

bool MainWindowCommands::exec(KTextEditor::View *, const QString &cmd, QString &, const KTextEditor::Range &)
{
    const QStringList args(cmd.split(QLatin1Char(' '), Qt::KeepEmptyParts));
    if (args.isEmpty()) {
        return false;
    }

    if (args[0] == u"showtoolview" || args[0] == u"hidetoolview") {
        if (args.size() > 1) {
            const bool show = args[0] == u"showtoolview";
            if (QWidget *toolview = m_mainWindow->toolviewForName(args[1])) {
                if (show) {
                    m_mainWindow->showToolView(toolview);
                } else {
                    m_mainWindow->hideToolView(toolview);
                }
                return true;
            } else {
                error(i18n("No toolview found for name: '%1'. Is the plugin for it disabled or the name is misspelled?", toolviewNames()));
            }
        } else {
            error(i18n("Script error. Expected a toolview name. Available: %1", toolviewNames()));
        }
    }
    return false;
}

void MainWindowCommands::error(const QString &error)
{
    Utils::showMessage(error, QIcon::fromTheme(QStringLiteral("text-x-script")), i18n("Scripting"), MessageType::Error, m_mainWindow->wrapper());
}

QString MainWindowCommands::toolviewNames() const
{
    const auto names = m_mainWindow->toolviewNames();
    QString helpstr;
    for (const auto &id : names) {
        helpstr += QStringLiteral("%1, ").arg(id);
    }
    if (!helpstr.isEmpty() && helpstr.back() == u',') {
        helpstr.chop(1);
    }
    return helpstr;
}

bool MainWindowCommands::help(KTextEditor::View *, const QString &cmd, QString &help)
{
    bool res = false;
    if (cmd == u"showtoolview") {
        res = true;
        help += i18n("'showtoolview $toolview_name'. Show a toolview panel. Available panels: %1", toolviewNames());
    } else if (cmd == u"hidetoolview") {
        help += i18n("'showtoolview $toolview_name'. Hide a toolview panel. Available panels: %1", toolviewNames());
        res = true;
    }
    return res;
}

#include "moc_mainwindow_commands.cpp"
