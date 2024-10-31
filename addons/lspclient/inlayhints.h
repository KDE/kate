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
#include <vector>

namespace KTextEditor
{
class View;
class Document;
}

class LSPClientServerManager;
class InlayHintsManager;

class InlayHintNoteProvider : public KTextEditor::InlineNoteProvider
{
public:
    InlayHintNoteProvider(InlayHintsManager *mgr);
    void viewChanged(KTextEditor::View *v);

    const std::vector<LSPInlayHint> &hints() const;
    QList<int> inlineNotes(int line) const override;
    QSize inlineNoteSize(const KTextEditor::InlineNote &note) const override;
    void paintInlineNote(const KTextEditor::InlineNote &note, QPainter &painter, Qt::LayoutDirection) const override;

private:
    QColor m_noteColor;
    QColor m_noteBgColor;
    InlayHintsManager *const m_mgr;
};

class InlayHintsManager : public QObject
{
public:
    explicit InlayHintsManager(const std::shared_ptr<LSPClientServerManager> &manager, QObject *parent = nullptr);
    ~InlayHintsManager();

    void setActiveView(KTextEditor::View *v);
    void disable();

    const std::vector<LSPInlayHint> &hintsForActiveView();

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
        const std::vector<LSPInlayHint> addedHints;
    };
    InsertResult insertHintsForDoc(KTextEditor::Document *doc, KTextEditor::Range requestedRange, const std::vector<LSPInlayHint> &newHints);

    void onTextInserted(KTextEditor::Document *doc, KTextEditor::Cursor pos, const QString &text);
    void onTextRemoved(KTextEditor::Document *doc, KTextEditor::Range range, const QString &t);
    void onWrapped(KTextEditor::Document *doc, KTextEditor::Cursor position);
    void onUnwrapped(KTextEditor::Document *doc, int line);

    struct HintData {
        QPointer<KTextEditor::Document> doc;
        QByteArray checksum;
        std::vector<LSPInlayHint> m_hints;
    };
    std::vector<HintData> m_hintDataByDoc;

    QTimer m_requestTimer;
    QPointer<KTextEditor::View> m_currentView;
    InlayHintNoteProvider m_noteProvider;
    std::shared_ptr<LSPClientServerManager> m_serverManager;
    QList<KTextEditor::Range> pendingRanges;
    const std::vector<LSPInlayHint> m_emptyHintsArray;
};
