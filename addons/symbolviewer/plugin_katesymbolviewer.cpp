/***************************************************************************
 *                        plugin_katesymbolviewer.cpp  -  description
 *                           -------------------
 * Copyright (C) 2014 by Kåre Särs <kare.sars@iki.fi>
 *
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
 *                      Added an entry in the m_popup menu to switch between List and Tree mode
 *                      Various bugfixing.
 *  Apr 24 2003 v 0.5 - Added three check buttons in m_popup menu to show/hide symbols
 *  Apr 23 2003 v 0.4 - "View Symbol" moved in Settings menu. "Refresh List" is no
 *                      longer in Kate menu. Moved into a m_popup menu activated by a
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

#include <QAction>
#include <klocalizedstring.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <kaboutdata.h>
#include <ktoggleaction.h>
#include <KActionCollection>
#include <KXMLGUIFactory>
#include <KConfigGroup>
#include <KSharedConfig>


#include <ktexteditor/configinterface.h>
#include <ktexteditor/cursor.h>

#include <QPixmap>
#include <QVBoxLayout>
#include <QGroupBox>

#include <QResizeEvent>
#include <QMenu>
#include <QPainter>
#include <QTimer>

K_PLUGIN_FACTORY_WITH_JSON (KatePluginSymbolViewerFactory, "katesymbolviewerplugin.json", registerPlugin<KatePluginSymbolViewer>();)

KatePluginSymbolViewerView::KatePluginSymbolViewerView(KatePluginSymbolViewer *plugin, KTextEditor::MainWindow *mw)
:QObject(mw)
,m_mainWindow(mw)
,m_plugin(plugin)
{
  // FIXME KF5 KGlobal::locale()->insertCatalog("katesymbolviewerplugin");

  KXMLGUIClient::setComponentName (QLatin1String("katesymbolviewer"), i18n ("SymbolViewer"));
  setXMLFile(QLatin1String("ui.rc"));

  mw->guiFactory()->addClient (this);
  m_symbols = nullptr;

  m_popup = new QMenu(m_symbols);
  m_treeOn = m_popup->addAction(i18n("Tree Mode"), this, &KatePluginSymbolViewerView::parseSymbols);
  m_treeOn->setCheckable(true);
  m_sort = m_popup->addAction(i18n("Show Sorted"), this, &KatePluginSymbolViewerView::parseSymbols);
  m_sort->setCheckable(true);
  m_popup->addSeparator();
  m_macro = m_popup->addAction(i18n("Show Macros"), this, &KatePluginSymbolViewerView::parseSymbols);
  m_macro->setCheckable(true);
  m_struct = m_popup->addAction(i18n("Show Structures"), this, &KatePluginSymbolViewerView::parseSymbols);
  m_struct->setCheckable(true);
  m_func = m_popup->addAction(i18n("Show Functions"), this, &KatePluginSymbolViewerView::parseSymbols);
  m_func->setCheckable(true);

  KConfigGroup config(KSharedConfig::openConfig(), "PluginSymbolViewer");
  m_plugin->typesOn = config.readEntry(QLatin1String("ViewTypes"), false);
  m_plugin->expandedOn = config.readEntry(QLatin1String("ExpandTree"), false);
  m_treeOn->setChecked(config.readEntry(QLatin1String("TreeView"), false));
  m_sort->setChecked(config.readEntry(QLatin1String("SortSymbols"), false));

  m_macro->setChecked(true);
  m_struct->setChecked(true);
  m_func->setChecked(true);

  m_updateTimer.setSingleShot(true);
  connect(&m_updateTimer, &QTimer::timeout, this, &KatePluginSymbolViewerView::parseSymbols);

  m_currItemTimer.setSingleShot(true);
  connect(&m_currItemTimer, &QTimer::timeout, this, &KatePluginSymbolViewerView::updateCurrTreeItem);

  QPixmap cls( ( const char** ) class_xpm );

  m_toolview = m_mainWindow->createToolView(plugin, QLatin1String("kate_plugin_symbolviewer"),
                                            KTextEditor::MainWindow::Left,
                                            cls,
                                            i18n("Symbol List"));

  QWidget *container = new QWidget(m_toolview);
  QHBoxLayout *layout = new QHBoxLayout(container);

  m_symbols = new QTreeWidget();
  m_symbols->setFocusPolicy(Qt::NoFocus);
  m_symbols->setLayoutDirection( Qt::LeftToRight );
  layout->addWidget(m_symbols, 10);
  layout->setContentsMargins(0,0,0,0);

  connect(m_symbols, &QTreeWidget::itemClicked, this, &KatePluginSymbolViewerView::goToSymbol);
  connect(m_symbols, &QTreeWidget::customContextMenuRequested, this, &KatePluginSymbolViewerView::slotShowContextMenu);

  connect(m_mainWindow, &KTextEditor::MainWindow::viewChanged, this, &KatePluginSymbolViewerView::slotDocChanged);

  QStringList titles;
  titles << i18nc("@title:column", "Symbols") << i18nc("@title:column", "Position");
  m_symbols->setColumnCount(2);
  m_symbols->setHeaderLabels(titles);

  m_symbols->setColumnHidden(1, true);
  m_symbols->setSortingEnabled(m_sort->isChecked());
  m_symbols->setRootIsDecorated(0);
  m_symbols->setContextMenuPolicy(Qt::CustomContextMenu);
  m_symbols->setIndentation(10);

  m_toolview->installEventFilter(this);
}

KatePluginSymbolViewerView::~KatePluginSymbolViewerView()
{
  m_mainWindow->guiFactory()->removeClient (this);
  delete m_toolview;
  delete m_popup;
}

void KatePluginSymbolViewerView::slotDocChanged()
{
 parseSymbols();

 KTextEditor::View *view = m_mainWindow->activeView();
 //qDebug()<<"Document changed !!!!" << view;
 if (view) {
   connect(view, &KTextEditor::View::cursorPositionChanged, this, &KatePluginSymbolViewerView::cursorPositionChanged, Qt::UniqueConnection);

   if (view->document()) {
     connect(view->document(), &KTextEditor::Document::textChanged, this, &KatePluginSymbolViewerView::slotDocEdited, Qt::UniqueConnection);
   }
 }
}

void KatePluginSymbolViewerView::slotDocEdited()
{
  m_currItemTimer.stop(); // Avoid unneeded update
  m_updateTimer.start(500);
}

void KatePluginSymbolViewerView::cursorPositionChanged()
{
  if (m_updateTimer.isActive()) {
    // No need for update, will come anyway
    return;
  }
  m_currItemTimer.start(100);
}

void KatePluginSymbolViewerView::updateCurrTreeItem()
{
  if (!m_mainWindow) {
    return;
  }
  KTextEditor::View* editView = m_mainWindow->activeView();
  if (!editView) {
    return;
  }
  KTextEditor::Document* doc = editView->document();
  if (!doc) {
    return;
  }

  int currLine = editView->cursorPositionVirtual().line();
  if (currLine == m_oldCursorLine) {
    // Nothing to do
    return;
  }
  m_oldCursorLine = currLine;

  int newItemLine = 0;
  QTreeWidgetItem *newItem = nullptr;
  QTreeWidgetItem *tmp = nullptr;
  for (int i=0; i<m_symbols->topLevelItemCount(); i++) {
    tmp = newActveItem(newItemLine, currLine, m_symbols->topLevelItem(i));
    if (tmp) newItem = tmp;
  }

  if (newItem) {
    m_symbols->blockSignals(true);
    m_symbols->setCurrentItem(newItem);
    m_symbols->blockSignals(false);
  }
}

QTreeWidgetItem *KatePluginSymbolViewerView::newActveItem(int &newItemLine, int currLine, QTreeWidgetItem *item)
{
  QTreeWidgetItem *newItem = nullptr;
  QTreeWidgetItem *tmp = nullptr;
  int itemLine = item->data(1, Qt::DisplayRole).toInt();
  if ((itemLine <= currLine) && (itemLine >= newItemLine)) {
    newItemLine = itemLine;
    newItem = item;
  }

  for (int i=0; i<item->childCount(); i++) {
    tmp = newActveItem(newItemLine, currLine, item->child(i));
    if (tmp) newItem = tmp;
  }

  return newItem;
}

bool KatePluginSymbolViewerView::eventFilter(QObject *obj, QEvent *event)
{
  if (event->type() == QEvent::KeyPress) {
    QKeyEvent *ke = static_cast<QKeyEvent*>(event);
    if ((obj == m_toolview) && (ke->key() == Qt::Key_Escape)) {
      m_mainWindow->activeView()->setFocus();
      event->accept();
      return true;
    }
  }
  return QObject::eventFilter(obj, event);
}

void KatePluginSymbolViewerView::slotShowContextMenu(const QPoint&)
{
  m_popup->popup(QCursor::pos(), m_treeOn);
}

void KatePluginSymbolViewerView::parseSymbols()
{
  if (!m_symbols)
    return;

  m_symbols->clear();
  // Qt docu recommends to populate view with disabled sorting
  // https://doc.qt.io/qt-5/qtreeview.html#sortingEnabled-prop
  m_symbols->setSortingEnabled(false);

  if (!m_mainWindow->activeView())
    return;

  KTextEditor::Document *doc = m_mainWindow->activeView()->document();

  // be sure we have some document around !
  if (!doc)
    return;

  /** Get the current highlighting mode */
  QString hlModeName = doc->mode();

  if (hlModeName.contains(QLatin1String("C++")) || hlModeName == QLatin1String("C") || hlModeName == QLatin1String("ANSI C89"))
     parseCppSymbols();
 else if (hlModeName == QLatin1String("PHP (HTML)"))
    parsePhpSymbols();
  else if (hlModeName == QLatin1String("Tcl/Tk"))
     parseTclSymbols();
  else if (hlModeName == QLatin1String("Fortran"))
     parseFortranSymbols();
  else if (hlModeName == QLatin1String("Perl"))
     parsePerlSymbols();
  else if (hlModeName == QLatin1String("Python"))
     parsePythonSymbols();
 else if (hlModeName == QLatin1String("Ruby"))
    parseRubySymbols();
  else if (hlModeName == QLatin1String("Java"))
     parseCppSymbols();
  else if (hlModeName == QLatin1String("xslt"))
     parseXsltSymbols();
  else if (hlModeName == QLatin1String("Bash"))
     parseBashSymbols();
  else if (hlModeName == QLatin1String("ActionScript 2.0") ||
    hlModeName == QLatin1String("JavaScript") ||
    hlModeName == QLatin1String("QML"))
     parseEcmaSymbols();
  else
    new QTreeWidgetItem(m_symbols,  QStringList(i18n("Sorry. Language not supported yet") ) );

  m_oldCursorLine = -1;
  updateCurrTreeItem();
  if (m_sort->isChecked())
    m_symbols->sortItems(0, Qt::AscendingOrder);
}

