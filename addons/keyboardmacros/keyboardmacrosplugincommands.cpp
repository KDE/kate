/*
 * SPDX-FileCopyrightText: 2022 Pablo Rauzy <r .at. uzy .dot. me>
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <QString>
#include <QStringList>

#include <KLocalizedString>

#include <KTextEditor/Command>
#include <KTextEditor/Range>
#include <KTextEditor/View>

#include "keyboardmacrosplugin.h"
#include "keyboardmacrosplugincommands.h"

KeyboardMacrosPluginCommands::KeyboardMacrosPluginCommands(KeyboardMacrosPlugin *plugin)
    : KTextEditor::Command(QStringList() << QStringLiteral("kmsave") << QStringLiteral("kmload") << QStringLiteral("kmplay") << QStringLiteral("kmwipe"),
                           plugin)
    , m_plugin(plugin)
{
}

bool KeyboardMacrosPluginCommands::exec(KTextEditor::View *view, const QString &cmd, QString &msg, const KTextEditor::Range &)
{
    const QStringList &actionAndName = cmd.split(QRegularExpression(QStringLiteral("\\s+")));
    const QString &action = actionAndName[0];
    // kmplay can take either zero or one argument, all other commands require exactly one
    if (actionAndName.length() > 2 || (action != QStringLiteral("kmplay") && actionAndName.length() != 2)) {
        msg = i18n("Usage: %1 <name>.", action);
        return false;
    }
    if (action == QStringLiteral("kmplay")) {
        // set focus on the view otherwise the macro is executed in the command line
        view->setFocus();
        if (actionAndName.length() == 1) {
            // no argument: play the current macro
            m_plugin->play();
            return true;
        } else {
            // otherwise play the given macro
            const QString &name = actionAndName[1];
            if (!m_plugin->play(name)) {
                msg = i18n("No keyboard macro named '%1' found.", name);
                return false;
            }
            return true;
        }
    }
    const QString &name = actionAndName[1];
    if (action == QStringLiteral("kmsave")) {
        if (!m_plugin->save(name)) {
            msg = i18n("Cannot save empty keyboard macro.");
            return false;
        }
        return true;
    } else if (action == QStringLiteral("kmload")) {
        if (!m_plugin->load(name)) {
            msg = i18n("No keyboard macro named '%1' found.", name);
            return false;
        }
        return true;
    } else if (action == QStringLiteral("kmwipe")) {
        if (!m_plugin->wipe(name)) {
            msg = i18n("No keyboard macro named '%1' found.", name);
            return false;
        }
        return true;
    }
    return false;
}

bool KeyboardMacrosPluginCommands::help(KTextEditor::View *, const QString &cmd, QString &msg)
{
    QString macros;
    if (!m_plugin->m_namedMacros.keys().isEmpty()) {
        macros = QStringLiteral("<p><b>Named macros:</b> ") + QStringList(m_plugin->m_namedMacros.keys()).join(QStringLiteral(", ")) + QStringLiteral(".</p>");
    }
    if (cmd == QStringLiteral("kmsave")) {
        msg = i18n("<qt><p>Usage: <code>kmsave &lt;name&gt;</code></p><p>Save current keyboard macro as <code>&lt;name&gt;</code>.</p>%1</qt>", macros);
        return true;
    } else if (cmd == QStringLiteral("kmload")) {
        msg = i18n("<qt><p>Usage: <code>kmload &lt;name&gt;</code></p><p>Load saved keyboard macro <code>&lt;name&gt;</code> as current macro.</p>%1</qt>",
                   macros);
        return true;
    } else if (cmd == QStringLiteral("kmplay")) {
        msg = i18n("<qt><p>Usage: <code>kmplay &lt;name&gt;</code></p><p>Play saved keyboard macro <code>&lt;name&gt;</code> without loading it.</p>%1</qt>",
                   macros);
        return true;
    } else if (cmd == QStringLiteral("kmwipe")) {
        msg = i18n("<qt><p>Usage: <code>kmwipe &lt;name&gt;</code></p><p>Wipe saved keyboard macro <code>&lt;name&gt;</code>.</p>%1</qt>", macros);
        return true;
    }
    return false;
}
