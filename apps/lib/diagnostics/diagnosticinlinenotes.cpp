/*
    SPDX-FileCopyrightText: 2026 Jaume Alcazo Castellarnau <jaume.alcazo@gmail.com>

    SPDX-License-Identifier: MIT
*/
#include "diagnosticinlinenotes.h"

#include <KSyntaxHighlighting/Theme>
#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/InlineNote>
#include <KTextEditor/MovingRange>
#include <KTextEditor/View>

#include <QFontMetricsF>
#include <QPaintDevice>
#include <QPainter>

#include <algorithm>

namespace
{
// gap between the end of the code and the note, in average character widths
constexpr qreal gapChars = 4.;
// longest note, in average character widths; longer messages are elided
constexpr qreal maxChars = 100.;
// when the line already reaches the edge of the view, still show this much
constexpr qreal minChars = 20.;
// horizontal padding inside the background
constexpr qreal padding = 4.;

// lower is more severe; Unknown sorts last
int severityRank(DiagnosticSeverity s)
{
    return s == DiagnosticSeverity::Unknown ? 100 : static_cast<int>(s);
}

QColor severityColor(DiagnosticSeverity s)
{
    using Style = KSyntaxHighlighting::Theme::TextStyle;
    const auto theme = KTextEditor::Editor::instance()->theme();
    switch (s) {
    case DiagnosticSeverity::Error:
        return QColor::fromRgba(theme.textColor(Style::Error));
    case DiagnosticSeverity::Warning:
        return QColor::fromRgba(theme.textColor(Style::Warning));
    default:
        return QColor::fromRgba(theme.textColor(Style::Information));
    }
}

// italic, so the message reads as an annotation rather than code
QFont noteFont(const KTextEditor::InlineNote &note)
{
    QFont font = note.font();
    font.setItalic(true);
    return font;
}
}

DiagnosticInlineNoteProvider::DiagnosticInlineNoteProvider(KTextEditor::Document *doc)
    : m_doc(doc)
{
    const auto views = m_doc->views();
    for (auto view : views) {
        view->registerInlineNoteProvider(this);
    }
    connect(m_doc, &KTextEditor::Document::viewCreated, this, [this](KTextEditor::Document *, KTextEditor::View *view) {
        view->registerInlineNoteProvider(this);
    });
    connect(m_doc, &KTextEditor::Document::textChanged, this, [this] {
        m_cacheDirty = true;
    });
}

DiagnosticInlineNoteProvider::~DiagnosticInlineNoteProvider()
{
    if (m_doc) {
        const auto views = m_doc->views();
        for (auto view : views) {
            view->unregisterInlineNoteProvider(this);
        }
    }
}

void DiagnosticInlineNoteProvider::addDiagnostic(KTextEditor::MovingRange *range, const QString &message, DiagnosticSeverity severity)
{
    // only the first line: embedded newlines do not render inline
    const auto firstLine = message.section(u'\n', 0, 0, QString::SectionSkipEmpty).trimmed();
    if (!range || firstLine.isEmpty()) {
        return;
    }
    m_entries.push_back({range, firstLine, severity});
    m_cacheDirty = true;
}

void DiagnosticInlineNoteProvider::notifyChanged()
{
    Q_EMIT inlineNotesReset();
}

DiagnosticInlineNoteProvider::LineNote DiagnosticInlineNoteProvider::noteForLine(int line) const
{
    if (m_cacheDirty) {
        m_lineCache.clear();
        for (const auto &entry : m_entries) {
            const auto start = entry.range->start().toCursor();
            if (!start.isValid()) {
                continue;
            }
            auto &note = m_lineCache[start.line()];
            ++note.count;
            if (!note.entry || severityRank(entry.severity) < severityRank(note.entry->severity)) {
                note.entry = &entry;
            }
        }
        m_cacheDirty = false;
    }
    const auto it = m_lineCache.find(line);
    return it != m_lineCache.end() ? it->second : LineNote{};
}

QString DiagnosticInlineNoteProvider::noteText(const LineNote &note, const QFontMetricsF &fm, qreal maxWidth) const
{
    // elide only the message, the "(+N)" count must stay visible
    const QString suffix = note.count > 1 ? QStringLiteral(" (+%1)").arg(note.count - 1) : QString();
    const qreal messageWidth = std::max(maxWidth - fm.horizontalAdvance(suffix), minChars * fm.averageCharWidth());
    return fm.elidedText(note.entry->message, Qt::ElideRight, messageWidth) + suffix;
}

QList<int> DiagnosticInlineNoteProvider::inlineNotes(int line) const
{
    if (!m_doc || !noteForLine(line).entry) {
        return {};
    }
    return {m_doc->lineLength(line)};
}

QSize DiagnosticInlineNoteProvider::inlineNoteSize(const KTextEditor::InlineNote &note) const
{
    const auto lineNote = noteForLine(note.position().line());
    if (!lineNote.entry) {
        return {};
    }
    const QFontMetricsF fm(noteFont(note));
    const qreal charWidth = fm.averageCharWidth();
    const auto text = noteText(lineNote, fm, maxChars * charWidth);
    const qreal width = gapChars * charWidth + fm.horizontalAdvance(text) + 2 * padding;
    return QSize(qCeil(width), note.lineHeight());
}

void DiagnosticInlineNoteProvider::paintInlineNote(const KTextEditor::InlineNote &note, QPainter &painter, Qt::LayoutDirection) const
{
    const auto lineNote = noteForLine(note.position().line());
    if (!lineNote.entry) {
        return;
    }
    const auto font = noteFont(note);
    const QFontMetricsF fm(font);
    const qreal charWidth = fm.averageCharWidth();
    // the reserved size is computed without knowing where the line ends on screen;
    // here we do: the painter is translated to the note, so shrink to the visible area
    qreal maxWidth = maxChars * charWidth;
    if (const auto device = painter.device()) {
        const qreal visible = device->width() - painter.transform().dx() - gapChars * charWidth - 2 * padding;
        maxWidth = std::min(maxWidth, visible);
    }
    const auto text = noteText(lineNote, fm, maxWidth);
    const auto color = severityColor(lineNote.entry->severity);

    painter.setFont(font);
    painter.setRenderHint(QPainter::Antialiasing);

    // faint background, Error Lens style
    QRectF bg(gapChars * charWidth, 0., fm.horizontalAdvance(text) + 2 * padding, fm.height());
    bg.moveTop((note.lineHeight() - bg.height()) / 2.);
    QColor bgColor = color;
    bgColor.setAlphaF(0.12f);
    painter.setPen(Qt::NoPen);
    painter.setBrush(bgColor);
    painter.drawRoundedRect(bg, 3., 3.);

    painter.setPen(color);
    painter.drawText(bg.adjusted(padding, 0., -padding, 0.), Qt::AlignLeft | Qt::AlignVCenter, text);
}
