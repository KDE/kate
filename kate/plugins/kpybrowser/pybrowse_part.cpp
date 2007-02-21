/***************************************************************************
                          pybrowse_part.cpp  -  description
                             -------------------
    begin                : Tue Aug 28 2001
    copyright            : (C) 2001 by Christian Bird
    email                : chrisb@lineo.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "pybrowse_part.h"
#include "pybrowse_part.moc"
#include "kpybrowser.h"
#include "pybrowse.xpm"

#include <kgenericfactory.h>
#include <kaction.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kmessagebox.h>
#include <qimage.h>
#include <kactioncollection.h>
//Added by qt3to4:
#include <QPixmap>
#include <k3dockwidget.h>

K_EXPORT_COMPONENT_FACTORY( katepybrowseplugin, KGenericFactory<KatePluginPyBrowse>( "katepybrowse" ) )

PluginViewPyBrowse::PluginViewPyBrowse (Kate::MainWindow *w)
 : win (w)
{
   QAction *a = actionCollection()->addAction("python_update_pybrowse");
   a->setText(i18n("Update Python Browser"));
   connect(a, SIGNAL(triggered()), this, SLOT(slotUpdatePyBrowser()));

   //set up the menus
   setComponentData(KComponentData("kate"));
   setXMLFile( "plugins/katepybrowse/ui.rc" );
   win->guiFactory()->addClient(this);

   //create a python head pixmap for the tab
   QPixmap *py_pixmap = new QPixmap(pybrowse_xpm);
   QImage py_image = py_pixmap->convertToImage().smoothScale(20, 20);
   py_pixmap->convertFromImage(py_image);

   //create the browser and put it into a dockwidget using kate's tool view manager

   my_dock = win->createToolView("kate_plugin_kpybrowser", Kate::MainWindow::Left, (*py_pixmap), i18n("Python Browser"));
   kpybrowser = new KPyBrowser(my_dock, "kpybrowser");

   connect(kpybrowser, SIGNAL(selected(QString, int)), this, SLOT(slotSelected(QString, int)));
}

PluginViewPyBrowse::~PluginViewPyBrowse ()
{
  win->guiFactory()->removeClient (this);
  delete my_dock;
}


void PluginViewPyBrowse::slotSelected(QString name, int line)
{
  //TO DO - deal with setting the active view to be the file that has this class
  //if multiple files are open.

  if (name == "Classes" || name == "Globals")
          return;

  KTextEditor::View *view = win->activeView();

  KTextEditor::Document *doc = view->document();
  QString docline = doc->line(line);
  int numlines = doc->lines();
  int done = 0, apiline = -1, forward_line = line,backward_line = line-1;
  while (!done)
  {
    done = 1;
          if (forward_line < numlines)
          {
                  if (doc->line(forward_line).find(name) > -1)
                  {
                          apiline = forward_line;
                          break;
                  }
                  forward_line++;
                  done = 0;
          }
          if (backward_line > -1)
          {
            if (doc->line(backward_line).find(name) > -1)
                  {
                          apiline = backward_line;
                          break;
                  }
                  backward_line--;
                  done = 0;
          }
  }
  if (apiline == -1)
  {
          KMessageBox::information(0,
                  i18n("Could not find method/class %1.", name), i18n("Selection"));
  }
  else
  {
          view->setCursorPosition(KTextEditor::Cursor (apiline, 0));
  }
  view->setFocus();
}

void PluginViewPyBrowse::slotUpdatePyBrowser()
{
  KTextEditor::View *view = win->activeView();
  if (view == NULL)
  	return;
  QString pytext(view->document()->text());
  kpybrowser->parseText(pytext);
}

void PluginViewPyBrowse::slotShowPyBrowser()
{
   //TO DO implement this later so that you can turn the browser off and on
}

KatePluginPyBrowse::KatePluginPyBrowse( QObject* parent, const QStringList& )
    : Kate::Plugin ( (Kate::Application *)parent, "KatePluginPyBrowse" )
{
}

KatePluginPyBrowse::~KatePluginPyBrowse()
{
}

void KatePluginPyBrowse::addView (Kate::MainWindow *win)
{
   PluginViewPyBrowse *view = new PluginViewPyBrowse(win);
   m_views.append (view);
}

void KatePluginPyBrowse::removeView(Kate::MainWindow *win)
{
  for (uint z=0; z < m_views.count(); z++)
    if (m_views.at(z)->win == win)
    {
      PluginViewPyBrowse *view = m_views.at(z);
      m_views.remove (view);
      delete view;
    }
}

void KatePluginPyBrowse::storeViewConfig(KConfig* config, Kate::MainWindow* win, const QString& groupPrefix)
{
  // TODO: FIXME: port to new Kate interfaces
}
 
void KatePluginPyBrowse::loadViewConfig(KConfig* config, Kate::MainWindow*win, const QString& groupPrefix)
{
  // TODO: FIXME: port to new Kate interfaces
}

void KatePluginPyBrowse::storeGeneralConfig(KConfig* config, const QString& groupPrefix)
{
  // TODO: FIXME: port to new Kate interfaces
}

void KatePluginPyBrowse::loadGeneralConfig(KConfig* config, const QString& groupPrefix)
{
  // TODO: FIXME: port to new Kate interfaces
}
 
