/***************************************************************************
 *                        plugin_katesymbolviewer.cpp  -  description
 *                           -------------------
 *  begin                : Apr 2 2003
 *  author               : 2003 Massimo Callegari
 *  email                : massimocallegari@yahoo.it
 *
 *  Changes:
 *  Nov 09 2004 v.1.3 - For changelog please refer to KDE CVS
 *  Nov 05 2004 v.1.2 - Choose parser from the current highlight. Minor i18n changes.
 *  Nov 28 2003 v.1.1 - Structured for multilanguage support
 *                      Added preliminary Tcl/Tk parser (thanks Rohit). To be improved.
 *                      Various bugfixing.
 *  Jun 19 2003 v.1.0 - Removed QTimer (polling is Evil(tm)... )
 *                      - Captured documentChanged() event to refresh symbol list
 *                      - Tooltips vanished into nowhere...sigh :(
 *  May 04 2003 v 0.6 - Symbol List becomes a K3ListView object. Removed Tooltip class.
 *                      Added a QTimer that every 200ms checks:
 *                      * if the list width has changed
 *                      * if the document has changed
 *                      Added an entry in the popup menu to switch between List and Tree mode
 *                      Various bugfixing.
 *  Apr 24 2003 v 0.5 - Added three check buttons in popup menu to show/hide symbols
 *  Apr 23 2003 v 0.4 - "View Symbol" moved in Settings menu. "Refresh List" is no
 *                      longer in Kate menu. Moved into a popup menu activated by a
 *                      mouse right button click. + Bugfixing.
 *  Apr 22 2003 v 0.3 - Added macro extraction + several bugfixing
 *  Apr 19 2003 v 0.2 - Added to CVS. Extract functions and structures
 *  Apr 07 2003 v 0.1 - First version.
 *
 ***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "plugin_katesymbolviewer.h"
#include "plugin_katesymbolviewer.moc"

#include <kaction.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kgenericfactory.h>
#include <kfiledialog.h>
#include <ktoggleaction.h>
#include <kactioncollection.h>

#include <QPixmap>
#include <QVBoxLayout>
#include <QLayout>
#include <QGroupBox>

#include <QResizeEvent>
#include <QMenu>

K_EXPORT_COMPONENT_FACTORY( katesymbolviewerplugin, KGenericFactory<KatePluginSymbolViewer>( "katesymbolviewer" ) )

KatePluginSymbolViewerView::KatePluginSymbolViewerView(Kate::MainWindow *w)
{
    KGlobal::locale()->insertCatalog("katesymbolviewer");
    KToggleAction *act = actionCollection()->add<KToggleAction>("view_insert_symbolviewer");
    act->setText(i18n("Hide Symbols"));
    connect(act,SIGNAL(toggled( bool )),this,SLOT(slotInsertSymbol()));
    act->setCheckedState(KGuiItem(i18n("Show Symbols")));

    setComponentData (KComponentData("kate"));
    setXMLFile("plugins/katesymbolviewer/ui.rc");
    w->guiFactory()->addClient (this);
  win = w;
  symbols = 0;

 m_Active = false;
 popup = new QMenu();
 popup->insertItem(i18n("Refresh List"), this, SLOT(slotRefreshSymbol()));
 popup->addSeparator();
 m_macro = popup->insertItem(i18n("Show Macros"), this, SLOT(toggleShowMacros()));
 m_struct = popup->insertItem(i18n("Show Structures"), this, SLOT(toggleShowStructures()));
 m_func = popup->insertItem(i18n("Show Functions"), this, SLOT(toggleShowFunctions()));
 popup->addSeparator();
 popup->insertItem(i18n("List/Tree Mode"), this, SLOT(slotChangeMode()));
 m_sort = popup->insertItem(i18n("Enable sorting"), this, SLOT(slotEnableSorting()));

 popup->setItemChecked(m_macro, true);
 popup->setItemChecked(m_struct, true);
 popup->setItemChecked(m_func, true);
 macro_on = true;
 struct_on = true;
 func_on = true;
 slotInsertSymbol();
}

KatePluginSymbolViewerView::~KatePluginSymbolViewerView()
{
  win->guiFactory()->removeClient (this);
  delete dock;
  delete popup;
}

void KatePluginSymbolViewerView::toggleShowMacros(void)
{
 bool s = !popup->isItemChecked(m_macro);
 popup->setItemChecked(m_macro, s);
 macro_on = s;
 slotRefreshSymbol();
}

void KatePluginSymbolViewerView::toggleShowStructures(void)
{
 bool s = !popup->isItemChecked(m_struct);
 popup->setItemChecked(m_struct, s);
 struct_on = s;
 slotRefreshSymbol();
}

void KatePluginSymbolViewerView::toggleShowFunctions(void)
{
 bool s = !popup->isItemChecked(m_func);
 popup->setItemChecked(m_func, s);
 func_on = s;
 slotRefreshSymbol();
}

void KatePluginSymbolViewerView::slotInsertSymbol()
{
 QPixmap cls( ( const char** ) class_xpm );
 QStringList titles;

 if (m_Active == false)
     {
      dock = win->createToolView("kate_plugin_symbolviewer", Kate::MainWindow::Left, cls, i18n("Symbol List"));

      symbols = new QTreeWidget(dock);
      treeMode = 0;

      connect(symbols, SIGNAL(itemActivated(QTreeWidgetItem *, int)), this, SLOT(goToSymbol(QTreeWidgetItem *)));
      connect(symbols, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(slotShowContextMenu(const QPoint&)));

      connect(win, SIGNAL(viewChanged()), this, SLOT(slotDocChanged()));
      //connect(symbols, SIGNAL(resizeEvent(QResizeEvent *)), this, SLOT(slotViewChanged(QResizeEvent *)));

      m_Active = true;
      //symbols->addColumn(i18n("Symbols"), symbols->parentWidget()->width());
      titles << tr("Symbols") << tr("Position");
      symbols->setColumnCount(2);
      symbols->setHeaderLabels(titles);

      symbols->setColumnHidden(1, TRUE);
      symbols->setSortingEnabled(FALSE); // was "symbols->setSorting(-1, FALSE);"
      symbols->setRootIsDecorated(0);
      symbols->setContextMenuPolicy(Qt::CustomContextMenu);
      symbols->setIndentation(10);
      //symbols->setShowToolTips(TRUE);

      /* First Symbols parsing here...*/
      parseSymbols();
     }
  else
     {
      delete dock;
      dock = 0;
      symbols = 0;
      m_Active = false;
     }
}

