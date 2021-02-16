/*  SPDX-License-Identifier: MIT

    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: MIT
*/
#ifndef LSPTOOLTIP_H
#define LSPTOOLTIP_H

#include <QPoint>

class QWidget;
class QString;
class Tooltip;

namespace KTextEditor
{
class View;
}

class GitBlameTooltip
{
public:
    static void show(const QString &text, KTextEditor::View *v);
};

#endif // LSPTOOLTIP_H
