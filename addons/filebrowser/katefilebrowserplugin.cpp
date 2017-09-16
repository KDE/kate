/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>
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

//BEGIN Includes
#include "katefilebrowserplugin.h"
#include "katefilebrowserconfig.h"
#include "katefilebrowser.h"

#include <KLocalizedString>
#include <KPluginFactory>

#include <QKeyEvent>
#include <QIcon>
//END Includes

K_PLUGIN_FACTORY_WITH_JSON (KateFileBrowserPluginFactory, "katefilebrowserplugin.json", registerPlugin<KateFileBrowserPlugin>();)

//BEGIN KateFileBrowserPlugin
KateFileBrowserPlugin::KateFileBrowserPlugin(QObject* parent, const QList<QVariant>&)
  : KTextEditor::Plugin (parent)
{
}

QObject *KateFileBrowserPlugin::createView (KTextEditor::MainWindow *mainWindow)
{
  KateFileBrowserPluginView* view = new KateFileBrowserPluginView (this, mainWindow);
  connect(view, &KateFileBrowserPluginView::destroyed, this, &KateFileBrowserPlugin::viewDestroyed);
  m_views.append(view);
  return view;
}

void KateFileBrowserPlugin::viewDestroyed(QObject* view)
{
  // do not access the view pointer, since it is partially destroyed already
  m_views.removeAll(static_cast<KateFileBrowserPluginView *>(view));
}

int KateFileBrowserPlugin::configPages() const
{
  return 1;
}

KTextEditor::ConfigPage *KateFileBrowserPlugin::configPage (int number, QWidget *parent)
{
  if (number != 0)
    return nullptr;
  return new KateFileBrowserConfigPage(parent, m_views[0]->m_fileBrowser);
}
//END KateFileBrowserPlugin



//BEGIN KateFileBrowserPluginView
KateFileBrowserPluginView::KateFileBrowserPluginView (KTextEditor::Plugin *plugin, KTextEditor::MainWindow *mainWindow)
  : QObject (mainWindow)
  , m_toolView(
        mainWindow->createToolView(plugin,
            QStringLiteral ("kate_private_plugin_katefileselectorplugin")
          , KTextEditor::MainWindow::Left
          , QIcon::fromTheme(QStringLiteral("document-open"))
          , i18n("Filesystem Browser")
          )
      )
  , m_fileBrowser(new KateFileBrowser(mainWindow, m_toolView))
  , m_mainWindow (mainWindow)
{
  m_toolView->installEventFilter(this);
}

KateFileBrowserPluginView::~KateFileBrowserPluginView ()
{
  // cleanup, kill toolview + console
  delete m_fileBrowser->parentWidget();
}

void KateFileBrowserPluginView::readSessionConfig (const KConfigGroup& config)
{
  m_fileBrowser->readSessionConfig(config);
}

void KateFileBrowserPluginView::writeSessionConfig (KConfigGroup& config)
{
  m_fileBrowser->writeSessionConfig(config);
}

bool KateFileBrowserPluginView::eventFilter(QObject* obj, QEvent* event)
{
  if (event->type() == QEvent::KeyPress)
  {
    QKeyEvent* ke = static_cast<QKeyEvent*>(event);
    if ((obj == m_toolView) && (ke->key() == Qt::Key_Escape))
    {
      m_mainWindow->hideToolView(m_toolView);
      event->accept();
      return true;
    }
  }
  return QObject::eventFilter(obj, event);
}
//ENDKateFileBrowserPluginView

#include "katefilebrowserplugin.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
