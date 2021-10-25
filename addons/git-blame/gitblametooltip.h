/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-FileCopyrightText: 2021 Kåre Särs <kare.sars@iki.fi>

    SPDX-License-Identifier: MIT
*/
#ifndef GitBlameTooltip_h
#define GitBlameTooltip_h

#include <QPointer>

class QString;
class KateGitBlamePluginView;

namespace KTextEditor
{
class View;
}

class GitBlameTooltip
{
public:
    GitBlameTooltip(KateGitBlamePluginView *pv);
    ~GitBlameTooltip();

    void show(const QString &text, QPointer<KTextEditor::View> view);

    void setIgnoreKeySequence(QKeySequence sequence);

private:
    class Private;
    Private *const d;
};

#endif // GitBlameTooltip_h
