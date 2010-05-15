/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2001-2010 Christoph Cullmann <cullmann@kde.org>
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

#ifndef KATE_MODEMENU_H__
#define KATE_MODEMENU_H__

#include <QtCore/QStringList>
#include <QtCore/QPointer>
#include <QtCore/QHash>

#include "katedialogs.h"
#include "katemodemanager.h"

class KateDocument;

class KateModeMenu : public KActionMenu
{
  Q_OBJECT

  public:
    KateModeMenu(const QString& text, QObject *parent)
       : KActionMenu(text, parent) { init(); }

    ~KateModeMenu();

    void updateMenu (KTextEditor::Document *doc);

  private:
    void init();

    QPointer<KateDocument> m_doc;
    QStringList subMenusName;
    QStringList names;
    QList<QMenu*> subMenus;
    QActionGroup *m_actionGroup;

  public  Q_SLOTS:
    void slotAboutToShow();

  private Q_SLOTS:
    void setType (QAction*);
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
