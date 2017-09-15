/* This file is part of the KDE project
   Copyright (C) xxxx KFile Authors
   Copyright (C) 2002 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright (C) 2007 Mirko Stocker <me@misto.ch>
   Copyright (C) 2009 Dominik Haumann <dhaumann kde org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KATE_BOOKMARK_HANDLER_H
#define KATE_BOOKMARK_HANDLER_H

#include <kbookmarkmanager.h>
#include <kbookmarkmenu.h>

class KateFileBrowser;
class QMenu;

class KateBookmarkHandler : public QObject, public KBookmarkOwner
{
    Q_OBJECT

  public:
    explicit KateBookmarkHandler( KateFileBrowser *parent, QMenu *kpopupmenu = nullptr );
    ~KateBookmarkHandler() override;

    // KBookmarkOwner interface:
    QUrl currentUrl() const override;
    QString currentTitle() const override;

    QMenu *menu() const
    {
      return m_menu;
    }
    void openBookmark( const KBookmark &, Qt::MouseButtons, Qt::KeyboardModifiers ) override;

  Q_SIGNALS:
    void openUrl( const QString& url );

  private:
    KateFileBrowser *mParent;
    QMenu *m_menu;
    KBookmarkMenu *m_bookmarkMenu;
};

#endif // KATE_BOOKMARK_HANDLER_H
// kate: space-indent on; indent-width 2; replace-tabs on;

