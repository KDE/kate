/*
    SPDX-FileCopyrightText: 2021 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "inlayhints.h"

#include "lspclientservermanager.h"
#include <ktexteditor_utils.h>

#include <KSyntaxHighlighting/Theme>
#include <KTextEditor/Document>
#include <KTextEditor/InlineNote>

#include <QApplication>
#include <QPainter>
#include <QSet>

static constexpr int textChangedDelay = 1000; // 1s

size_t qHash(const LSPInlayHint &s, size_t seed = 0)
{
    return qHashMulti(seed, s.position, s.label);
}

[[maybe_unused]] static bool operator==(const LSPInlayHint &l, const LSPInlayHint &r)
{
    return l.position == r.position && l.label == r.label;
}

template<typename T>
static auto binaryFind(T &&hints, int line)
{
    auto it = std::lower_bound(hints.begin(), hints.end(), line, [](const LSPInlayHint &h, int l) {
        return h.position.line() < l;
    });
    if (it != hints.end() && it->position.line() == line) {
        return it;
    }
    return hints.end();
}

static auto binaryFind(const QList<LSPInlayHint> &hints, KTextEditor::Cursor pos)
{
    auto it = std::lower_bound(hints.begin(), hints.end(), pos, [](const LSPInlayHint &h, KTextEditor::Cursor p) {
        return h.position < p;
    });
    if (it != hints.end() && it->position == pos) {
        return it;
    }
    return hints.end();
}

static void removeInvalidRanges(QList<LSPInlayHint> &hints, QList<LSPInlayHint>::iterator begin, QList<LSPInlayHint>::iterator end)
{
    hints.erase(std::remove_if(begin,
                               end,
                               [](const LSPInlayHint &h) {
                                   return !h.position.isValid();
                               }),
                end);
}

InlayHintNoteProvider::InlayHintNoteProvider()
{
}

void InlayHintNoteProvider::setView(KTextEditor::View *v)
{
    m_view = v;
    if (v) {
        m_noteColor = QColor::fromRgba(v->theme().textColor(KSyntaxHighlighting::Theme::Normal));
        m_noteBgColor = m_noteColor;
        m_noteBgColor.setAlphaF(0.1f);
        m_noteColor.setAlphaF(0.5f);
    }
    m_hints = {};
}

void InlayHintNoteProvider::setHints(const QList<LSPInlayHint> &hints)
{
    m_hints = hints;
}

QList<int> InlayHintNoteProvider::inlineNotes(int line) const
{
    QList<int> ret;
    auto it = binaryFind(m_hints, line);
    while (it != m_hints.cend() && it->position.line() == line) {
        ret.push_back(it->position.column());
        ++it;
    }
    return ret;
}

QSize InlayHintNoteProvider::inlineNoteSize(const KTextEditor::InlineNote &note) const
{
    auto it = binaryFind(m_hints, note.position());
    if (it == m_hints.end()) {
        qWarning() << Q_FUNC_INFO << note.view()->document()->documentName() << "failed to find note in m_hints, Note.position:" << note.position();
        return {};
    }

    const LSPInlayHint &hint = *it;
    const int padding = (hint.paddingLeft || hint.paddingRight) ? 4 : 0;
    if (hint.width == 0) {
        const auto font = qApp->font();
        const auto fm = QFontMetrics(font);
        const_cast<LSPInlayHint &>(hint).width = fm.horizontalAdvance(hint.label) + padding;
    }
    return {hint.width, note.lineHeight()};
}

void InlayHintNoteProvider::paintInlineNote(const KTextEditor::InlineNote &note, QPainter &painter, Qt::LayoutDirection) const
{
    auto it = binaryFind(m_hints, note.position());
    if (it != m_hints.end()) {
        const auto font = qApp->font();
        painter.setFont(font);
        QRectF r{0., 0., (qreal)it->width, (qreal)note.lineHeight()};

        // draw background rectangle
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(m_noteBgColor);
        painter.setPen(Qt::NoPen);
        auto bgRect = r;
        bgRect.setHeight(QFontMetricsF(font).height());
        bgRect.moveTop((r.height() - bgRect.height()) / 2.);
        painter.drawRoundedRect(bgRect, 3., 3.);

        // draw the hint
        painter.setPen(m_noteColor);
        if (it->paddingLeft) {
            r.adjust(4., 0., 0., 0.);
        } else if (it->paddingRight) {
            r.adjust(0., 0., -4., 0.);
        }
        painter.drawText(r, Qt::AlignLeft | Qt::AlignVCenter, it->label);
    }
}

InlayHintsManager::InlayHintsManager(const std::shared_ptr<LSPClientServerManager> &manager, QObject *parent)
    : QObject(parent)
    , m_serverManager(manager)
{
    m_requestTimer.setSingleShot(true);
    m_requestTimer.callOnTimeout(this, &InlayHintsManager::sendPendingRequests);
}

InlayHintsManager::~InlayHintsManager()
{
    unregisterView(m_currentView);
}

void InlayHintsManager::registerView(KTextEditor::View *v)
{
    using namespace KTextEditor;
    if (v) {
        // when reloading the view is same
        bool reloaded = m_currentView == v;
        m_currentView = v;
        m_currentView->registerInlineNoteProvider(&m_noteProvider);
        m_noteProvider.setView(v);
        auto d = v->document();

        connect(d, &Document::textInserted, this, &InlayHintsManager::onTextInserted, Qt::UniqueConnection);
        connect(d, &Document::textRemoved, this, &InlayHintsManager::onTextRemoved, Qt::UniqueConnection);
        connect(d, &Document::lineWrapped, this, &InlayHintsManager::onWrapped, Qt::UniqueConnection);
        connect(d, &Document::lineUnwrapped, this, &InlayHintsManager::onUnwrapped, Qt::UniqueConnection);

        auto it = std::find_if(m_hintDataByDoc.begin(), m_hintDataByDoc.end(), [doc = v->document()](const HintData &hd) {
            return hd.doc == doc;
        });

        // If the document was found and checksum hasn't changed
        if (it != m_hintDataByDoc.end() && it->checksum == d->checksum() && !it->m_hints.empty() && !reloaded) {
            m_noteProvider.setHints(it->m_hints);
            m_noteProvider.inlineNotesReset();
        } else {
            if (it != m_hintDataByDoc.end()) {
                m_hintDataByDoc.erase(it);
            }
            // clear hints from the inline note provider and reset it
            m_noteProvider.setHints({});
            m_noteProvider.inlineNotesReset();
            // Send delayed request for inlay hints
            sendRequestDelayed(v->document()->documentRange(), 1);
        }
    }

    clearHintsForDoc(nullptr);
}

void InlayHintsManager::unregisterView(KTextEditor::View *v)
{
    if (v) {
        v->disconnect(this);
        v->document()->disconnect(this);
        m_currentView->unregisterInlineNoteProvider(&m_noteProvider);
        auto it = std::find_if(m_hintDataByDoc.begin(), m_hintDataByDoc.end(), [doc = v->document()](const HintData &hd) {
            return hd.doc == doc;
        });
        // update checksum
        // we use it to check if doc was changed when restoring hints
        if (it != m_hintDataByDoc.end()) {
            it->checksum = v->document()->checksum();
        }
    }
    m_noteProvider.setView(nullptr);
}

void InlayHintsManager::setActiveView(KTextEditor::View *v)
{
    unregisterView(m_currentView);
    registerView(v);
}

void InlayHintsManager::disable()
{
    unregisterView(m_currentView);
    m_currentView.clear();
}

void InlayHintsManager::sendRequestDelayed(KTextEditor::Range r, int delay)
{
    // If its a single line range and the last range is on the same line
    if (r.onSingleLine() && !pendingRanges.isEmpty() && pendingRanges.back().onSingleLine() && pendingRanges.back().end().line() == r.start().line()) {
        pendingRanges.back() = r;
        // start column must always be zero
        pendingRanges.back().start().setColumn(0);
    } else {
        pendingRanges.append(r);
    }

    m_requestTimer.start(delay);
}

void InlayHintsManager::sendPendingRequests()
{
    if (pendingRanges.empty()) {
        return;
    }

    KTextEditor::Range rangeToRequest = pendingRanges.first();
    for (auto r : std::as_const(pendingRanges)) {
        rangeToRequest.expandToRange(r);
    }
    pendingRanges.clear();

    if (rangeToRequest.isValid()) {
        sendRequest(rangeToRequest);
    }
}

void InlayHintsManager::sendRequest(KTextEditor::Range rangeToRequest)
{
    if (!m_currentView || !m_currentView->document()) {
        return;
    }

    const auto url = m_currentView->document()->url();

    auto v = m_currentView;
    auto server = m_serverManager->findServer(v, false);
    if (server) {
        server->documentInlayHint(url, rangeToRequest, this, [v = QPointer(m_currentView), rangeToRequest, this](QList<LSPInlayHint> hints) {
            if (!v || m_currentView != v) {
                return;
            }

            // Some server e.g., dart-analyzer will just ignore the range we sent in the request
            // and send over inlay hints for the full document anyways. To avoid issues, we
            // remove all the inlay hints that fall outside the range we requested hints for.
            if (rangeToRequest.isValid()) {
                hints.erase(std::remove_if(hints.begin(),
                                           hints.end(),
                                           [rangeToRequest](const LSPInlayHint &h) {
                                               return !rangeToRequest.contains(h.position);
                                           }),
                            hints.end());
            }

            const auto result = insertHintsForDoc(v->document(), rangeToRequest, hints);
            m_noteProvider.setHints(result.addedHints);
            if (result.newDoc) {
                m_noteProvider.inlineNotesReset();
            } else {
                // qDebug() << "got hints: " << hints.size() << "changed lines: " << result.changedLines.size();
                for (const auto &line : result.changedLines) {
                    m_noteProvider.inlineNotesChanged(line);
                }
            }
        });
    }
}

void InlayHintsManager::onTextInserted(KTextEditor::Document *doc, KTextEditor::Cursor pos, const QString &text)
{
    auto it = std::find_if(m_hintDataByDoc.begin(), m_hintDataByDoc.end(), [doc](const HintData &hd) {
        return hd.doc == doc;
    });
    if (it != m_hintDataByDoc.end()) {
        auto &list = it->m_hints;
        bool changed = false;
        auto bit = binaryFind(list, pos.line());
        for (; bit != list.end(); ++bit) {
            if (bit->position.line() > pos.line()) {
                break;
            }
            if (bit->position > pos) {
                bit->position.setColumn(bit->position.column() + text.size());
                changed = true;
            }
        }
        if (changed) {
            m_noteProvider.setHints(list);
        }
    }

    KTextEditor::Range r(pos.line(), 0, pos.line(), doc->lineLength(pos.line()));
    sendRequestDelayed(r, textChangedDelay);
}

void InlayHintsManager::onTextRemoved(KTextEditor::Document *doc, KTextEditor::Range range, const QString &t)
{
    if (!range.onSingleLine()) {
        KTextEditor::Range r(range.start().line(), 0, range.end().line(), doc->lineLength(range.end().line()));
        sendRequestDelayed(r, textChangedDelay);
        return;
    }

    auto it = std::find_if(m_hintDataByDoc.begin(), m_hintDataByDoc.end(), [doc](const HintData &hd) {
        return hd.doc == doc;
    });
    if (it == m_hintDataByDoc.end()) {
        return;
    }
    auto &list = it->m_hints;
    auto bit = binaryFind(list, range.start().line());
    auto lineStartIt = bit;
    auto removeBegin = bit;
    auto removeEnd = list.end();
    bool changed = false;
    bool resort = false;
    for (; bit != list.end(); ++bit) {
        if (bit->position.line() > range.start().line()) {
            removeEnd = bit;
            break;
        }
        if (range.contains(bit->position) && range.start() < bit->position) {
            // was inside range? remove this note
            bit->position = KTextEditor::Cursor::invalid();
            changed = true;
        } else if (bit->position > range.end()) {
            // in front? => adjust position
            bit->position.setColumn(bit->position.column() - t.size());
            changed = true;
            resort = true;
        }
    }
    if (changed) {
        removeInvalidRanges(list, removeBegin, removeEnd);
        if (resort) {
            // resort the hints on this line
            std::sort(lineStartIt, removeEnd, [](const LSPInlayHint &l, const LSPInlayHint &r) {
                return l.position < r.position;
            });
        }
        m_noteProvider.setHints(list);
    }

    KTextEditor::Range r(range.start().line(), 0, range.end().line(), doc->lineLength(range.end().line()));
    sendRequestDelayed(r, textChangedDelay);
}

void InlayHintsManager::onWrapped(KTextEditor::Document *doc, KTextEditor::Cursor position)
{
    auto it = std::find_if(m_hintDataByDoc.begin(), m_hintDataByDoc.end(), [doc](const HintData &hd) {
        return hd.doc == doc;
    });
    if (it == m_hintDataByDoc.end()) {
        return;
    }
    auto &hints = it->m_hints;
    auto bit = std::lower_bound(hints.begin(), hints.end(), position.line(), [](const LSPInlayHint &h, int l) {
        return h.position.line() < l;
    });

    // Invalidate the ranges on the line that are after @p position
    auto removeBegin = bit;
    auto removeEnd = hints.end();
    bool changed = false;
    while (bit != hints.end()) {
        if (bit->position.line() > position.line()) {
            removeEnd = bit;
            break;
        }
        if (bit->position >= position) {
            // invalidate
            changed = true;
            bit->position = KTextEditor::Cursor::invalid();
        }
        ++bit;
    }
    changed = changed || bit != hints.end();
    while (bit != hints.end()) {
        bit->position.setLine(bit->position.line() + 1);
        ++bit;
    }

    // remove invalidated stuff
    if (changed) {
        removeInvalidRanges(hints, removeBegin, removeEnd);
        m_noteProvider.setHints(hints);
    }

    KTextEditor::Range r(position.line(), 0, position.line(), doc->lineLength(position.line()));
    sendRequestDelayed(r, textChangedDelay);
}

void InlayHintsManager::onUnwrapped(KTextEditor::Document *doc, int line)
{
    auto it = std::find_if(m_hintDataByDoc.begin(), m_hintDataByDoc.end(), [doc](const HintData &hd) {
        return hd.doc == doc;
    });
    if (it == m_hintDataByDoc.end()) {
        return;
    }

    auto &hints = it->m_hints;
    auto bit = std::lower_bound(hints.begin(), hints.end(), line, [](const LSPInlayHint &h, int l) {
        return h.position.line() < l;
    });

    auto removeBegin = bit;
    auto removeEnd = hints.end();
    bool changed = false;
    while (bit != hints.end()) {
        if (bit->position.line() > line) {
            removeEnd = bit;
            break;
        }
        // invalidate
        changed = true;
        bit->position = KTextEditor::Cursor::invalid();
        ++bit;
    }

    changed = changed || bit != hints.end();
    while (bit != hints.end()) {
        bit->position.setLine(bit->position.line() - 1);
        ++bit;
    }

    // remove invalidated stuff
    if (changed) {
        removeInvalidRanges(hints, removeBegin, removeEnd);
        m_noteProvider.setHints(hints);
    }

    KTextEditor::Range r(line - 1, 0, line - 1, doc->lineLength(line));
    sendRequestDelayed(r, textChangedDelay);
}

void InlayHintsManager::clearHintsForDoc(KTextEditor::Document *doc)
{
    m_hintDataByDoc.erase(std::remove_if(m_hintDataByDoc.begin(),
                                         m_hintDataByDoc.end(),
                                         [doc](const HintData &hd) {
                                             if (!doc) {
                                                 // remove all null docs and docs where checksum doesn't match
                                                 return !hd.doc || (hd.doc->checksum() != hd.checksum);
                                             }
                                             return hd.doc == doc;
                                         }),
                          m_hintDataByDoc.end());
}

InlayHintsManager::InsertResult
InlayHintsManager::insertHintsForDoc(KTextEditor::Document *doc, KTextEditor::Range requestedRange, const QList<LSPInlayHint> &newHints)
{
    auto it = std::find_if(m_hintDataByDoc.begin(), m_hintDataByDoc.end(), [doc](const HintData &hd) {
        return hd.doc == doc;
    });
    // New document
    if (it == m_hintDataByDoc.end()) {
        auto &r = m_hintDataByDoc.emplace_back();
        r = HintData{.doc = doc, .checksum = doc->checksum(), .m_hints = newHints};
        return {.newDoc = true, .changedLines = {}, .addedHints = r.m_hints};
    }
    // Old
    auto &existing = it->m_hints;
    if (newHints.isEmpty()) {
        const auto r = requestedRange;
        auto bit = std::lower_bound(existing.begin(), existing.end(), r.start().line(), [](const LSPInlayHint &h, int l) {
            return h.position.line() < l;
        });
        if (bit != existing.end()) {
            QSet<int> affectedLines;
            existing.erase(std::remove_if(bit,
                                          existing.end(),
                                          [r, &affectedLines](const LSPInlayHint &h) {
                                              bool contains = r.contains(h.position);
                                              if (contains) {
                                                  affectedLines.insert(h.position.line());
                                                  return true;
                                              }
                                              return false;
                                          }),
                           existing.end());
            return {.newDoc = false, .changedLines = {affectedLines.begin(), affectedLines.end()}, .addedHints = existing};
        }
        return {};
    }

    QSet<int> affectedLines;
    for (const auto &newHint : newHints) {
        affectedLines.insert(newHint.position.line());
    }

    QSet<LSPInlayHint> rangesToInsert(newHints.begin(), newHints.end());

    auto pred = [&affectedLines, &rangesToInsert](const LSPInlayHint &h) {
        if (affectedLines.contains(h.position.line())) {
            auto i = rangesToInsert.find(h);
            if (i != rangesToInsert.end()) {
                // if this range already exists then it doesn't need to be reinserted
                rangesToInsert.erase(i);
                return false;
            }
            return true;
        }
        return false;
    };
    // remove existing hints that contain affectedLines
    existing.erase(std::remove_if(existing.begin(), existing.end(), pred), existing.end());

    // now add new ones
    affectedLines.clear();
    for (const auto &h : rangesToInsert) {
        existing.append(h);
        // update affectedLines with lines that are actually changed
        affectedLines.insert(h.position.line());
    }

    //  and sort them
    std::sort(existing.begin(), existing.end(), [](const LSPInlayHint &l, const LSPInlayHint &r) {
        return l.position < r.position;
    });

    return {.newDoc = false, .changedLines = {affectedLines.begin(), affectedLines.end()}, .addedHints = existing};
}

#include "moc_inlayhints.cpp"
