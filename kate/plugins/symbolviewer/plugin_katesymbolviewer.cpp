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
 *                      Added an entry in the m_popup menu to switch between List and Tree mode
 *                      Various bugfixing.
 *  Apr 24 2003 v 0.5 - Added three check buttons in m_popup menu to show/hide m_symbols
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
#include "plugin_katesymbolviewer.moc"

#include <kaction.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <kaboutdata.h>
#include <kfiledialog.h>
#include <ktoggleaction.h>
#include <kactioncollection.h>
#include <kiconloader.h>

#include <ktexteditor/configinterface.h>
#include <ktexteditor/cursor.h>

#include <QPixmap>
#include <QVBoxLayout>
#include <QGroupBox>

#include <QResizeEvent>
#include <QMenu>
#include <QPainter>

static const int s_lineWidth = 100;
static const int s_pixmapWidth = 40;

K_PLUGIN_FACTORY(KateSymbolViewerFactory, registerPlugin<KatePluginSymbolViewer>();)
K_EXPORT_PLUGIN(KateSymbolViewerFactory(KAboutData("katesymbolviewer","katesymbolviewer",ki18n("SymbolViewer"), "0.1", ki18n("View m_symbols"), KAboutData::License_LGPL_V2)) )


KatePluginSymbolViewerView::KatePluginSymbolViewerView(Kate::MainWindow *w, KatePluginSymbolViewer *plugin) :
Kate::PluginView(w),
Kate::XMLGUIClient(KateSymbolViewerFactory::componentData()),
m_plugin(plugin)
{
  KGlobal::locale()->insertCatalog("katesymbolviewer");

  w->guiFactory()->addClient (this);
  m_symbols = 0;

  m_popup = new QMenu(m_symbols);
  m_popup->insertItem(i18n("Refresh List"), this, SLOT(slotRefreshSymbol()));
  m_popup->addSeparator();
  m_macro = m_popup->insertItem(i18n("Show Macros"), this, SLOT(toggleShowMacros()));
  m_struct = m_popup->insertItem(i18n("Show Structures"), this, SLOT(toggleShowStructures()));
  m_func = m_popup->insertItem(i18n("Show Functions"), this, SLOT(toggleShowFunctions()));
  m_popup->addSeparator();
  m_popup->insertItem(i18n("List/Tree Mode"), this, SLOT(slotChangeMode()));
  m_sort = m_popup->insertItem(i18n("Enable Sorting"), this, SLOT(slotEnableSorting()));

  m_popup->setItemChecked(m_macro, true);
  m_popup->setItemChecked(m_struct, true);
  m_popup->setItemChecked(m_func, true);
  m_popup->setItemChecked(m_sort, false);
  macro_on = true;
  struct_on = true;
  func_on = true;
  treeMode = false;
  lsorting = false;

  m_updateTimer.setSingleShot(true);
  connect(&m_updateTimer, SIGNAL(timeout()), this, SLOT(updatePixmapEdit()));

  QPixmap cls( ( const char** ) class_xpm );

  m_toolview = mainWindow()->createToolView("kate_plugin_symbolviewer", Kate::MainWindow::Left, cls, i18n("Symbol List"));

  QWidget *container = new QWidget(m_toolview);
  QHBoxLayout *layout = new QHBoxLayout(container);

  m_label = new QLabel();
  m_pixmap = QPixmap(80, s_lineWidth);
  m_pixmap.fill();
  m_label->setPixmap(m_pixmap);
  m_label->setScaledContents(true);
  m_label->setMinimumWidth(s_pixmapWidth);
  m_label->setMaximumWidth(s_pixmapWidth);
  
  m_symbols = new QTreeWidget();
  layout->addWidget(m_label);
  layout->addWidget(m_symbols, 10);

  m_symbols->setLayoutDirection( Qt::LeftToRight );

  connect(m_symbols, SIGNAL(itemActivated(QTreeWidgetItem*,int)), this, SLOT(goToSymbol(QTreeWidgetItem*)));
  connect(m_symbols, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(slotShowContextMenu(QPoint)));
  
  connect(mainWindow(), SIGNAL(viewChanged()), this, SLOT(slotDocChanged()));
  
  QStringList titles;
  titles << i18nc("@title:column", "Symbols") << i18nc("@title:column", "Position");
  m_symbols->setColumnCount(2);
  m_symbols->setHeaderLabels(titles);
  
  m_symbols->setColumnHidden(1, true);
  m_symbols->setSortingEnabled(false);
  m_symbols->setRootIsDecorated(0);
  m_symbols->setContextMenuPolicy(Qt::CustomContextMenu);
  m_symbols->setIndentation(10);
  //m_symbols->setShowToolTips(true);

  m_toolview->installEventFilter(this);

  /* First Symbols parsing here...*/
  parseSymbols();
}

