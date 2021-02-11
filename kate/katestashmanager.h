/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2021 Méven Car <meven.car@kdemail.net>

   SPDX-License-Identifier: LGPL-2.0-only
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
    bool stash(const QList<KTextEditor::Document *> &modifieddocuments);
    void popStash(KateViewManager *viewManager);

    bool stashDocument(KTextEditor::Document *doc, const QString &stashfileName, KConfigGroup &kconfig, const QString &path);
    bool popDocument(KTextEditor::Document *doc, const KConfigGroup &kconfig);
};

#endif // KATESTASHMANAGER_H
