/* This file is part of the KDE project
   Copyright (C) 2008 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2010 Dominik Haumann <dhaumann kde org>

   In addition to the following license there is an exception, that the KDE e.V.
   may decide on other forms of relicensing.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "katecontainer.h"
#include "kateapp.h"
#include "katemainwindow.h"
#include "kateviewmanager.h"

#include "katedebug.h"

KateContainer::KateContainer(KateApp* parent)
  : QObject(parent)
  , KTextEditor::MdiContainer()
  , m_app(parent)
{
}

KateContainer::~KateContainer() {
}

//BEGIN KTextEditor::MdiContainer
void KateContainer::setActiveView(KTextEditor::View *view)
{
  if ( view && KateApp::self()->activeMainWindow()->viewManager()->viewList().contains(view) ) {
    KateApp::self()->activeMainWindow()->viewManager()->setActiveView(view);
  }
}

KTextEditor::View *KateContainer::activeView()
{
  return KateApp::self()->activeMainWindow()->viewManager()->activeView();
}

KTextEditor::Document *KateContainer::createDocument()
{
  return KateDocManager::self()->createDoc();
}

bool KateContainer::closeDocument(KTextEditor::Document *doc)
{
  if ( doc && KateDocManager::self()->findDocument(doc) ) {
    return KateDocManager::self()->closeDocument(doc);
  }
  return false;
}

KTextEditor::View *KateContainer::createView(KTextEditor::Document *doc)
{
  if ( ! doc || ! KateApp::self()->documentManager()->findDocument(doc) ) {
    return 0;
  }
  const bool success = KateApp::self()->activeMainWindow()->viewManager()->createView(doc);
  if (success) {
    return KateApp::self()->activeMainWindow()->viewManager()->activeView();
  }
  return 0;
}

bool KateContainer::closeView(KTextEditor::View *view)
{
  if ( view && KateApp::self()->activeMainWindow()->viewManager()->viewList().contains(view) ) {
    return view->close();
  }
  return false;
}
//END KTextEditor::MdiContainer


#include "katecontainer.moc"
// kate: space-indent on; indent-width 2; replace-tabs on;