void KatePluginSymbolViewerView::goToSymbol(QTreeWidgetItem *it)
{
  KTextEditor::View *kv = m_mainWindow->activeView();

  // be sure we really have a view !
  if (!kv)
    return;

  //qDebug()<<"Slot Activated at pos: "<<m_symbols->indexOfTopLevelItem(it);

  kv->setCursorPosition (KTextEditor::Cursor (it->text(1).toInt(nullptr, 10), 0));
}

KatePluginSymbolViewer::KatePluginSymbolViewer( QObject* parent, const QList<QVariant>& )
: KTextEditor::Plugin (parent)
{
 //qDebug()<<"KatePluginSymbolViewer";
}

KatePluginSymbolViewer::~KatePluginSymbolViewer()
{
  //qDebug()<<"~KatePluginSymbolViewer";
}

QObject *KatePluginSymbolViewer::createView (KTextEditor::MainWindow *mainWindow)
{
  m_view = new KatePluginSymbolViewerView (this, mainWindow);
  return m_view;
}

KTextEditor::ConfigPage* KatePluginSymbolViewer::configPage(int, QWidget *parent)
{
  KatePluginSymbolViewerConfigPage* p = new KatePluginSymbolViewerConfigPage(this, parent);

  KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("PluginSymbolViewer"));
  p->viewReturns->setChecked(config.readEntry(QLatin1String("ViewTypes"), false));
  p->expandTree->setChecked(config.readEntry(QLatin1String("ExpandTree"), false));
  p->treeView->setChecked(config.readEntry(QLatin1String("TreeView"), false));
  p->sortSymbols->setChecked(config.readEntry(QLatin1String("SortSymbols"), false));
  connect(p, &KatePluginSymbolViewerConfigPage::configPageApplyRequest, this, &KatePluginSymbolViewer::applyConfig);
  return (KTextEditor::ConfigPage*)p;
}

