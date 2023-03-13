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

class GitBlameTooltip
{
public:
    explicit GitBlameTooltip(KateGitBlamePluginView *pv);
    ~GitBlameTooltip();

    void show(const QString &text, KTextEditor::View *view);

    void setIgnoreKeySequence(const QKeySequence &sequence);

private:
    class Private;
    std::unique_ptr<Private> d;
    KateGitBlamePluginView *m_pluginView;
};