KatePluginSymbolViewerView::~KatePluginSymbolViewerView()
{
  mainWindow()->guiFactory()->removeClient (this);
  delete m_toolview;
  delete m_popup;
}

void KatePluginSymbolViewerView::toggleShowMacros(void)
{
 bool s = !m_popup->isItemChecked(m_macro);
 m_popup->setItemChecked(m_macro, s);
 macro_on = s;
 slotRefreshSymbol();
}

void KatePluginSymbolViewerView::toggleShowStructures(void)
{
 bool s = !m_popup->isItemChecked(m_struct);
 m_popup->setItemChecked(m_struct, s);
 struct_on = s;
 slotRefreshSymbol();
}

void KatePluginSymbolViewerView::toggleShowFunctions(void)
{
 bool s = !m_popup->isItemChecked(m_func);
 m_popup->setItemChecked(m_func, s);
 func_on = s;
 slotRefreshSymbol();
}

void KatePluginSymbolViewerView::slotRefreshSymbol()
{
  if (!m_symbols)
    return;
 m_symbols->clear();
 parseSymbols();
}

void KatePluginSymbolViewerView::slotChangeMode()
{
 treeMode = !treeMode;
 m_symbols->clear();
 parseSymbols();
}

void KatePluginSymbolViewerView::slotEnableSorting()
{
 lsorting = !lsorting;
 m_popup->setItemChecked(m_sort, lsorting);
 m_symbols->clear();
 if (lsorting == true)
     m_symbols->setSortingEnabled(true);
 else
     m_symbols->setSortingEnabled(false);

 parseSymbols();
 if (lsorting == true) m_symbols->sortItems(0, Qt::AscendingOrder);
}

void KatePluginSymbolViewerView::slotDocChanged()
{
 slotRefreshSymbol();

 m_visibleStart = -1;
 m_visibleLines = -1;
 KTextEditor::View *view = mainWindow()->activeView();
 //kDebug()<<"Document changed !!!!" << view;
 if (view) {
   connect(view, SIGNAL(verticalScrollPositionChanged(KTextEditor::View*,KTextEditor::Cursor)),
           this, SLOT(verticalScrollPositionChanged(KTextEditor::View*,KTextEditor::Cursor)), Qt::UniqueConnection);

   if (view->document()) {
     connect(view->document(), SIGNAL(textChanged(KTextEditor::Document*)), 
             this, SLOT(slotDocEdited()), Qt::UniqueConnection);
   }
   m_updateTimer.start(10);
 }
}

void KatePluginSymbolViewerView::slotViewChanged(QResizeEvent *)
{
  //kDebug(13000)<<"View changed !!!!";
  //m_symbols->setColumnWidth(0, m_symbols->parentWidget()->width());
  
}

void KatePluginSymbolViewerView::slotDocEdited()
{
  //kDebug() << "";
  m_updateTimer.start(500);
}

void KatePluginSymbolViewerView::verticalScrollPositionChanged(
  KTextEditor::View *view,
  const KTextEditor::Cursor &newPos)
{
  if (view != mainWindow()->activeView()) {
    return;
  }
  QFont f;
  KTextEditor::ConfigInterface* ciface = qobject_cast<KTextEditor::ConfigInterface*>(view);
  if (ciface) f = ciface->configValue("font").value<QFont>();
  m_visibleStart = newPos.line();
  m_visibleLines = mainWindow()->activeView()->height() / (QFontMetrics(f).height());
  updatePixmapScroll();
}


