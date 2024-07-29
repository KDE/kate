/*
 *    SPDX-FileCopyrightText: 2022 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
 *
 *    SPDX-License-Identifier: MIT
 */

#include "lspclientutils.h"

#include <KTextEditor/Document>

LSPRange transformRange(const QUrl &url, const LSPClientRevisionSnapshot &snapshot, const LSPRange &range)
{
    KTextEditor::Document *doc;
    qint64 revision;

    auto result = range;
    snapshot.find(url, doc, revision);
    if (doc) {
        doc->transformRange(result, KTextEditor::MovingRange::DoNotExpand, KTextEditor::MovingRange::AllowEmpty, revision);
    }
    return result;
}

void applyEdits(KTextEditor::Document *doc, const LSPClientRevisionSnapshot *snapshot, const QList<LSPTextEdit> &edits)
{
    // NOTE:
    // server might be pretty sloppy wrt edits (e.g. python-language-server)
    // e.g. send one edit for the whole document rather than 'surgical edits'
    // and that even when requesting format for a limited selection
    // ... but then we are but a client and do as we are told
    // all-in-all a low priority feature

    // all coordinates in edits are wrt original document,
    // so create moving ranges that will adjust to preceding edits as they are applied
    QList<KTextEditor::MovingRange *> ranges;
    for (const auto &edit : edits) {
        KTextEditor::Range editRange = edit.range;
        // Some servers use values like INT_MAX to say they want to apply an edit to
        // the whole document. Snap the range to document end for such cases. This is
        // what VSCode does as well.
        if (edit.range.isValid() && edit.range.end() > doc->documentEnd()) {
            editRange.setEnd(doc->documentEnd());
        }

        auto range = snapshot ? transformRange(doc->url(), *snapshot, editRange) : editRange;
        KTextEditor::MovingRange *mr = doc->newMovingRange(range);
        ranges.append(mr);
    }

    // now make one transaction (a.o. for one undo) and apply in sequence
    if (!ranges.empty()) {
        KTextEditor::Document::EditingTransaction transaction(doc);
        for (int i = 0; i < ranges.length(); ++i) {
            auto range = ranges.at(i)->toRange();
            if (range.isValid()) {
                doc->replaceText(range, edits.at(i).newText);
            }
        }
    }

    qDeleteAll(ranges);
}
