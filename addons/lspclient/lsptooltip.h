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

class LspTooltip
{
public:
    // tooltip hidden after timeout msec (if > 0)
    static void show(const QString &text, QPoint pos, KTextEditor::View *v, int timeout);
};

#endif // LSPTOOLTIP_H
