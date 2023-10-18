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
        auto range = snapshot ? transformRange(doc->url(), *snapshot, edit.range) : edit.range;
        KTextEditor::MovingRange *mr = doc->newMovingRange(range);
        ranges.append(mr);
    }

    // now make one transaction (a.o. for one undo) and apply in sequence
    if (!ranges.empty()) {
        KTextEditor::Document::EditingTransaction transaction(doc);
        for (int i = 0; i < ranges.length(); ++i) {
            doc->replaceText(ranges.at(i)->toRange(), edits.at(i).newText);
        }
    }

    qDeleteAll(ranges);
}
