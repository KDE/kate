/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2002 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright (C) 2007-2008 Dominik Haumann <dhaumann kde org>

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

#include "katefindinfiles.h"
#include "katefindinfiles.moc"

#include "katefindoptions.h"
#include "kateresultview.h"

#include <kparts/part.h>
#include <kaction.h>
#include <kactioncollection.h>

#include <kconfig.h>
#include <kdebug.h>
#include <kgenericfactory.h>
#include <klibloader.h>
#include <klocale.h>

K_EXPORT_COMPONENT_FACTORY( katefindinfilesplugin, KGenericFactory<KateFindInFilesPlugin>( "katefindinfilesplugin" ) )

KateFindInFilesPlugin::KateFindInFilesPlugin( QObject* parent, const QStringList& ):
    Kate::Plugin ( (Kate::Application*)parent )
{
}

Kate::PluginView *KateFindInFilesPlugin::createView (Kate::MainWindow *mainWindow)
{
  return new KateFindInFilesView (mainWindow);
}

void KateFindInFilesPlugin::readSessionConfig (KConfigBase* config, const QString& groupPrefix)
{
  KateFindInFilesOptions::self().load(KConfigGroup(config, groupPrefix + ":find-in-files"));
}

void KateFindInFilesPlugin::writeSessionConfig (KConfigBase* config, const QString& groupPrefix)
{
  KConfigGroup cg(config, groupPrefix + ":find-in-files");
  KateFindInFilesOptions::self().save(cg);
}


KateFindInFilesView::KateFindInFilesView (Kate::MainWindow *mw)
  : Kate::PluginView (mw)
  , KXMLGUIClient()
  , m_mw(mw)
  , m_findDialog(0)
{
  // this must be called before putting anything into actionCollection()
  setComponentData(KComponentData("kate"));

  QAction* a = actionCollection()->addAction("findinfiles_edit_find_in_files");
  a->setIcon(KIcon("edit-find"));
  a->setText(i18n("&Find in Files..."));
  connect(a, SIGNAL(triggered()), this, SLOT(find()));

  setXMLFile("plugins/findinfiles/ui.rc");
  mw->guiFactory()->addClient(this);
}

KateFindInFilesView::~KateFindInFilesView ()
{
  foreach(KateResultView* view, m_resultViews)
    delete view->toolView(); // view is child of toolview -> view is deleted, too
  m_resultViews.clear();

  m_mw->guiFactory()->removeClient(this);

  delete m_findDialog;
  m_findDialog = 0;
}

void KateFindInFilesView::addResultView(KateResultView* view)
{
  m_resultViews.append(view);
}

void KateFindInFilesView::removeResultView(KateResultView* view)
{
  m_resultViews.removeAll(view);
}

KateFindDialog* KateFindInFilesView::findDialog()
{
  if (!m_findDialog) {
    m_findDialog = new KateFindDialog(m_mw, this);
  }
  return m_findDialog;
}

void KateFindInFilesView::find()
{
  if (!findDialog()->isVisible()) {
    findDialog()->useResultView(-1);
    findDialog()->show();
  }
}

int KateFindInFilesView::freeId()
{
  if (m_resultViews.size())
    return m_resultViews.last()->id() + 1;
  return 1;
}

KateResultView* KateFindInFilesView::toolViewFromId(int id)
{
  foreach (KateResultView* view, m_resultViews) {
    if (view->id() == id)
      return view;
  }
  return 0;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
