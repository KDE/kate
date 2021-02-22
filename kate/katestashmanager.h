/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2021 Méven Car <meven.car@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATESTASHMANAGER_H
#define KATESTASHMANAGER_H

#include "kconfiggroup.h"

namespace KTextEditor
{
class Document;
}

class KateViewManager;

class KateStashManager : QObject
{
    Q_OBJECT
public:
    KateStashManager(QObject *parent = nullptr);

    int stashUnsaveChanges()
    {
        return m_stashUnsaveChanges;
    }

    void setStashUnsaveChanges(int stashUnsaveChanges)
    {
        m_stashUnsaveChanges = stashUnsaveChanges;
    }

    bool willStashDoc(KTextEditor::Document *doc);

    void stashDocument(KTextEditor::Document *doc, const QString &stashfileName, KConfigGroup &kconfig, const QString &path);
    bool popDocument(KTextEditor::Document *doc, const KConfigGroup &kconfig);

private:
    /**
     * Stash unsave changes setting
     *
     * stash unsaved file by default
     *
     * 0 => Never
     * 1 => for unsaved files
     * 2 => for all files
     */
    int m_stashUnsaveChanges = 1;
};

#endif // KATESTASHMANAGER_H
