/* This file is part of the KDE project
   Copyright (C) xxxx KFile Authors
   Copyright (C) 2002 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright (C) 2009 Dominik Haumann <dhaumann kde org>
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

#include "katebookmarkhandler.h"
#include "katefilebrowser.h"

#include <kdiroperator.h>

#include <QStandardPaths>
#include <QMenu>

KateBookmarkHandler::KateBookmarkHandler( KateFileBrowser *parent, QMenu* kpopupmenu )
  : QObject( parent ),
  KBookmarkOwner(),
  mParent( parent ),
  m_menu( kpopupmenu )
{
  setObjectName( QStringLiteral ("KateBookmarkHandler") );
  if (!m_menu)
    m_menu = new QMenu( parent);

  QString file = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("kate/fsbookmarks.xml"));
  if ( file.isEmpty() )
    file = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QStringLiteral("/kate/fsbookmarks.xml");

  KBookmarkManager *manager = KBookmarkManager::managerForFile( file, QStringLiteral("kate") );
  manager->setUpdate( true );

  m_bookmarkMenu = new KBookmarkMenu( manager, this, m_menu, parent->actionCollection() );
}

KateBookmarkHandler::~KateBookmarkHandler()
{
  delete m_bookmarkMenu;
}

QUrl KateBookmarkHandler::currentUrl() const
{
  return mParent->dirOperator()->url();
}

QString KateBookmarkHandler::currentTitle() const
{
  return currentUrl().url();
}

void KateBookmarkHandler::openBookmark( const KBookmark & bm, Qt::MouseButtons, Qt::KeyboardModifiers )
{
  emit openUrl(bm.url().url());
}

// kate: space-indent on; indent-width 2; replace-tabs on;
