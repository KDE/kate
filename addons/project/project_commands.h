/*
    SPDX-FileCopyrightText: 2024 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include <KTextEditor/Command>
#include <QString>

namespace KTextEditor
{
class View;
class Range;
}

class ProjectPluginCommands : public KTextEditor::Command
{
    Q_OBJECT
public:
    ProjectPluginCommands(QObject *parent);

public:
    bool exec(KTextEditor::View *view, const QString &cmd, QString &msg, const KTextEditor::Range &range = KTextEditor::Range::invalid()) override;
    bool help(KTextEditor::View *view, const QString &cmd, QString &msg) override;

private:
    void error(const QString &error);
};
