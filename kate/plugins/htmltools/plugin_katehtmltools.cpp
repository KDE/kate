/***************************************************************************
                          plugin_katehtmltools.cpp  -  description
                             -------------------
    begin                : FRE Feb 23 2001
    copyright            : (C) 2001 by Joseph Wenninger
    email                : jowenn@bigfoot.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "plugin_katehtmltools.h"
#include "plugin_katehtmltools.moc"
#include <kactioncollection.h>
#include <ktexteditor/cursor.h>

#include <kinputdialog.h>
#include <kaction.h>
#include <kcomponentdata.h>
#include <klocale.h>
#include <cassert>
#include <kdebug.h>
#include <kgenericfactory.h>

K_EXPORT_COMPONENT_FACTORY( katehtmltoolsplugin, KGenericFactory<PluginKateHtmlTools>( "katehtmltools" ) )


class PluginView : public KXMLGUIClient
{
  friend class PluginKateHtmlTools;

  public:
    Kate::MainWindow *win;
};

PluginKateHtmlTools::PluginKateHtmlTools( QObject* parent, const QStringList& )
    : Kate::Plugin ( (Kate::Application *)parent, "kate-html-tools-plugin" )
{
}

PluginKateHtmlTools::~PluginKateHtmlTools()
{
}

void PluginKateHtmlTools::addView(Kate::MainWindow *win)
{
    // TODO: doesn't this have to be deleted?
    PluginView *view = new PluginView ();

    QAction *a = view->actionCollection()->addAction("edit_HTML_tag");
    a->setText(i18n("HT&ML Tag..."));
    a->setShortcut( Qt::ALT + Qt::Key_Minus );
    connect( a, SIGNAL( triggered(bool) ), this, SLOT( slotEditHTMLtag() ) );

    view->setComponentData (KComponentData("kate"));
    view->setXMLFile( "plugins/katehtmltools/ui.rc" );
    win->guiFactory()->addClient (view);
    view->win = win;

   m_views.append (view);
}

void PluginKateHtmlTools::removeView(Kate::MainWindow *win)
{
  for (int z=0; z < m_views.count(); z++)
    if (m_views.at(z)->win == win)
    {
      PluginView *view = m_views.at(z);
      m_views.removeAll (view);
      win->guiFactory()->removeClient (view);
      delete view;
    }
}
void PluginKateHtmlTools::storeViewConfig(KConfig* config, Kate::MainWindow* win, const QString& groupPrefix)
{
  // TODO: FIXME: port to new Kate interfaces
}

void PluginKateHtmlTools::loadViewConfig(KConfig* config, Kate::MainWindow*win, const QString& groupPrefix)
{
  // TODO: FIXME: port to new Kate interfaces
}

void PluginKateHtmlTools::storeGeneralConfig(KConfig* config, const QString& groupPrefix)
{
  // TODO: FIXME: port to new Kate interfaces
}

void PluginKateHtmlTools::loadGeneralConfig(KConfig* config, const QString& groupPrefix)
{
  // TODO: FIXME: port to new Kate interfaces
}

void PluginKateHtmlTools::slotEditHTMLtag()
//  PCP
{
  if (!application()->activeMainWindow())
    return;

  KTextEditor::View *kv=application()->activeMainWindow()->activeView();
  if (!kv) return;

  QString text ( KatePrompt ( i18n("HTML Tag"),
                        i18n("Enter HTML tag contents (the <, >, and closing tag will be supplied):"),
                        (QWidget *)kv)
                         );

  if ( !text.isEmpty () )
      slipInHTMLtag (*kv, text); // user entered something and pressed ok

}


QString PluginKateHtmlTools::KatePrompt
        (
        const QString & strTitle,
        const QString & strPrompt,
        QWidget * that
        )
{
  //  TODO: Make this a "memory edit" field with a combo box
  //  containing prior entries
  bool ok;
  QString result=KInputDialog::getText( strTitle, strPrompt,
    QString::null, &ok,that);	//krazy:exclude=nullstrassign for old broken gcc
  if (ok) return result; else return "";
}


void PluginKateHtmlTools::slipInHTMLtag (KTextEditor::View & view, QString text)  //  PCP
{
  view.document()->startEditing();

  QStringList list = text.split(' ');
  QString marked = view.selectionText ();
  int preDeleteLine = 0, preDeleteCol = 0;
  view.cursorPosition ().position (preDeleteLine, preDeleteCol);

  if (marked.length() > 0)
    view.removeSelectionText ();
  int line = 0, col = 0;
  view.cursorPosition ().position (line, col);
  QString pre ("<" + text + ">");
  QString post;
  if (list.count () > 0)  post = "</" + list[0] + ">";
  view.document()->insertText (KTextEditor::Cursor (line,col),pre + marked + post);

  //  all this muck to leave the cursor exactly where the user
  //  put it...

  //  Someday we will can all this (unless if it already
  //  is canned and I didn't find it...)

  //  The second part of the if disrespects the display bugs
  //  when we try to reselect. TODO: fix those bugs, and we can
  //  un-break this if...

#ifdef __GNUC__
#warning "fix me (shiftCurosor*)"
#endif  
#if 0
  if (preDeleteLine == line && -1 == marked.find ('\n'))
    if (preDeleteLine == line && preDeleteCol == col)
        {
        ci->setCursorPosition (line, col + pre.length () + marked.length () - 1);

        for (int x (marked.length());  x--;)
                view.shiftCursorLeft ();
        }
    else
        {
        ci->setCursorPosition (line, col += pre.length ());

        for (int x (marked.length());  x--;)
                view.shiftCursorRight ();
        }
#endif

  view.document()->endEditing();
}
