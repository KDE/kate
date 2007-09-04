/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
 
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

#include "documentmanager.h"
#include "documentmanager.moc"

#include "plugin.h"
#include "pluginmanager.h"

#include "application.h"

#include "../app/katedocmanager.h"

namespace Kate
{

  class PrivateDocumentManager
  {
    public:
      PrivateDocumentManager ()
      {}

      ~PrivateDocumentManager ()
      {}

      KateDocManager *docMan;
  };

  DocumentManager::DocumentManager (void *documentManager) : QObject ((KateDocManager*) documentManager)
  {
    d = new PrivateDocumentManager ();
    d->docMan = (KateDocManager*) documentManager;
  }

  DocumentManager::~DocumentManager ()
  {
    delete d;
  }

  const QList<KTextEditor::Document*> &DocumentManager::documents () const
  {
    return d->docMan->documentList ();
  }

  KTextEditor::Document *DocumentManager::findUrl (const KUrl &url) const
  {
    return d->docMan->findDocument (url);
  }

  KTextEditor::Document *DocumentManager::openUrl(const KUrl&url, const QString &encoding)
  {
    return d->docMan->openUrl (url, encoding);
  }

  bool DocumentManager::closeDocument(KTextEditor::Document *document)
  {
    return d->docMan->closeDocument (document);
  }

  DocumentManager *documentManager ()
  {
    return application()->documentManager ();
  }

}

// kate: space-indent on; indent-width 2; replace-tabs on;

