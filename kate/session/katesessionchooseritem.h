/* This file is part of the KDE project
 *
 *  Copyright (C) 2005 Christoph Cullmann <cullmann@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#ifndef __KATE_SESSION_CHOOSER_ITEM_H__
#define __KATE_SESSION_CHOOSER_ITEM_H__

#include <QTreeWidgetItem>

#include "katesession.h"

class KateSessionChooserItem : public QTreeWidgetItem
{
public:
    KateSessionChooserItem(QTreeWidget *tw, KateSession::Ptr s)
        : QTreeWidgetItem(tw, QStringList(s->name()))
        , session(s) {
        QString docs;
        docs.setNum(s->documents());
        setText(1, docs);
    }

    KateSession::Ptr session;
};

#endif

