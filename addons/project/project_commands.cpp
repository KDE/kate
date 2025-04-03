/*
    SPDX-FileCopyrightText: 2024 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "project_commands.h"
#include "kateprojectpluginview.h"
#include "ktexteditor_utils.h"

#include <KLocalizedString>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>

#include <QIcon>

static QStringList commands()
{
    return {
        QStringLiteral("pexec"),
    };
};

static KateProjectPluginView *getProjectPluginView(KTextEditor::MainWindow *mainWindow)
{
    return qobject_cast<KateProjectPluginView *>(mainWindow->pluginView(QStringLiteral("kateprojectplugin")));
}

ProjectPluginCommands::ProjectPluginCommands(QObject *parent)
    : KTextEditor::Command(commands(), parent)
{
}

bool ProjectPluginCommands::help(KTextEditor::View *, const QString &cmd, QString &msg)
{
    if (cmd.indexOf(u"pexec") == 0) {
        msg = i18n("Execute a command in the active project's terminal.");
        return true;
    }
    return false;
}

bool ProjectPluginCommands::exec(KTextEditor::View *view, const QString &cmd, QString &, const KTextEditor::Range &)
{
    if (cmd.indexOf(u"pexec") == 0) {
        if (KateProjectPluginView *pluginView = getProjectPluginView(view->mainWindow())) {
            QString cmdToRun = cmd.mid(strlen("pexec") + 1);
            if (cmdToRun.isEmpty()) {
                error(i18n("No cmd provided"));
                return false;
            }
            pluginView->runCmdInTerminal(cmdToRun);
            return true;
        }
    }

    return false;
}

void ProjectPluginCommands::error(const QString &error)
{
    Utils::showMessage(error, QIcon::fromTheme(QStringLiteral("text-x-script")), i18n("Project Command"), MessageType::Error);
}

#include "moc_project_commands.cpp"
