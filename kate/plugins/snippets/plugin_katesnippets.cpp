/*
 * Copyright (C) 2004 Stephan Möres <Erdling@gmx.net>
 */

#include "plugin_katesnippets.h"

#include <kaction.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kgenericfactory.h>
#include <QDate>
#include <QTime>
// let the world know ...
K_EXPORT_COMPONENT_FACTORY(katesnippetsplugin, KGenericFactory<KatePluginSnippets>( "katesnippets" ) )


// < IMPLEMENTAIONS for KatePluginSnippetsView >
//
//

/**
 * ctor KatePluginSnippetsView
 * @param w
 * @return
 */
KatePluginSnippetsView::KatePluginSnippetsView(Kate::MainWindow *w, QWidget *dock) : CWidgetSnippets(dock,"snippetswidget")
  , dock (dock)
{
  setComponentData (KComponentData("kate"));
  setXMLFile("plugins/katesnippets/plugin_katesnippets.rc");

  w->guiFactory()->addClient (this);
  win = w;


  //<make connections>
  connect (
    lvSnippets, SIGNAL( selectionChanged(Q3ListViewItem *) ),
    this, SLOT( slot_lvSnippetsSelectionChanged(Q3ListViewItem *) )
  );
  connect (
    lvSnippets, SIGNAL( doubleClicked (Q3ListViewItem *) ),
    this, SLOT( slot_lvSnippetsClicked(Q3ListViewItem  *) )
  );
  connect (
    lvSnippets, SIGNAL( itemRenamed(Q3ListViewItem *, int, const QString &) ),
    this, SLOT( slot_lvSnippetsItemRenamed(Q3ListViewItem *, int, const QString &) )
  );

  connect (
    btnNew, SIGNAL( clicked () ),
    this, SLOT( slot_btnNewClicked() )
  );
  connect (
    btnSave, SIGNAL( clicked () ),
    this, SLOT( slot_btnSaveClicked() )
  );
  connect (
    btnDelete, SIGNAL( clicked () ),
    this, SLOT( slot_btnDeleteClicked() )
  );
  //</make connections>

  lSnippets.setAutoDelete( TRUE ); // the list owns the objects

  config = new KConfig("katesnippetspluginrc");
  readConfig();

  // set text of selected item at startup
  slot_lvSnippetsSelectionChanged(lvSnippets->selectedItem() );
}


/**
 * dtor KatePluginSnippetsView
 * @return
 */
KatePluginSnippetsView::~ KatePluginSnippetsView() {
  writeConfig();

  win->guiFactory()->removeClient(this);
}


//
//
// < IMPLEMENTAIONS for KatePluginSnippetsView >




// < IMPLEMENTAIONS for KatePluginSnippets >
//
//

/**
 * ctor KatePluginSnippets
 * @param parent
 * @param name
 * @return
 */
KatePluginSnippets::KatePluginSnippets( QObject* parent, const QStringList& )
    : Kate::Plugin ( (Kate::Application*)parent, "KatePluginSnippets" ) {}

/**
 * dtor KatePluginSnippets
 * @return
 */
KatePluginSnippets::~KatePluginSnippets() {}

void KatePluginSnippets::storeViewConfig(KConfig* config, Kate::MainWindow* win, const QString& groupPrefix)
{
  // TODO: FIXME: port to new Kate interfaces
}
 
void KatePluginSnippets::loadViewConfig(KConfig* config, Kate::MainWindow*win, const QString& groupPrefix)
{
  // TODO: FIXME: port to new Kate interfaces
}

void KatePluginSnippets::storeGeneralConfig(KConfig* config, const QString& groupPrefix)
{
  // TODO: FIXME: port to new Kate interfaces
}

void KatePluginSnippets::loadGeneralConfig(KConfig* config, const QString& groupPrefix)
{
  // TODO: FIXME: port to new Kate interfaces
}

/**
 *
 * @param win
 */
void KatePluginSnippets::addView(Kate::MainWindow *win)
{
  QWidget *dock = win->createToolView(
              "kate_plugin_snippets",
              Kate::MainWindow::Left,
              SmallIcon("contents"),
              i18n("Snippets"));

  KatePluginSnippetsView *view = new KatePluginSnippetsView (win,dock);
  m_views.append(view);
}


/**
 *
 * @param win
 */
void KatePluginSnippets::removeView(Kate::MainWindow *win) {
  for (uint z=0; z < m_views.count(); z++)
    if (m_views.at(z)->win == win) {
      KatePluginSnippetsView *view = m_views.at(z);
      m_views.remove (view);
      delete view->dock;
    }
}

/**
 *
 * @param item
 */
void KatePluginSnippetsView::slot_lvSnippetsSelectionChanged(Q3ListViewItem  * item) {
  CSnippet *snippet;
  if ( (snippet = findSnippetByListViewItem(item))!= NULL ) {
    teSnippetText->setText(snippet->getValue());
  }

}


/**
 * Special meaning of <mark/> and <cursor/> ...
 * @param item
 */
