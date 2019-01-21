/* This file is part of the KDE project
 *
 *  Copyright 2019 Dominik Haumann <dhaumann@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */
#include "externaltools.h"
#include "externaltoolsplugin.h"

#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/View>
#include <KActionCollection>

KateExternalToolsCommand::KateExternalToolsCommand(KateExternalToolsPlugin* plugin)
    : KTextEditor::Command({/* FIXME */})
    , m_plugin(plugin)
{
//     reload();
}

// FIXME
// const QStringList& KateExternalToolsCommand::cmds()
// {
//     return m_list;
// }
#if 0
void KateExternalToolsCommand::reload()
{
    m_list.clear();
    m_map.clear();
    m_name.clear();

    KConfig _config(QStringLiteral("externaltools"), KConfig::NoGlobals, QStandardPaths::ApplicationsLocation);
    KConfigGroup config(&_config, "Global");
    const QStringList tools = config.readEntry("tools", QStringList());

    for (QStringList::const_iterator it = tools.begin(); it != tools.end(); ++it) {
        if (*it == QStringLiteral("---"))
            continue;

        config = KConfigGroup(&_config, *it);

        KateExternalTool t;
        t.load(config);
        // FIXME test for a command name first!
        if (t.hasexec && (!t.cmdname.isEmpty())) {
            m_list.append(QStringLiteral("exttool-") + t.cmdname);
            m_map.insert(QStringLiteral("exttool-") + t.cmdname, t.actionName);
            m_name.insert(QStringLiteral("exttool-") + t.cmdname, t.name);
        }
    }
}
#endif
bool KateExternalToolsCommand::exec(KTextEditor::View* view, const QString& cmd, QString& msg,
                                    const KTextEditor::Range& range)
{
    Q_UNUSED(msg)
    Q_UNUSED(range)

    auto wv = dynamic_cast<QWidget*>(view);
    if (!wv) {
        //   qDebug()<<"KateExternalToolsCommand::exec: Could not get view widget";
        return false;
    }

    //  qDebug()<<"cmd="<<cmd.trimmed();
    const QString actionName = m_map[cmd.trimmed()];
    if (actionName.isEmpty())
        return false;
    //  qDebug()<<"actionName is not empty:"<<actionName;
    /*  KateExternalToolsMenuAction *a =
        dynamic_cast<KateExternalToolsMenuAction*>(dmw->action("tools_external"));
      if (!a) return false;*/
    KateExternalToolsPluginView* extview = m_plugin->extView(wv->window());
    if (!extview)
        return false;
    if (!extview->externalTools)
        return false;
    //  qDebug()<<"trying to find action";
    QAction* a1 = extview->externalTools->actionCollection()->action(actionName);
    if (!a1)
        return false;
    //  qDebug()<<"activating action";
    a1->trigger();
    return true;
}

bool KateExternalToolsCommand::help(KTextEditor::View*, const QString&, QString&)
{
    return false;
}

// kate: space-indent on; indent-width 4; replace-tabs on;
