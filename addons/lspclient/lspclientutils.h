/*
    SPDX-FileCopyrightText: 2022 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>

    SPDX-License-Identifier: MIT
*/

#ifndef LSPCLIENTUTILS_H
#define LSPCLIENTUTILS_H

#include "lspclientservermanager.h"

LSPRange transformRange(const QUrl &url, const LSPClientRevisionSnapshot &snapshot, const LSPRange &range);

void applyEdits(KTextEditor::Document *doc, const LSPClientRevisionSnapshot *snapshot, const QList<LSPTextEdit> &edits);

#endif
