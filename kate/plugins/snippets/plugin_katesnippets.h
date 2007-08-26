/*
 * Copyright (C) 2004 Stephan Möres <Erdling@gmx.net>
 */

#ifndef _PLUGIN_KATESNIPPETS_H_
#define _PLUGIN_KATESNIPPETS_H_

#include <kate/application.h>
#include <kate/documentmanager.h>
#include <ktexteditor/document.h>
#include <kate/mainwindow.h>
#include <kate/plugin.h>
#include <ktexteditor/view.h>
#include <klibloader.h>
#include <klocale.h>


#include <q3listview.h>
#include <qregexp.h>
#include <kconfig.h>
#include <q3ptrlist.h>
#include <qtoolbutton.h>
#include <q3textedit.h>
#include <kiconloader.h>

#include "csnippet.h"
#include "cwidgetsnippets.h"

class KatePluginSnippetsView : public CWidgetSnippets, public KXMLGUIClient {

  Q_OBJECT

  friend class KatePluginSnippets;

public:
  KatePluginSnippetsView (Kate::MainWindow *w, QWidget *dock);
  virtual ~KatePluginSnippetsView ();
  CSnippet* findSnippetByListViewItem(Q3ListViewItem *item);

public slots:
  void slot_lvSnippetsSelectionChanged(Q3ListViewItem  * item);
  void slot_lvSnippetsClicked (Q3ListViewItem * item);
  void slot_lvSnippetsItemRenamed(Q3ListViewItem *lvi,int col, const QString& text);
  void slot_btnNewClicked();
  void slot_btnSaveClicked();
  void slot_btnDeleteClicked();

protected:
  void readConfig();
  void writeConfig();

private:
  KConfig                                 *config;
  Q3PtrList<CSnippet>                      lSnippets;

public:
  Kate::MainWindow *win;
  QWidget *dock;
};

class KatePluginSnippets : public Kate::Plugin, Kate::PluginViewInterface {
  Q_OBJECT

public:
  explicit KatePluginSnippets( QObject* parent = 0, const QStringList& = QStringList() );
  virtual ~KatePluginSnippets();

  void addView (Kate::MainWindow *win);
  void removeView (Kate::MainWindow *win);
  void storeViewConfig(KConfig* config, Kate::MainWindow* win, const QString& groupPrefix);
  void loadViewConfig(KConfig* config, Kate::MainWindow* win, const QString& groupPrefix);
  void storeGeneralConfig(KConfig* config, const QString& groupPrefix);
  void loadGeneralConfig(KConfig* config, const QString& groupPrefix);

private:
  Q3PtrList<class KatePluginSnippetsView>  m_views;

};

#endif // _PLUGIN_KATESNIPPETS_H_
