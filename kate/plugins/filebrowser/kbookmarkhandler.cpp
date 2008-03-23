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

#include <stdio.h>
#include <stdlib.h>

#include <QByteArray>

#include <kbookmarkimporter.h>
#include <kmenu.h>
#include <ksavefile.h>
#include <kstandarddirs.h>
#include <kdebug.h>

#include "katefileselector.h"

#include "kbookmarkhandler.h"
#include "kbookmarkhandler.moc"

KBookmarkHandler::KBookmarkHandler( KateFileSelector *parent, KMenu* kpopupmenu )
    : QObject( parent ),
    KBookmarkOwner(),
    mParent( parent ),
    m_menu( kpopupmenu ),
    m_importStream( 0L )
{
  setObjectName( "KBookmarkHandler" );
  if (!m_menu)
    m_menu = new KMenu( parent);

  QString file = KStandardDirs::locate( "data", "kate/fsbookmarks.xml" );
  if ( file.isEmpty() )
    file = KStandardDirs::locateLocal( "data", "kate/fsbookmarks.xml" );

  KBookmarkManager *manager = KBookmarkManager::managerForFile( file, "kate" );
  manager->setUpdate( true );

  m_bookmarkMenu = new KBookmarkMenu( manager, this, m_menu, parent->actionCollection() );
}

KBookmarkHandler::~KBookmarkHandler()
{
  delete m_bookmarkMenu;
}

QString KBookmarkHandler::currentUrl() const
{
  return mParent->dirOperator()->url().url();
}

QString KBookmarkHandler::currentTitle() const
{
  return currentUrl();
}

void KBookmarkHandler::openBookmark( const KBookmark & bm, Qt::MouseButtons, Qt::KeyboardModifiers )
{
  emit openUrl(bm.url().url());
}

void KBookmarkHandler::slotNewBookmark( const QString& /*text*/,
                                        const QByteArray& url,
                                        const QString& additionalInfo )
{
  *m_importStream << "<bookmark icon=\"" << KMimeType::iconNameForUrl( KUrl( url ) );
  *m_importStream << "\" href=\"" << QString::fromUtf8(url) << "\">\n";
  *m_importStream << "<title>" << (additionalInfo.isEmpty() ? QString::fromUtf8(url) : additionalInfo) << "</title>\n</bookmark>\n";
}

void KBookmarkHandler::slotNewFolder( const QString& text, bool /*open*/,
                                      const QString& /*additionalInfo*/ )
{
  *m_importStream << "<folder icon=\"bookmark_folder\">\n<title=\"";
  *m_importStream << text << "\">\n";
}

void KBookmarkHandler::newSeparator()
{
  *m_importStream << "<separator/>\n";
}

void KBookmarkHandler::endFolder()
{
  *m_importStream << "</folder>\n";
}

// kate: space-indent on; indent-width 2; replace-tabs on;

