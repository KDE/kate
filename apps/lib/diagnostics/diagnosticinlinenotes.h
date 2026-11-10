/*
    SPDX-FileCopyrightText: 2026 Jaume Alcazo Castellarnau <jaume.alcazo@gmail.com>

    SPDX-License-Identifier: MIT
*/
#pragma once

#include "diagnostic_types.h"
#include "kateprivate_export.h"

#include <KTextEditor/InlineNoteProvider>

#include <QFontMetricsF>
#include <QPointer>
#include <QString>

#include <unordered_map>
#include <vector>

namespace KTextEditor
{
class Document;
class MovingRange;
}

/**
 * Shows the message of a diagnostic at the end of the line it starts on
 * ("error lens" style), for every view of one document.
 *
 * It does not own the ranges: they are the underline ranges DiagnosticsView
 * creates in addMarks(), so the notes follow the text while the user edits.
 * DiagnosticsView must destroy this provider before it deletes those ranges.
 */
class KATE_PRIVATE_EXPORT DiagnosticInlineNoteProvider : public KTextEditor::InlineNoteProvider
{
    Q_OBJECT
    friend class DiagnosticInlineNotesTest;

public:
    explicit DiagnosticInlineNoteProvider(KTextEditor::Document *doc);
    ~DiagnosticInlineNoteProvider() override;

    void addDiagnostic(KTextEditor::MovingRange *range, const QString &message, DiagnosticSeverity severity);

    /// call after a batch of addDiagnostic() so the views repaint once
    void notifyChanged();

    QList<int> inlineNotes(int line) const override;
    QSize inlineNoteSize(const KTextEditor::InlineNote &note) const override;
    void paintInlineNote(const KTextEditor::InlineNote &note, QPainter &painter, Qt::LayoutDirection direction) const override;

private:
    struct Entry {
        KTextEditor::MovingRange *range;
        QString message;
        DiagnosticSeverity severity;
    };

    struct LineNote {
        const Entry *entry = nullptr; // most severe entry on the line
        int count = 0; // number of entries on the line
    };

    LineNote noteForLine(int line) const;
    QString noteText(const LineNote &note, const QFontMetricsF &fm, qreal maxWidth) const;

    QPointer<KTextEditor::Document> m_doc;
    std::vector<Entry> m_entries;
    // line -> note, rebuilt lazily: the ranges move while the user edits,
    // and the views ask about every visible line several times per paint
    mutable std::unordered_map<int, LineNote> m_lineCache;
    mutable bool m_cacheDirty = true;
};
