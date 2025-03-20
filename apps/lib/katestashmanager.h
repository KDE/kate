/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2021 Méven Car <meven.car@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "katesession.h"
#include "kconfiggroup.h"

#include <span>

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

    void stashDocuments(KConfig *cfg, std::span<KTextEditor::Document *const> documents) const;

    bool willStashDoc(KTextEditor::Document *doc) const;

    void stashDocument(KTextEditor::Document *doc, const QString &stashfileName, KConfigGroup &kconfig, const QString &path) const;
    static void popDocument(KTextEditor::Document *doc, const KConfigGroup &kconfig);

    static void clearStashForSession(const KateSession::Ptr &session);
};
