/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include "lspclientprotocol.h"
#include <QObject>
#include <QPointer>
#include <QString>
#include <QTimer>

#include <KTextEditor/InlineNoteProvider>
#include <KTextEditor/MovingRange>

#include <memory>
#include <unordered_map>
#include <vector>

namespace KTextEditor
{
class View;
class Document;
}

class LSPClientServerManager;

class InlayHintNoteProvider : public KTextEditor::InlineNoteProvider
{
public:
    InlayHintNoteProvider();
    void setView(KTextEditor::View *v);
    void setHints(const QList<LSPInlayHint> &hints);

    QList<int> inlineNotes(int line) const override;
    QSize inlineNoteSize(const KTextEditor::InlineNote &note) const override;
    void paintInlineNote(const KTextEditor::InlineNote &note, QPainter &painter, Qt::LayoutDirection) const override;

private:
    QColor m_noteColor;
    QColor m_noteBgColor;
    QPointer<KTextEditor::View> m_view;
    QList<LSPInlayHint> m_hints;
};

class InlayHintsManager : public QObject
{
    Q_OBJECT
public:
    explicit InlayHintsManager(const std::shared_ptr<LSPClientServerManager> &manager, QObject *parent = nullptr);
    ~InlayHintsManager();

    void setActiveView(KTextEditor::View *v);
    void disable();

private:
    void registerView(KTextEditor::View *);
    void unregisterView(KTextEditor::View *);
    void sendRequestDelayed(KTextEditor::Range, int delay = 300);
    void sendPendingRequests();
    void sendRequest(KTextEditor::Range r);

    // if doc is null, it will clear hints for all invalid docs
    void clearHintsForDoc(KTextEditor::Document *);
    struct InsertResult {
        const bool newDoc = false;
        const QVarLengthArray<int, 16> changedLines;
        const QList<LSPInlayHint> addedHints;
    };
    InsertResult insertHintsForDoc(KTextEditor::Document *doc, KTextEditor::Range requestedRange, const QList<LSPInlayHint> &newHints);

    void onTextInserted(KTextEditor::Document *doc, KTextEditor::Cursor pos, const QString &text);
    void onTextRemoved(KTextEditor::Document *doc, KTextEditor::Range range, const QString &t);
    void onWrapped(KTextEditor::Document *doc, KTextEditor::Cursor position);
    void onUnwrapped(KTextEditor::Document *doc, int line);

    struct HintData {
        QPointer<KTextEditor::Document> doc;
        QByteArray checksum;
        QList<LSPInlayHint> m_hints;
    };
    std::vector<HintData> m_hintDataByDoc;

    QTimer m_requestTimer;
    QPointer<KTextEditor::View> m_currentView;
    InlayHintNoteProvider m_noteProvider;
    std::shared_ptr<LSPClientServerManager> m_serverManager;
    QList<KTextEditor::Range> pendingRanges;
};