void KatePluginSymbolViewerView::slotRefreshSymbol()
{
  if (!symbols)
    return;
 symbols->clear();
 parseSymbols();
}

void KatePluginSymbolViewerView::slotChangeMode()
{
 treeMode = !treeMode;
 symbols->clear();
 parseSymbols();
}

void KatePluginSymbolViewerView::slotEnableSorting()
{
 lsorting = !lsorting;
 popup->setItemChecked(m_sort, lsorting);
 symbols->clear();
 if (lsorting == TRUE)
     symbols->setSortingEnabled(TRUE); // symbols->setSorting(0, TRUE);
 else
     symbols->setSortingEnabled(FALSE); // symbols->setSorting(-1, FALSE);

 parseSymbols();
 if (lsorting == TRUE) symbols->sortItems(0, Qt::AscendingOrder);
}

void KatePluginSymbolViewerView::slotDocChanged()
{
 //kDebug(13000)<<"Document changed !!!!"<<endl;
 slotRefreshSymbol();
}

void KatePluginSymbolViewerView::slotViewChanged(QResizeEvent *)
{
 kDebug(13000)<<"View changed !!!!"<<endl;
 symbols->setColumnWidth(0, symbols->parentWidget()->width());
}

void KatePluginSymbolViewerView::slotShowContextMenu(const QPoint &p)
{
 popup->popup(p);
}

void KatePluginSymbolViewerView::parseSymbols(void)
{
  QTreeWidgetItem *node = NULL;

  if (!win->activeView())
    return;

  KTextEditor::Document *doc = win->activeView()->document();

  // be sure we have some document around !
  if (!doc)
    return;

  /** Get the current highlighting mode */
  QString hlModeName = doc->mode();

  if (hlModeName == "C++" || hlModeName == "C")
     parseCppSymbols();
  else if (hlModeName == "Tcl/Tk")
     parseTclSymbols();
  else if (hlModeName == "Fortran")
     parseFortranSymbols();
  else if (hlModeName == "Perl")
     parsePerlSymbols();
  else if (hlModeName == "Python")
     parsePythonSymbols();
  else if (hlModeName == "Java")
     parseCppSymbols();
  else
     node = new QTreeWidgetItem(symbols,  QStringList(i18n("Sorry. Language not supported yet") ) );
}

void KatePluginSymbolViewerView::goToSymbol(QTreeWidgetItem *it)
{
  KTextEditor::View *kv = win->activeView();

  // be sure we really have a view !
  if (!kv)
    return;

//  kDebug(13000)<<"Slot Activated at pos: "<<symbols->itemIndex(it) <<endl;

  kv->setCursorPosition (KTextEditor::Cursor (it->text(1).toInt(NULL, 10), 0));
}

