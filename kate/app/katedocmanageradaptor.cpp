/* This file is part of the KDE project
   Copyright (C) 2003 Ian Reinhart Geiser <geiseri@kde.org>
 
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

#include "katedocmanageradaptor.h"
#include "katedocmanageradaptor.moc"

#include "katedocmanager.h"

#include <kdebug.h>

KateDocManagerAdaptor::KateDocManagerAdaptor (KateDocManager *dm)
    : QDBusAbstractAdaptor(dm ), m_dm (dm)
{
}
#ifdef __GNUC__
#warning "kde4: need to port kate lib to dbus"
#endif
#if 0
// bit more error save than the forcing c cast ;()
QDBusObjectPath KateDocManagerAdaptor::document (uint n)
{
  KTextEditor::Document *doc = m_dm->document(n);

  if (!doc)
    return QDBusObjectPath ();

  return QDBusObjectPath(doc);
}

QDBusObjectPath KateDocManagerAdaptor::activeDocument ()
{
  KTextEditor::Document *doc = m_dm->activeDocument();

  if (!doc )
    return QDBusObjectPath ();

  return QDBusObjectPath (doc);
}

QDBusObjectPath KateDocManagerAdaptor::openUrl (QString url, QString encoding)
{
  KTextEditor::Document *doc = m_dm->openUrl (url, encoding);

  if (!doc )
    return QDBusObjectPath ();

  return QDBusObjectPath (doc);
}
#endif

bool KateDocManagerAdaptor::closeDocument(uint n)
{
  return m_dm->closeDocument(n);
}

bool KateDocManagerAdaptor::closeAllDocuments()
{
  return m_dm->closeAllDocuments();
}

bool KateDocManagerAdaptor::isOpen(QString url)
{
  return m_dm->isOpen (url);
}

uint KateDocManagerAdaptor::documents ()
{
  return m_dm->documents();
}


// kate: space-indent on; indent-width 2; replace-tabs on;

