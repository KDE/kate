/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-FileCopyrightText: 2021 Kåre Särs <kare.sars@iki.fi>

    SPDX-License-Identifier: MIT
*/
#pragma once

#include <memory>

class QString;
class KateGitBlamePluginView;
class QKeySequence;

namespace KTextEditor
{
class View;
}

class GitBlameTooltipPrivate;

class GitBlameTooltip
{
public:
    explicit GitBlameTooltip(KateGitBlamePluginView *pv);
    ~GitBlameTooltip();

    void show(const QString &text, KTextEditor::View *view);
    void hide();

    void setIgnoreKeySequence(const QKeySequence &sequence);

private:
    std::unique_ptr<GitBlameTooltipPrivate> d;
    KateGitBlamePluginView *m_pluginView;
};
