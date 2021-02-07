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

class KateStashManager
{
public:
    KateStashManager();
    bool stash(const QList<KTextEditor::Document *> &modifieddocuments);
    void popStash(KateViewManager *viewManager);

private:
    KConfigGroup m_stashGroup;
};

#endif // KATESTASHMANAGER_H
