/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-FileCopyrightText: 2021 Kåre Särs <kare.sars@iki.fi>

    SPDX-License-Identifier: MIT
*/
#ifndef GitBlameTooltip_h
#define GitBlameTooltip_h

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
    GitBlameTooltip(KateGitBlamePluginView *pv);
    ~GitBlameTooltip();

    void show(const QString &text, KTextEditor::View *view);

    void setIgnoreKeySequence(const QKeySequence &sequence);

private:
    class Private;
    std::unique_ptr<Private> d;
    KateGitBlamePluginView *m_pluginView;
};

#endif // GitBlameTooltip_h