void KatePluginSymbolViewerView::updatePixmapEdit()
{
  if (!mainWindow()) {
    return;
  }
  KTextEditor::View* editView = mainWindow()->activeView();
  if (!editView) {
    return;
  }
  KTextEditor::Document* doc = editView->document();
  if (!doc) {
    return;
  }
  int docLines = qMax(doc->lines(), 100);
  int labelHeight = m_label->height();
  int numJumpLines = 1;
  if ((labelHeight > 10) && (docLines > labelHeight*2)) {
    numJumpLines = docLines / labelHeight;
  }
  docLines /= numJumpLines;

  //kDebug() << labelHeight << doc->lines() << docLines << numJumpLines;

  m_pixmap = QPixmap(s_lineWidth, docLines+1);
  m_pixmap.fill();

  QString line;
  int pixX;
  QPainter p;
  if (p.begin(&m_pixmap)) {
    p.setPen(Qt::black);
    for (int y=0; y < doc->lines(); y+=numJumpLines) {
      line = doc->line(y);
      pixX=0;
      for (int x=0; x <line.size(); x++) {
        if (pixX >= s_lineWidth) {
          break;
        }
        if (line[x] == ' ') {
          pixX++;
        }
        else if (line[x] == '\t') {
          pixX += 4; // FIXME: tab width...
        }
        else if (line[x] != '\r') {
          p.drawPoint(pixX, y/numJumpLines);
          pixX++;
        }
      }
    }
    p.end();
  }

  QFont f;
  KTextEditor::ConfigInterface* ciface = qobject_cast<KTextEditor::ConfigInterface*>(editView);
  if (ciface) f = ciface->configValue("font").value<QFont>();
  m_visibleStart = editView->cursorPositionVirtual().line();
  m_visibleStart -= editView->cursorPositionCoordinates().y() / (QFontMetrics(f).height());
  m_visibleLines = editView->height() / (QFontMetrics(f).height());

  updatePixmapScroll();
}

void KatePluginSymbolViewerView::updatePixmapScroll()
{
  if (!mainWindow()) {
    return;
  }
  KTextEditor::View* editView = mainWindow()->activeView();
  if (!editView) {
    return;
  }
  KTextEditor::Document* doc = editView->document();
  if (!doc) {
    return;
  }
  
  int docLines = qMax(doc->lines(), 100);
  int labelHeight = m_label->height();
  int numJumpLines = 1;
  if ((labelHeight > 10) && (docLines > labelHeight*2)) {
    numJumpLines = docLines / labelHeight;
  }
  docLines /= numJumpLines;
  
  //kDebug() << labelHeight << doc->lines() << docLines << numJumpLines;
  
  QPixmap pixmap = m_pixmap;
  
  QPainter p;
  if (p.begin(&pixmap)) {
    if ((m_visibleStart > -1) && (m_visibleLines > 0)) {
      p.setBrush(QColor(50,50,255, 100));
      p.setPen(QColor(20,20,255, 127));
      p.drawRect(0, m_visibleStart/numJumpLines, s_lineWidth, m_visibleLines/numJumpLines);
    }
    p.end();
  }
  //kDebug() << editView->visibleRange().start() << editView->visibleRange().end();
  m_label->setPixmap(pixmap);
  m_label->setScaledContents(true);
}

bool KatePluginSymbolViewerView::eventFilter(QObject *obj, QEvent *event)
{
  if (event->type() == QEvent::KeyPress) {
    QKeyEvent *ke = static_cast<QKeyEvent*>(event);
    if ((obj == m_toolview) && (ke->key() == Qt::Key_Escape)) {
      mainWindow()->activeView()->setFocus();
      event->accept();
      return true;
    }
  }
  else if (event->type() == QEvent::MouseButtonPress) {
    QMouseEvent *me = static_cast<QMouseEvent*>(event);
    if ((me->button() == Qt::LeftButton) || (me->button() == Qt::MiddleButton)) {
      int line = (me->y() * mainWindow()->activeView()->document()->lines()) / m_label->height();
      mainWindow()->activeView()->setCursorPosition(KTextEditor::Cursor(line, 0));
    }
  }
  
// This does not work for some reason...
//   else if (event->type() == QEvent::Wheel) {
//     QWheelEvent *we = static_cast<QWheelEvent*>(event);
//     QWheelEvent *we2 = new QWheelEvent(QPoint(50, 5), we->delta(), we->buttons(), we->modifiers(), we->orientation());
//     QApplication::postEvent(mainWindow()->activeView(), we2);
//   }
  
  return QObject::eventFilter(obj, event);
}

void KatePluginSymbolViewerView::slotShowContextMenu(const QPoint &p)
{
 m_popup->popup(m_symbols->mapToGlobal(p));
}

