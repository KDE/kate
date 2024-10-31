/*
    SPDX-FileCopyrightText: 2024 Karthik Nishanth <kndevl@outlook.com>
    SPDX-FileCopyrightText: 2023 Waqar Ahmed <waqar.17a@gmail.com
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
    SPDX-FileCopyrightText: 2019 Christoph Cullmann <cullmann@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include "hintstate.h"

class QTextEdit;
class KateTextHintManager;

class KateTextHintView : public QObject
{
public:
    explicit KateTextHintView(KTextEditor::MainWindow *mainWindow, KateTextHintManager *parent);
    void setView(KTextEditor::View *view);
    void update(size_t instanceId, const QString &text, TextHintMarkupKind kind, KTextEditor::View *view);

private:
    void render();

    QWidget *m_pane;
    QTextEdit *m_edit;
    HintState m_state;
    KTextEditor::View *m_view;

    // To destroy the connections in a deterministic order when ~KateTextHintView() is called
    QMetaObject::Connection m_viewTracker;
    QMetaObject::Connection m_cursorTracker;
};
