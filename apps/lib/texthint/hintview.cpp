/*
    SPDX-FileCopyrightText: 2024 Karthik Nishanth <kndevl@outlook.com>
    SPDX-FileCopyrightText: 2023 Waqar Ahmed <waqar.17a@gmail.com
    SPDX-FileCopyrightText: 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
    SPDX-FileCopyrightText: 2019 Christoph Cullmann <cullmann@kde.org>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "hintview.h"
#include "KateTextHintManager.h"

#include <QKeyEvent>
#include <QTextEdit>

#include <KLocalizedString>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>
#include <kateapp.h>

KateTextHintView::KateTextHintView(KTextEditor::MainWindow *mainWindow, KateTextHintManager *parent)
    : QObject(parent)
    , m_pane(mainWindow->createToolView(nullptr,
                                        QStringLiteral("hint_view"),
                                        KTextEditor::MainWindow::Bottom,
                                        QIcon::fromTheme(QStringLiteral("autocorrection")),
                                        i18n("Context")))
    , m_edit(new QTextEdit(m_pane))
    , m_view(nullptr)
{
    m_edit->setReadOnly(true);
    m_viewTracker = connect(mainWindow, &KTextEditor::MainWindow::viewChanged, this, &KateTextHintView::setView);
    connect(this, &QObject::destroyed, m_pane, &QObject::deleteLater);
    connect(mainWindow, &KTextEditor::MainWindow::unhandledShortcutOverride, this, [mainWindow, this](QEvent *event) {
        const auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (m_pane->isVisible() && keyEvent->key() == Qt::Key_Escape && keyEvent->modifiers() == Qt::NoModifier) {
            mainWindow->hideToolView(m_pane);
        }
    });
    setView(mainWindow->activeView());
}

void KateTextHintView::setView(KTextEditor::View *view)
{
    if (m_view != view) {
        // Clear the view
        m_state.clear();
        render();

        m_view = view;

        // At a given point in time, we want a single connection between KTextEditor::View and KateTextHintView.
        // Disconnect the previous connection before creating another one.
        disconnect(m_cursorTracker);

        if (m_view) {
            m_cursorTracker = connect(m_view, &KTextEditor::View::cursorPositionChanged, this, [this](KTextEditor::View *v, KTextEditor::Cursor c) {
                // Request hint
                qobject_cast<KateTextHintManager *>(parent())->ontextHintRequested(v, c, KateTextHintManager::Requestor::CursorChange);
            });
        }
    }
}

void KateTextHintView::update(size_t instanceId, const QString &text, TextHintMarkupKind kind, KTextEditor::View *view)
{
    if (text.isEmpty() || text.trimmed().isEmpty() || m_view != view) {
        m_state.remove(instanceId);
    } else {
        m_state.upsert(instanceId, text, kind);
    }

    render();
}

void KateTextHintView::render()
{
    m_state.render([this](const HintState::Hint &contents) {
        const auto &[text, kind] = contents;
        if (kind == TextHintMarkupKind::PlainText) {
            m_edit->setPlainText(text);
        } else {
            m_edit->setMarkdown(text);
        }
    });
}
