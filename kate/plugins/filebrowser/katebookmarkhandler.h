/* This file is part of the KDE project
   Copyright (C) xxxx KFile Authors
   Copyright (C) 2002 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright (C) 2007 Mirko Stocker <me@misto.ch>
 
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

#ifndef _KBOOKMARKHANDLER_H_
#define _KBOOKMARKHANDLER_H_

#include <kbookmarkmanager.h>
#include <kbookmarkmenu.h>
#include <QTextStream>
#include <QByteArray>


class QTextStream;
class KMenu;

class KateFileSelector;

class KBookmarkHandler : public QObject, public KBookmarkOwner
{
    Q_OBJECT

  public:
    explicit KBookmarkHandler( KateFileSelector *parent, KMenu *kpopupmenu = 0 );
    ~KBookmarkHandler();

    // KBookmarkOwner interface:
    virtual QString currentUrl() const;
    virtual QString currentTitle() const;

    KMenu *menu() const
    {
      return m_menu;
    }
    virtual void openBookmark( const KBookmark &, Qt::MouseButtons, Qt::KeyboardModifiers );

  Q_SIGNALS:
    void openUrl( const QString& url );

  private Q_SLOTS:
    void slotNewBookmark( const QString& text, const QByteArray& url,
                          const QString& additionalInfo );
    void slotNewFolder( const QString& text, bool open,
                        const QString& additionalInfo );
    void newSeparator();
    void endFolder();

  private:
    KateFileSelector *mParent;
    KMenu *m_menu;
    KBookmarkMenu *m_bookmarkMenu;

    QTextStream *m_importStream;

    //class KBookmarkHandlerPrivate *d;
};

#endif // _KBOOKMARKHANDLER_H_
// kate: space-indent on; indent-width 2; replace-tabs on;