KatePluginSymbolViewer::KatePluginSymbolViewer( QObject* parent, const QStringList& )
    : Kate::Plugin ( (Kate::Application*)parent, "katesymbolviewerplugin" ),
    pConfig("katesymbolviewerpluginrc")
{
 kDebug(13000)<<"KatePluginSymbolViewer"<<endl;
}

KatePluginSymbolViewer::~KatePluginSymbolViewer()
{
  kDebug(13000)<<"~KatePluginSymbolViewer"<<endl;
  pConfig.sync();
}

Kate::PluginView *KatePluginSymbolViewer::createView (Kate::MainWindow *mainWindow)
{
  KatePluginSymbolViewerView *view = new KatePluginSymbolViewerView (mainWindow);
  m_views.append (view);
  return (Kate::PluginView *)view;
  //return new KatePluginSymbolViewerView2 (mainWindow);
}

void KatePluginSymbolViewer::storeViewConfig(KConfig* config, Kate::MainWindow* win, const QString& groupPrefix)
{
  // TODO: FIXME: port to new interfaces
}

void KatePluginSymbolViewer::loadViewConfig(KConfig* config, Kate::MainWindow* win, const QString& groupPrefix)
{
  // TODO: FIXME: port to new interfaces
}

void KatePluginSymbolViewer::storeGeneralConfig(KConfig* config, const QString& groupPrefix)
{
  // TODO: FIXME: port to new interfaces
}

void KatePluginSymbolViewer::loadGeneralConfig(KConfig* config, const QString& groupPrefix)
{
  // TODO: FIXME: port to new interfaces
}

Kate::PluginConfigPage* KatePluginSymbolViewer::configPage(
    uint, QWidget *w, const char* /*name*/)
{
  KatePluginSymbolViewerConfigPage* p = new KatePluginSymbolViewerConfigPage(this, w);
  initConfigPage( p );
  connect( p, SIGNAL(configPageApplyRequest(KatePluginSymbolViewerConfigPage*)),
      SLOT(applyConfig(KatePluginSymbolViewerConfigPage *)) );
  return (Kate::PluginConfigPage*)p;
}

void KatePluginSymbolViewer::initConfigPage( KatePluginSymbolViewerConfigPage* p )
{
  p->viewReturns->setChecked(pConfig.group("global").readEntry("view_types", true));
  p->expandTree->setChecked(pConfig.group("global").readEntry("expand_tree", false));
}

void KatePluginSymbolViewer::applyConfig( KatePluginSymbolViewerConfigPage* p )
{
 for (int z=0; z < m_views.count(); z++)
  {
    m_views.at(z)->types_on = p->viewReturns->isChecked();
    m_views.at(z)->expanded_on = p->expandTree->isChecked();
  //kDebug(13000)<<"KatePluginSymbolViewer: Configuration applied.("<<m_SymbolView->types_on<<")"<<endl;
    m_views.at(z)->slotRefreshSymbol();
  }

  pConfig.group("global").writeEntry("view_types", p->viewReturns->isChecked());
  pConfig.group("global").writeEntry("expand_tree", p->expandTree->isChecked());
}

// BEGIN KatePluginSymbolViewerConfigPage
KatePluginSymbolViewerConfigPage::KatePluginSymbolViewerConfigPage(
    QObject* /*parent*/ /*= 0L*/, QWidget *parentWidget /*= 0L*/)
  : Kate::PluginConfigPage( parentWidget )
{
  QGroupBox* groupBox = new QGroupBox( i18n("Parser Options"), this);

  viewReturns = new QCheckBox(i18n("Display functions parameters"));
  expandTree = new QCheckBox(i18n("Automatically expand nodes in tree mode"));

  QVBoxLayout* top = new QVBoxLayout();
  top->addWidget(viewReturns);
  top->addWidget(expandTree);
  top->addStretch(1);

  groupBox->setLayout(top);

//  throw signal changed
  connect(viewReturns, SIGNAL(toggled(bool)), this, SIGNAL(changed()));
  connect(expandTree, SIGNAL(toggled(bool)), this, SIGNAL(changed()));
}

KatePluginSymbolViewerConfigPage::~KatePluginSymbolViewerConfigPage() {}

void KatePluginSymbolViewerConfigPage::apply()
{
    emit configPageApplyRequest( this );
}
// END KatePluginSymbolViewerConfigPage

