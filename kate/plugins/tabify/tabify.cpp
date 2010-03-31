/***************************************************************************
*   Copyright (C) 2010 by Abhishek Patil <abhishekworld@gmail.com>        *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
***************************************************************************/
#include "tabify.h"
#include <kate/documentmanager.h>
#include <kate/application.h>

#include <kaction.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kglobalsettings.h>
#include <kglobal.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <kaboutdata.h>

#include <kiconloader.h>
#include <ktabbar.h>
#include <kdebug.h>
#include <QtGui/QBoxLayout>
#include <QtCore/QMutex>

K_PLUGIN_FACTORY(TabBarFactory, registerPlugin<TabBarPlugin>();)
K_EXPORT_PLUGIN(TabBarFactory(KAboutData("tabifyplugin", "katetabifyplugin",
                              ki18n("TabifyPlugin"), "0.1", ki18n("Tabify Plugin"), KAboutData::License_LGPL_V2)))

///////////////////////////////////////////////////////////////////////////////
// TabBarPluginView
///////////////////////////////////////////////////////////////////////////////
TabBarPluginView::TabBarPluginView(Kate::MainWindow* mainwindow)
    : Kate::PluginView(mainwindow)
{
  m_tabBar = new KTabBar(mainWindow()->centralWidget());
  m_tabIsDeleting = false;

  m_tabBar->setTabsClosable(true);
  m_tabBar->setDocumentMode(true);

  QBoxLayout* layout = qobject_cast<QBoxLayout*>(mainWindow()->centralWidget()->layout());
  layout->insertWidget(0, m_tabBar);

  connect(Kate::application()->documentManager(), SIGNAL(documentCreated(KTextEditor::Document*)),
          this, SLOT(slotDocumentCreated(KTextEditor::Document*)));
  connect(Kate::application()->documentManager(), SIGNAL(documentDeleted(KTextEditor::Document*)),
          this, SLOT(slotDocumentDeleted(KTextEditor::Document*)));
  connect(mainWindow(), SIGNAL(viewChanged()),
          this, SLOT(slotViewChanged()));

  connect(m_tabBar, SIGNAL(currentChanged(int)), this, SLOT(slotTabChanged(int)));
  connect(m_tabBar, SIGNAL(closeRequest(int)), this, SLOT(slotTabCloseRequest(int)));
  connect(m_tabBar, SIGNAL(mouseMiddleClick(int)), this, SLOT(slotTabCloseRequest(int)));

  foreach(KTextEditor::Document* document, Kate::application()->documentManager()->documents()) {
    slotDocumentCreated(document);
  }
}

TabBarPluginView::~TabBarPluginView()
{
  delete m_tabBar;
}

void TabBarPluginView::slotDocumentCreated(KTextEditor::Document* document)
{
  if (!document)
    return;
  connect(document, SIGNAL(modifiedChanged(KTextEditor::Document*)),
          this, SLOT(slotDocumentChanged(KTextEditor::Document*)));
  connect(document, SIGNAL(modifiedOnDisk(KTextEditor::Document*, bool,
                                          KTextEditor::ModificationInterface::ModifiedOnDiskReason)),
          this, SLOT(slotModifiedOnDisc(KTextEditor::Document*, bool,
                                        KTextEditor::ModificationInterface::ModifiedOnDiskReason)));
  connect(document, SIGNAL(documentNameChanged(KTextEditor::Document*)),
          this, SLOT(slotNameChanged(KTextEditor::Document*)));

  int index = m_tabBar->addTab(document->documentName());
  m_tabBar->setTabToolTip(index, document->url().prettyUrl());
  m_tabDocMap[index] = document;
  m_docTabMap[document] = index;
}

void TabBarPluginView::slotTabChanged(int index)
{
  if (m_tabIsDeleting) {
    return;
  }

  mainWindow()->activateView(m_tabDocMap[index]);
}

void TabBarPluginView::slotDocumentDeleted(KTextEditor::Document* document)
{
  m_tabIsDeleting = true;
  int index = m_docTabMap[document];
  m_tabBar->removeTab(index);
  m_tabDocMap.clear();
  m_docTabMap.clear();
  m_tabIsDeleting = false;

  //Rebuild the Map with current tab index...
  QList<KTextEditor::Document*>documentList =  Kate::application()->documentManager()->documents();
  for (int i = 0; i <  documentList.count(); i++) {
    m_tabBar->setTabToolTip(i, documentList.at(i)->url().prettyUrl());
    m_tabDocMap[i] = documentList.at(i);
    m_docTabMap[documentList.at(i)] = i;
  }
}

void TabBarPluginView::slotViewChanged()
{
  if (m_tabIsDeleting) {
    return;
  }

  KTextEditor::View* view = mainWindow()->activeView();
  if (!view) {
    return;
  }

  int tabID = m_docTabMap[view->document()];
  m_tabBar->setCurrentIndex(tabID);
}

void TabBarPluginView::slotTabCloseRequest(int tabId)
{
  Kate::application()->documentManager()->closeDocument(m_tabDocMap[tabId]);
}

void TabBarPluginView::slotDocumentChanged(KTextEditor::Document* document)
{

  int tabID = m_docTabMap[document];
  if (document->isModified()) {
    m_tabBar->setTabIcon(tabID, KIconLoader::global()
                         ->loadIcon("modified", KIconLoader::Small, 16));
  } else {
    m_tabBar->setTabIcon(tabID, QIcon());
  }
}

void TabBarPluginView::slotModifiedOnDisc(KTextEditor::Document* document, bool modified,
    KTextEditor::ModificationInterface::ModifiedOnDiskReason reason)
{
  int tabID = m_docTabMap[document];

  if (!modified) {
    m_tabBar->setTabIcon(tabID, QIcon());
  } else {
    switch (reason) {
    case KTextEditor::ModificationInterface::OnDiskModified:
      m_tabBar->setTabIcon(tabID, KIconLoader::global()
                           ->loadIcon("cancel", KIconLoader::Small));
      break;
    case KTextEditor::ModificationInterface::OnDiskCreated:
      m_tabBar->setTabIcon(tabID, KIconLoader::global()
                           ->loadIcon("modified", KIconLoader::Small));
      break;
    case KTextEditor::ModificationInterface::OnDiskDeleted:
      m_tabBar->setTabIcon(tabID, KIconLoader::global()
                           ->loadIcon("cancel", KIconLoader::Small));
    default:
      m_tabBar->setTabIcon(tabID, KIconLoader::global()
                           ->loadIcon("cancel", KIconLoader::Small));
    }
  }
}

void TabBarPluginView::slotNameChanged(KTextEditor::Document* document)
{

  if (!document) {
    return;
  }

  int tabID = m_docTabMap[document];
  m_tabBar->setTabText(tabID, document->documentName());
  m_tabBar->setTabToolTip(tabID, document->url().prettyUrl());
}

///////////////////////////////////////////////////////////////////////////////
// TabBarPlugin
///////////////////////////////////////////////////////////////////////////////
TabBarPlugin::TabBarPlugin(QObject* parent , const QList<QVariant>&)
    : Kate::Plugin((Kate::Application*)parent)
{
}

TabBarPlugin::~TabBarPlugin()
{
}

Kate::PluginView *TabBarPlugin::createView(Kate::MainWindow *mainWindow)
{
  TabBarPluginView *view = new TabBarPluginView(mainWindow);
  return view;
}

