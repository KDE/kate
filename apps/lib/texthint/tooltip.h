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
    static QObject *
    show(size_t instanceId, const QString &text, TextHintMarkupKind kind, QPoint pos, KTextEditor::View *v, bool manual, KTextEditor::Range hoveredRange);
};