void KatePluginSnippetsView::slot_lvSnippetsClicked (Q3ListViewItem  * item) {
  KTextEditor::View *kv = win->activeView();
  CSnippet *snippet;

  if (kv) {
    if ( (snippet = findSnippetByListViewItem(item))!= NULL ) {
      QString sText = snippet->getValue();
      QString sSelection = "";

      if ( kv->selection() ) {
        sSelection = kv->selectionText();
        // clear selection
        kv->removeSelectionText();
      }

      sText.replace( QRegExp("<mark/>"), sSelection );
      sText.replace( QRegExp("<date/>"), QDate::currentDate().toString(Qt::LocalDate) );
      sText.replace( QRegExp("<time/>"), QTime::currentTime().toString(Qt::LocalDate) );
      kv->document()->insertText ( kv->cursorPosition(), sText );
    }
    kv->setFocus();
  }
}


/**
 *
 * @param lvi
 * @param
 * @param text
 */
void KatePluginSnippetsView::slot_lvSnippetsItemRenamed(Q3ListViewItem *lvi,int /*col*/, const QString& text) {
  CSnippet *snippet;
  if ( (snippet = findSnippetByListViewItem(lvi)) != NULL ) {
    snippet->setKey( text );
    writeConfig();
  }
}


/**
 *
 */
void KatePluginSnippetsView::slot_btnNewClicked() {
  QString sKey = "New Snippet";
  QString sValue = "";

  Q3ListViewItem *lvi = insertItem(sKey, true);
  lSnippets.append( new CSnippet(sKey, sValue, lvi) );
}


/**
 *
 */
void KatePluginSnippetsView::slot_btnSaveClicked() {
  CSnippet *snippet;
  Q3ListViewItem *lvi = lvSnippets->selectedItem();
  if ( (snippet = findSnippetByListViewItem(lvi)) != NULL ) {
    snippet->setValue(teSnippetText->text() );
    writeConfig();
  }
}


/**
 *
 */
void KatePluginSnippetsView::slot_btnDeleteClicked() {
  CSnippet *snippet;
  Q3ListViewItem *lvi = lvSnippets->selectedItem();


  if ( (snippet = findSnippetByListViewItem(lvi)) != NULL ) {
    lvSnippets->takeItem(lvi);
    lSnippets.remove(snippet);
  }
}


/**
 *
 */
void KatePluginSnippetsView::readConfig() {
  QString sKey, sValue;
  Q3ListViewItem *lvi;

  config->setGroup("Snippets");

  int iNrOfSnippets = config->readEntry("NumberOfSnippets", "0").toInt() ;
  for (int i=0; i < iNrOfSnippets; i++) {
    QStringList slFields;
    slFields = config->readEntry ( QString::number(i),QStringList() );

    sKey   = slFields[0];
    sValue = slFields[1];

    lvi = insertItem(sKey, false);

    lSnippets.append( new CSnippet(sKey, sValue, lvi, this) );
  }

  // <defaults>
  if ( iNrOfSnippets == 0 ) {
    sKey	= "DEBUG variable";
    sValue = "## < DEBUG >\nout \"<pre>\\$<mark/> : \\\"$<mark/>\\\"\\n</pre>\"\n## </DEBUG >\n";
    lvi = insertItem(sKey, false);
    lSnippets.append( new CSnippet(sKey, sValue, lvi, this) );

    sKey	= "proc-header";
    sValue	= "## [created : <date/>, <time/>]\n## Description:\n## ============\n## The function \"<mark/>\" ...\n##\n##\n##\n##\n## Input:\n## ======\n##\n##\n##\nproc <mark/> {args} {\n\n	## add your code here\n\n	return \"\"\n}\n";
    lvi = insertItem(sKey, false);
    lSnippets.append( new CSnippet(sKey, sValue, lvi, this) );
  }
  // </defaults>

}


/**
 *
 */
void KatePluginSnippetsView::writeConfig() {
  config->setGroup("Snippets");

  int iNrOfSnippets = lSnippets.count();

  config->writeEntry("NumberOfSnippets", iNrOfSnippets );

  int i=0;

  CSnippet *snippet;
  for ( snippet = lSnippets.first(); snippet; snippet = lSnippets.next() ) {
    QStringList slFields;
    slFields.append( snippet->getKey() );
    slFields.append( snippet->getValue() );

    config->writeEntry ( QString::number(i), slFields, ',' );
    i++;
  }
  // sync to disc ...
  config->sync();
}


/**
 *
 * @param item
 * @return
 */
CSnippet* KatePluginSnippetsView::findSnippetByListViewItem(Q3ListViewItem *item) {
  CSnippet *snippet = NULL;
  for ( snippet = lSnippets.first(); snippet; snippet = lSnippets.next() ) {
    if ( snippet->getListViewItem() == item)
      break;
  }
  return snippet;
}

//
//
// < IMPLEMENTAIONS for KatePluginSnippets >

#include "plugin_katesnippets.moc"
