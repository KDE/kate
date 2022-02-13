/*
 *    SPDX-FileCopyrightText: 2022 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>
 *
 *    SPDX-License-Identifier: MIT
 */

#include "lspclientutils.h"

#include <KTextEditor/MovingInterface>

LSPRange transformRange(const QUrl &url, const LSPClientRevisionSnapshot &snapshot, const LSPRange &range)
{
    KTextEditor::MovingInterface *miface;
    qint64 revision;

    auto result = range;
    snapshot.find(url, miface, revision);
    if (miface) {
        miface->transformRange(result, KTextEditor::MovingRange::DoNotExpand, KTextEditor::MovingRange::AllowEmpty, revision);
    }
    return result;
}

void applyEdits(KTextEditor::Document *doc, const LSPClientRevisionSnapshot *snapshot, const QList<LSPTextEdit> &edits)
{
    KTextEditor::MovingInterface *miface = qobject_cast<KTextEditor::MovingInterface *>(doc);
    Q_ASSERT(miface);

    // NOTE:
    // server might be pretty sloppy wrt edits (e.g. python-language-server)
    // e.g. send one edit for the whole document rather than 'surgical edits'
    // and that even when requesting format for a limited selection
    // ... but then we are but a client and do as we are told
    // all-in-all a low priority feature

    // all coordinates in edits are wrt original document,
    // so create moving ranges that will adjust to preceding edits as they are applied
    QVector<KTextEditor::MovingRange *> ranges;
    for (const auto &edit : edits) {
        auto range = snapshot ? transformRange(doc->url(), *snapshot, edit.range) : edit.range;
        KTextEditor::MovingRange *mr = miface->newMovingRange(range);
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
