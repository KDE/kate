/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2021 Méven Car <meven.car@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "katesession.h"
#include <KConfigGroup>

#include <QPointer>
#include <QString>

#include <span>
#include <unordered_map>

namespace KTextEditor
{
class Document;
}

class KateViewManager;

class KateStashManager
{
public:
    KateStashManager() = default;

    bool stashUnsavedChanges = false;
    bool stashNewUnsavedFiles = true;

    bool canStash() const;

    void stashDocuments(KConfig *cfg, std::span<KTextEditor::Document *const> documents);

    bool willStashDoc(KTextEditor::Document *doc) const;

    static void popDocument(KTextEditor::Document *doc, const KConfigGroup &kconfig);

    static void clearStashForSession(const KateSession::Ptr &session);

private:
    struct StashedDocument {
        QPointer<KTextEditor::Document> doc;
        QString file;
        qint64 revision = -1;
    };
    // what was stashed last, to not write unchanged documents again on every session save
    std::unordered_map<KTextEditor::Document *, StashedDocument> m_stashedDocuments;
};