void KatePluginSymbolViewer::applyConfig( KatePluginSymbolViewerConfigPage* p )
{
  KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("PluginSymbolViewer"));
  config.writeEntry(QLatin1String("ViewTypes"), p->viewReturns->isChecked());
  config.writeEntry(QLatin1String("ExpandTree"), p->expandTree->isChecked());
  config.writeEntry(QLatin1String("TreeView"), p->treeView->isChecked());
  config.writeEntry(QLatin1String("SortSymbols"), p->sortSymbols->isChecked());

  typesOn = p->viewReturns->isChecked();
  expandedOn = p->expandTree->isChecked();
  if (m_view) {
    m_view->m_treeOn->setChecked(p->treeView->isChecked());
    m_view->m_sort->setChecked(p->sortSymbols->isChecked());
  }
}

// BEGIN KatePluginSymbolViewerConfigPage
KatePluginSymbolViewerConfigPage::KatePluginSymbolViewerConfigPage(
    QObject* /*parent*/ /*= 0L*/, QWidget *parentWidget /*= 0L*/)
  : KTextEditor::ConfigPage( parentWidget )
{
  QVBoxLayout *lo = new QVBoxLayout( this );
  //int spacing = KDialog::spacingHint();
  //lo->setSpacing( spacing );

  viewReturns = new QCheckBox(i18n("Display functions parameters"));
  expandTree = new QCheckBox(i18n("Automatically expand nodes in tree mode"));
  treeView = new QCheckBox(i18n("Always display symbols in tree mode"));
  sortSymbols = new QCheckBox(i18n("Always sort symbols"));
  

  QGroupBox* parserGBox = new QGroupBox( i18n("Parser Options"), this);
  QVBoxLayout* top = new QVBoxLayout(parserGBox);
  top->addWidget(viewReturns);
  top->addWidget(expandTree);
  top->addWidget(treeView);
  top->addWidget(sortSymbols);

  //QGroupBox* generalGBox = new QGroupBox( i18n("General Options"), this);
  //QVBoxLayout* genLay = new QVBoxLayout(generalGBox);
  //genLay->addWidget(  );

  lo->addWidget( parserGBox );
  //lo->addWidget( generalGBox );
  lo->addStretch( 1 );


//  throw signal changed
  connect(viewReturns, &QCheckBox::toggled, this, &KatePluginSymbolViewerConfigPage::changed);
  connect(expandTree, &QCheckBox::toggled, this, &KatePluginSymbolViewerConfigPage::changed);
  connect(treeView, &QCheckBox::toggled, this, &KatePluginSymbolViewerConfigPage::changed);
  connect(sortSymbols, &QCheckBox::toggled, this, &KatePluginSymbolViewerConfigPage::changed);
}

KatePluginSymbolViewerConfigPage::~KatePluginSymbolViewerConfigPage() {}

QString KatePluginSymbolViewerConfigPage::name() const { return i18n("Symbol Viewer"); }
QString KatePluginSymbolViewerConfigPage::fullName() const { return i18n("Symbol Viewer Configuration Page"); }
QIcon   KatePluginSymbolViewerConfigPage::icon() const { return QPixmap(( const char** ) class_xpm ); }

void KatePluginSymbolViewerConfigPage::apply()
{
  emit configPageApplyRequest( this );

}
// END KatePluginSymbolViewerConfigPage


#include "plugin_katesymbolviewer.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
