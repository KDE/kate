/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include "KateTextHintManager.h"
#include <QPoint>

class QWidget;
class QString;

namespace KTextEditor
{
class View;
}

class KateTooltip
{
public:
    // tooltip hidden after timeout msec (if > 0)
    static void show(const QString &text, TextHintMarkupKind kind, QPoint pos, KTextEditor::View *v, bool manual);
};