void KatePluginSymbolViewerView::parseSymbols(void)
{
  if (!mainWindow()->activeView())
    return;

  KTextEditor::Document *doc = mainWindow()->activeView()->document();

  // be sure we have some document around !
  if (!doc)
    return;

  /** Get the current highlighting mode */
  QString hlModeName = doc->mode();

  if (hlModeName == "C++" || hlModeName == "C" || hlModeName == "ANSI C89")
     parseCppSymbols();
 else if (hlModeName == "PHP (HTML)")
    parsePhpSymbols();
  else if (hlModeName == "Tcl/Tk")
     parseTclSymbols();
  else if (hlModeName == "Fortran")
     parseFortranSymbols();
  else if (hlModeName == "Perl")
     parsePerlSymbols();
  else if (hlModeName == "Python")
     parsePythonSymbols();
 else if (hlModeName == "Ruby")
    parseRubySymbols();
  else if (hlModeName == "Java")
     parseCppSymbols();
  else if (hlModeName == "xslt")
     parseXsltSymbols();
  else if (hlModeName == "Bash")
     parseBashSymbols();
  else if (hlModeName == "ActionScript 2.0" || 
           hlModeName == "JavaScript")
     parseEcmaSymbols();
  else
    new QTreeWidgetItem(m_symbols,  QStringList(i18n("Sorry. Language not supported yet") ) );
}

void KatePluginSymbolViewerView::goToSymbol(QTreeWidgetItem *it)
{
  KTextEditor::View *kv = mainWindow()->activeView();

  // be sure we really have a view !
  if (!kv)
    return;

  kDebug(13000)<<"Slot Activated at pos: "<<m_symbols->indexOfTopLevelItem(it);

  kv->setCursorPosition (KTextEditor::Cursor (it->text(1).toInt(NULL, 10), 0));
}

KatePluginSymbolViewer::KatePluginSymbolViewer( QObject* parent, const QList<QVariant>& )
    : Kate::Plugin ( (Kate::Application*)parent, "katesymbolviewerplugin" )
{
 kDebug(13000)<<"KatePluginSymbolViewer";
}

KatePluginSymbolViewer::~KatePluginSymbolViewer()
{
  kDebug(13000)<<"~KatePluginSymbolViewer";
}

Kate::PluginView *KatePluginSymbolViewer::createView (Kate::MainWindow *mainWindow)
{
  return new KatePluginSymbolViewerView (mainWindow, this);
}

Kate::PluginConfigPage* KatePluginSymbolViewer::configPage(
    uint, QWidget *w, const char* /*name*/)
{
  KatePluginSymbolViewerConfigPage* p = new KatePluginSymbolViewerConfigPage(this, w);

  KConfigGroup config(KGlobal::config(), "PluginSymbolViewer"); 
  p->viewReturns->setChecked(config.readEntry("ViewTypes", false));
  p->expandTree->setChecked(config.readEntry("ExpandTree", false));
  connect( p, SIGNAL(configPageApplyRequest(KatePluginSymbolViewerConfigPage*)),
      SLOT(applyConfig(KatePluginSymbolViewerConfigPage*)) );
  return (Kate::PluginConfigPage*)p;
}

KIcon KatePluginSymbolViewer::configPageIcon (uint number) const
{
  QPixmap cls( ( const char** ) class_xpm );
  if (number != 0) return KIcon();
  return KIcon(cls);
}

void KatePluginSymbolViewer::applyConfig( KatePluginSymbolViewerConfigPage* p )
{
  KConfigGroup config(KGlobal::config(), "PluginSymbolViewer");
  config.writeEntry("ViewTypes", p->viewReturns->isChecked());
  config.writeEntry("ExpandTree", p->expandTree->isChecked());

  types_on = KConfigGroup(KGlobal::config(), "PluginSymbolViewer").readEntry("ViewTypes", false);
  expanded_on = KConfigGroup(KGlobal::config(), "PluginSymbolViewer").readEntry("ExpandTree", false);
}

// BEGIN KatePluginSymbolViewerConfigPage
KatePluginSymbolViewerConfigPage::KatePluginSymbolViewerConfigPage(
    QObject* /*parent*/ /*= 0L*/, QWidget *parentWidget /*= 0L*/)
  : Kate::PluginConfigPage( parentWidget )
{
  QVBoxLayout *lo = new QVBoxLayout( this );
  int spacing = KDialog::spacingHint();
  lo->setSpacing( spacing );

  QGroupBox* groupBox = new QGroupBox( i18n("Parser Options"), this);

  viewReturns = new QCheckBox(i18n("Display functions parameters"));
  expandTree = new QCheckBox(i18n("Automatically expand nodes in tree mode"));

  QVBoxLayout* top = new QVBoxLayout();
  top->addWidget(viewReturns);
  top->addWidget(expandTree);
  groupBox->setLayout(top);
  lo->addWidget( groupBox );
  lo->addStretch( 1 );


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

