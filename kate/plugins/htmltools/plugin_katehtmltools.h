 /***************************************************************************
                          plugin_katehtmltools.h  -  description
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

#ifndef _PLUGIN_KANT_HTMLTOOLS_H
#define _PLUGIN_KANT_HTMLTOOLS_H

#include <qstring.h>

#include <kate/plugin.h>
#include <kate/application.h>
#include <ktexteditor/view.h>
#include <ktexteditor/document.h>
#include <kate/documentmanager.h>
#include <kate/mainwindow.h>

#include <QList>

class PluginKateHtmlTools : public Kate::Plugin, Kate::PluginViewInterface
{
  Q_OBJECT
 
public:
  PluginKateHtmlTools( QObject* parent = 0, const QStringList& = QStringList() );
  virtual ~PluginKateHtmlTools();

  void addView (Kate::MainWindow *win);
  void removeView (Kate::MainWindow *win);
  void storeViewConfig(KConfig* config, Kate::MainWindow* win, const QString& groupPrefix);
  void loadViewConfig(KConfig* config, Kate::MainWindow* win, const QString& groupPrefix);
  void storeGeneralConfig(KConfig* config, const QString& groupPrefix);
  void loadGeneralConfig(KConfig* config, const QString& groupPrefix);

private: 
  QList<class PluginView*> m_views; 

  QString KatePrompt (const QString & strTitle, const QString &strPrompt,
			     QWidget * that);
  void slipInHTMLtag (KTextEditor::View & view, QString text);

public slots:
  void slotEditHTMLtag();
};

#endif // _PLUGIN_KANT_HTMLTOOLS_H
