/* This file is part of the KDE libraries
   Copyright (C) 2005-2006 Hamish Rodda <rodda@kde.org>

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

#include "katecompletionwidget.h"

#include <QtGui/QBoxLayout>
#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>
#include <QtGui/QHeaderView>
#include <QtCore/QTimer>
#include <QtGui/QLabel>
#include <QtGui/QToolButton>
#include <QtGui/QSizeGrip>
#include <QtGui/QPushButton>
#include <QtGui/QAbstractScrollArea>
#include <QtGui/QScrollBar>

#include <kicon.h>
#include <kdialog.h>

#include <ktexteditor/cursorfeedback.h>

#include "kateview.h"
#include "katesmartmanager.h"
#include "katerenderer.h"
#include "kateconfig.h"
#include "katedocument.h"
#include "katesmartrange.h"
#include "kateedit.h"

#include "katecompletionmodel.h"
#include "katecompletiontree.h"
#include "katecompletionconfig.h"
#include "kateargumenthinttree.h"
#include "kateargumenthintmodel.h"

//#include "modeltest.h"

KateCompletionWidget::KateCompletionWidget(KateView* parent)
  : QFrame(parent, Qt::Tool | Qt::FramelessWindowHint)
  , m_presentationModel(new KateCompletionModel(this))
  , m_completionRange(0L)
  , m_entryList(new KateCompletionTree(this))
  , m_argumentHintModel(new KateArgumentHintModel(this))
  , m_argumentHintTree(new KateArgumentHintTree(this))
  , m_automaticInvocation(false)
  , m_filterInstalled(false)
  , m_inCompletionList(false)
  , m_isSuspended(false)
  , m_dontShowArgumentHints(false)
{
  //new ModelTest(m_presentationModel, this);

  setFrameStyle( QFrame::Box | QFrame::Plain );
  setLineWidth( 1 );
  //setWindowOpacity(0.8);

  m_entryList->setModel(m_presentationModel);
  m_argumentHintTree->setModel(m_argumentHintModel);

  m_statusBar = new QWidget(this);
  m_statusBar->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Fixed );

  m_sortButton = new QToolButton(m_statusBar);
  m_sortButton->setIcon(KIcon("sort"));
  m_sortButton->setCheckable(true);
  connect(m_sortButton, SIGNAL(toggled(bool)), m_presentationModel, SLOT(setSortingEnabled(bool)));

  m_sortText = new QLabel(i18n("Sort: None"), m_statusBar);

  m_filterButton = new QToolButton(m_statusBar);
  m_filterButton->setIcon(KIcon("search-filter"));
  m_filterButton->setCheckable(true);
  connect(m_filterButton, SIGNAL(toggled(bool)), m_presentationModel, SLOT(setFilteringEnabled(bool)));

  m_filterText = new QLabel(i18n("Filter: None"), m_statusBar);

  m_configButton = new QPushButton(KIcon("configure"), i18n("Setup"), m_statusBar);
  connect(m_configButton, SIGNAL(pressed()), SLOT(showConfig()));

  QSizeGrip* sg = new QSizeGrip(m_statusBar);

  QHBoxLayout* statusLayout = new QHBoxLayout(m_statusBar);
  statusLayout->addWidget(m_sortButton);
  statusLayout->addWidget(m_sortText);
  statusLayout->addSpacing(8);
  statusLayout->addWidget(m_filterButton);
  statusLayout->addWidget(m_filterText);
  statusLayout->addSpacing(8);
  statusLayout->addStretch();
  QVBoxLayout* gripLayout = new QVBoxLayout();
  gripLayout->addStretch();
  statusLayout->addWidget(m_configButton);
  gripLayout->addWidget(sg);
  statusLayout->addLayout(gripLayout);
  statusLayout->setMargin(0);
  statusLayout->setSpacing(2);

  QVBoxLayout* vl = new QVBoxLayout(this);
  vl->addWidget(m_entryList);
  vl->addWidget(m_statusBar);
  vl->setMargin(0);
  
  // Keep branches expanded
  connect(m_presentationModel, SIGNAL(modelReset()), this, SLOT(modelReset()), Qt::QueuedConnection);
  connect(m_presentationModel, SIGNAL(rowsInserted(const QModelIndex&, int, int)), SLOT(rowsInserted(const QModelIndex&, int, int)));

  connect(view(), SIGNAL(cursorPositionChanged(KTextEditor::View*, const KTextEditor::Cursor&)), SLOT(cursorPositionChanged()));
  connect(view()->doc()->history(), SIGNAL(editDone(KateEditInfo*)), SLOT(editDone(KateEditInfo*)), Qt::QueuedConnection);
  connect(view(), SIGNAL(focusOut(KTextEditor::View*)), this, SLOT(focusOut()));
  connect(view(), SIGNAL(focusIn(KTextEditor::View*)), this, SLOT(focusIn()));

  // This is a non-focus widget, it is passed keyboard input from the view
  setFocusPolicy(Qt::NoFocus);
  foreach (QWidget* childWidget, findChildren<QWidget*>())
    childWidget->setFocusPolicy(Qt::NoFocus);
}

KateCompletionWidget::~KateCompletionWidget() {
}

void KateCompletionWidget::focusOut() {
  if( isCompletionActive() ) {
    hide();
    m_isSuspended = true;
  }
}

void KateCompletionWidget::focusIn() {
  if( m_isSuspended ) {
    show();
    m_isSuspended = false;
  }
}

KateArgumentHintTree* KateCompletionWidget::argumentHintTree() const {
  return m_argumentHintTree;
}

KateArgumentHintModel* KateCompletionWidget::argumentHintModel() const {
  return m_argumentHintModel;
}

const KateCompletionModel* KateCompletionWidget::model() const {
  return m_presentationModel;
}

KateCompletionModel* KateCompletionWidget::model() {
  return m_presentationModel;
}

void KateCompletionWidget::rowsInserted(const QModelIndex& parent, int rowFrom, int rowEnd)
{
  for (int i = rowFrom; i <= rowEnd; ++i)
    m_entryList->expand(m_presentationModel->index(i, 0, parent));
}

KateView * KateCompletionWidget::view( ) const
{
  return static_cast<KateView*>(const_cast<QObject*>(parent()));
}

void KateCompletionWidget::startCompletion( const KTextEditor::Range & word, KTextEditor::CodeCompletionModel * model, KTextEditor::CodeCompletionModel::InvocationType invocationType)
{
  m_isSuspended = false;
  
  if (!word.isValid()) {
    kWarning(13035) << k_funcinfo << "Invalid range given to start code completion!";
    return;
  }

  kDebug(13035) << k_funcinfo << word << " " << model;

  if (!m_filterInstalled) {
    if (!QApplication::activeWindow()) {
      kWarning(13035) << k_funcinfo << "No active window to install event filter on!!";
      return;
    }
    // Enable the cc box to move when the editor window is moved
    QApplication::activeWindow()->installEventFilter(this);
    m_filterInstalled = true;
  }

  if (isCompletionActive())
    abortCompletion();

  m_completionRange = view()->doc()->smartManager()->newSmartRange(word);
  m_completionRange->setInsertBehavior(KTextEditor::SmartRange::ExpandRight);
  if(!m_completionRange->isValid()) {
    kWarning(13035) << k_funcinfo << "Could not construct valid smart-range from " << word;
    abortCompletion();
    return;
  }

  connect(m_completionRange->smartStart().notifier(), SIGNAL(characterDeleted(KTextEditor::SmartCursor*, bool)), SLOT(startCharacterDeleted(KTextEditor::SmartCursor*, bool)));

  cursorPositionChanged();

  if (model)
    m_presentationModel->setCompletionModel(model);
  else
    m_presentationModel->setCompletionModels(m_sourceModels);

  updatePosition(true);

  if (!m_presentationModel->completionModels().isEmpty()) {
    m_dontShowArgumentHints = true;
    
    show();
    
    foreach (KTextEditor::CodeCompletionModel* model, m_presentationModel->completionModels()) {
      model->completionInvoked(view(), word, invocationType);
      kDebug(13035) << "CC Model Statistics: " << model << endl << "  Completions: " << model->rowCount(QModelIndex());
    }

    m_entryList->resizeColumns(false, true);

    m_presentationModel->setCurrentCompletion(view()->doc()->text(KTextEditor::Range(m_completionRange->start(), view()->cursorPosition())));

    m_dontShowArgumentHints = false;
    m_argumentHintModel->buildRows();
    m_argumentHintTree->updateGeometry();
    
    m_argumentHintTree->show();
  }
}

void KateCompletionWidget::updatePosition(bool force)
{
  if (!force && !isCompletionActive())
    return;

  QPoint cursorPosition = view()->cursorToCoordinate(m_completionRange->start());
  if (cursorPosition == QPoint(-1,-1))
    // Start of completion range is now off-screen -> abort
    return abortCompletion();

  QPoint p = view()->mapToGlobal( cursorPosition );
  int x = p.x() - m_entryList->header()->sectionPosition(m_entryList->header()->visualIndex(m_presentationModel->translateColumn(KTextEditor::CodeCompletionModel::Name))) - 2;
  int y = p.y();
  if ( y + height() + view()->renderer()->config()->fontMetrics().height() > QApplication::desktop()->height() )
    y -= height();
  else
    y += view()->renderer()->config()->fontMetrics().height();

  if (x + width() > QApplication::desktop()->width())
    x = QApplication::desktop()->width() - width();

  move( QPoint(x,y) );

  //Now place the argument-hint widget
  QRect geom = m_argumentHintTree->geometry();
  geom.moveTo(QPoint(x,y));
  geom.setWidth(width());
  geom.moveBottom(y - 50);
  m_argumentHintTree->updateGeometry(geom);
}

void KateCompletionWidget::cursorPositionChanged( )
{
  if (!isCompletionActive())  {
    m_presentationModel->setCurrentCompletion(QString());
    return;
  }

  KTextEditor::Cursor cursor = view()->cursorPosition();

  if (m_completionRange->start() > cursor || m_completionRange->end() < cursor)
    return abortCompletion();

  QString currentCompletion = view()->doc()->text(KTextEditor::Range(m_completionRange->start(), view()->cursorPosition()));

  // FIXME - allow client to specify this?
  static QRegExp allowedText("^(\\w*)");
  if (!allowedText.exactMatch(currentCompletion))
    return abortCompletion();

  m_presentationModel->setCurrentCompletion(currentCompletion);
}

bool KateCompletionWidget::isCompletionActive( ) const
{
  bool ret = !isHidden();
  if (ret)
    Q_ASSERT(m_completionRange && m_completionRange->isValid());

  return ret;
}

void KateCompletionWidget::abortCompletion( )
{
  kDebug(13035) << k_funcinfo;

  m_isSuspended = false;
  
  if (!isCompletionActive())
    return;

  hide();

  clear();

  delete m_completionRange;
  m_completionRange = 0L;

  view()->sendCompletionAborted();
}

void KateCompletionWidget::clear() {
  m_presentationModel->clearCompletionModels();
  m_argumentHintTree->clearCompletion();
  m_argumentHintModel->clear();
}

void KateCompletionWidget::execute(bool shift)
{
  kDebug(13035) << k_funcinfo;

  if (!isCompletionActive())
    return;

  if( shift ) {
    QModelIndex index = selectedIndex();
    
    if( index.isValid() )
      index.data(KTextEditor::CodeCompletionModel::AccessibilityAccept);
    
    return;
  }
  
  QModelIndex toExecute = m_entryList->selectionModel()->currentIndex();

  if (!toExecute.isValid())
    return abortCompletion();

  toExecute = m_presentationModel->mapToSource(toExecute);
  KTextEditor::Cursor start = m_completionRange->start();

  // encapsulate all editing as being from the code completion, and undo-able in one step.
  view()->doc()->editStart(true, Kate::CodeCompletionEdit);

  KTextEditor::CodeCompletionModel* model = static_cast<KTextEditor::CodeCompletionModel*>(const_cast<QAbstractItemModel*>(toExecute.model()));
  model->executeCompletionItem(view()->document(), *m_completionRange, toExecute.row());

  view()->doc()->editEnd();

  hide();

  view()->sendCompletionExecuted(start, model, toExecute.row());
}

void KateCompletionWidget::resizeEvent( QResizeEvent * event )
{
  QWidget::resizeEvent(event);

  m_entryList->resizeColumns(true);
}

void KateCompletionWidget::showEvent ( QShowEvent * event )
{
  m_isSuspended = false;
  
  QWidget::showEvent(event);

  if( !m_dontShowArgumentHints )
    m_argumentHintTree->show();
}

void KateCompletionWidget::hideEvent( QHideEvent * event )
{
  QWidget::hideEvent(event);

  m_argumentHintTree->hide();
  
  if (isCompletionActive())
    abortCompletion();
}

KateSmartRange * KateCompletionWidget::completionRange( ) const
{
  return m_completionRange;
}

void KateCompletionWidget::modelReset( )
{
  m_argumentHintTree->expandAll();
  m_entryList->expandAll();
}

KateCompletionTree* KateCompletionWidget::treeView() const {
  return m_entryList;
}

QModelIndex KateCompletionWidget::selectedIndex() const {
  if( m_inCompletionList )
    return m_entryList->currentIndex();
  else
    return m_argumentHintTree->currentIndex();
}

bool KateCompletionWidget::cursorLeft( bool shift ) {
  if( shift ) {
    QModelIndex index = selectedIndex();
    
    if( index.isValid() )
      index.data(KTextEditor::CodeCompletionModel::AccessibilityPrevious);
    
    return true;
  }
  
  if (canCollapseCurrentItem() ) {
    setCurrentItemExpanded(false);
    return true;
  }
  return false;
}

bool KateCompletionWidget::cursorRight( bool shift ) {
  if( shift ) {
    QModelIndex index = selectedIndex();
    
    if( index.isValid() )
      index.data(KTextEditor::CodeCompletionModel::AccessibilityNext);
    
    return true;
  }
  
  if ( canExpandCurrentItem() ) {
    setCurrentItemExpanded(true);
    return true;
  }
  return false;
}

bool KateCompletionWidget::canExpandCurrentItem() const {
  if( m_inCompletionList ) {
    if( !m_entryList->currentIndex().isValid() ) return false;
    return model()->isExpandable( m_entryList->currentIndex() ) && !model()->isExpanded( m_entryList->currentIndex() );
  } else {
    if( !m_argumentHintTree->currentIndex().isValid() ) return false;
    return argumentHintModel()->isExpandable( m_argumentHintTree->currentIndex() ) && !argumentHintModel()->isExpanded( m_argumentHintTree->currentIndex() );
  }
}

bool KateCompletionWidget::canCollapseCurrentItem() const {
  if( m_inCompletionList ) {
    if( !m_entryList->currentIndex().isValid() ) return false;
    return model()->isExpandable( m_entryList->currentIndex() ) && model()->isExpanded( m_entryList->currentIndex() );
  }else{
    if( !m_argumentHintTree->currentIndex().isValid() ) return false;
    return m_argumentHintModel->isExpandable( m_argumentHintTree->currentIndex() ) && m_argumentHintModel->isExpanded( m_argumentHintTree->currentIndex() );
  }
}

void KateCompletionWidget::setCurrentItemExpanded( bool expanded ) {
  if( m_inCompletionList ) {
    if( !m_entryList->currentIndex().isValid() ) return;
    model()->setExpanded(m_entryList->currentIndex(), expanded);
  }else{
    if( !m_argumentHintTree->currentIndex().isValid() ) return;
    m_argumentHintModel->setExpanded(m_argumentHintTree->currentIndex(), expanded);
  }
}

void KateCompletionWidget::startCharacterDeleted( KTextEditor::SmartCursor*, bool deletedBefore )
{
  if (deletedBefore)
    // Cannot abort completion from this function, or the cursor will be deleted before returning
    QTimer::singleShot(0, this, SLOT(abortCompletion()));
}

bool KateCompletionWidget::eventFilter( QObject * watched, QEvent * event )
{
  bool ret = QFrame::eventFilter(watched, event);

  if (watched != this)
    if (event->type() == QEvent::Move)
      updatePosition();

  return ret;
}

void KateCompletionWidget::cursorDown( bool shift )
{
  if( shift ) {
    QModelIndex index = selectedIndex();
    if( dynamic_cast<const ExpandingWidgetModel*>(index.model()) ) {
      const ExpandingWidgetModel* model = static_cast<const ExpandingWidgetModel*>(index.model());
      if( model->isExpanded(index) ) {
        QWidget* widget = model->expandingWidget(index);
        QAbstractScrollArea* area = qobject_cast<QAbstractScrollArea*>(widget);
        if( area && area->verticalScrollBar() )
          area->verticalScrollBar()->setSliderPosition( area->verticalScrollBar()->sliderPosition() + area->verticalScrollBar()->singleStep() );
      }
    }
    return;
  }
  
  if( m_inCompletionList )
    m_entryList->nextCompletion();
  else {
    if( !m_argumentHintTree->nextCompletion() )
      switchList();
  }
}

void KateCompletionWidget::cursorUp( bool shift )
{
  if( shift ) {
    QModelIndex index = selectedIndex();
    if( dynamic_cast<const ExpandingWidgetModel*>(index.model()) ) {
      const ExpandingWidgetModel* model = static_cast<const ExpandingWidgetModel*>(index.model());
      if( model->isExpanded(index) ) {
        QWidget* widget = model->expandingWidget(index);
        QAbstractScrollArea* area = qobject_cast<QAbstractScrollArea*>(widget);
        if( area && area->verticalScrollBar() )
          area->verticalScrollBar()->setSliderPosition( area->verticalScrollBar()->sliderPosition() - area->verticalScrollBar()->singleStep() );
      }
    }
    return;
  }
  
  if( m_inCompletionList ) {
    if( !m_entryList->previousCompletion() )
      switchList();
  }else{
    m_argumentHintTree->previousCompletion();
  }
}

void KateCompletionWidget::pageDown( )
{
  if( m_inCompletionList )
    m_entryList->pageDown();
  else {
    if( !m_argumentHintTree->pageDown() )
      switchList();
  }
}

void KateCompletionWidget::pageUp( )
{
  if( m_inCompletionList ) {
    if( !m_entryList->pageUp() )
      switchList();
  }else{
    m_argumentHintTree->pageUp();
  }
}

void KateCompletionWidget::top( )
{
  if( m_inCompletionList )
    m_entryList->top();
  else
    m_argumentHintTree->top();
}

void KateCompletionWidget::bottom( )
{
  if( m_inCompletionList )
    m_entryList->bottom();
  else
    m_argumentHintTree->bottom();
}

void KateCompletionWidget::switchList() {
  if( m_inCompletionList ) {
      m_entryList->setCurrentIndex(QModelIndex());
      if( m_argumentHintModel->rowCount(QModelIndex()) != 0 )
        m_argumentHintTree->setCurrentIndex(m_argumentHintModel->index(m_argumentHintModel->rowCount(QModelIndex())-1, 0));
  } else {
      m_argumentHintTree->setCurrentIndex(QModelIndex());
      if( m_presentationModel->rowCount(QModelIndex()) != 0 )
        m_entryList->setCurrentIndex(m_presentationModel->index(0, 0));
  }
  m_inCompletionList = !m_inCompletionList;
}

void KateCompletionWidget::showConfig( )
{
  abortCompletion();

  KateCompletionConfig* config = new KateCompletionConfig(m_presentationModel, view());
  config->exec();
  delete config;
}

void KateCompletionWidget::registerCompletionModel(KTextEditor::CodeCompletionModel* model)
{
  m_sourceModels.append(model);

  if (isCompletionActive()) {
    m_presentationModel->addCompletionModel(model);
  }
}

void KateCompletionWidget::unregisterCompletionModel(KTextEditor::CodeCompletionModel* model)
{
  m_sourceModels.removeAll(model);
}

bool KateCompletionWidget::isAutomaticInvocationEnabled() const
{
  return m_automaticInvocation;
}

void KateCompletionWidget::setAutomaticInvocationEnabled(bool enabled)
{
  m_automaticInvocation = enabled;
}

void KateCompletionWidget::editDone(KateEditInfo * edit)
{
  if (isCompletionActive())
    return;

  if (!isAutomaticInvocationEnabled())
    return;

  if (edit->editSource() != Kate::UserInputEdit)
    return;

  if (edit->isRemoval())
    return;

  if (edit->newText().isEmpty())
    return;

  QString lastLine = edit->newText().last();

  if (lastLine.isEmpty())
    return;

  QChar lastChar = lastLine.at(lastLine.count() - 1);

  if (lastChar.isLetter() || lastChar.isNumber() || lastChar == '.' || lastChar == '_' || lastChar == '>') {
    // Start automatic code completion
    KTextEditor::Range range = determineRange();
    if (range.isValid())
      startCompletion(range, 0, KTextEditor::CodeCompletionModel::AutomaticInvocation);
    else
      kWarning(13035) << k_funcinfo << "Completion range was invalid even though it was expected to be valid.";
  }
}

void KateCompletionWidget::userInvokedCompletion()
{
  startCompletion(determineRange(), 0, KTextEditor::CodeCompletionModel::UserInvocation);
}

KTextEditor::Range KateCompletionWidget::determineRange() const
{
  KTextEditor::Cursor end = view()->cursorPosition();

  // the end cursor should always be valid, otherwise things go wrong
  // Assumption: view()->cursorPosition() is always valid.
  Q_ASSERT(end.isValid());

  QString text = view()->document()->line(end.line());

  static QRegExp findWordStart( "\\b([_\\w]+)$" );
  static QRegExp findWordEnd( "^([_\\w]*)\\b" );

  KTextEditor::Cursor start = end;

  if (findWordStart.lastIndexIn(text.left(end.column())) >= 0)
    start.setColumn(findWordStart.pos(1));

  if (findWordEnd.indexIn(text.mid(end.column())) >= 0)
    end.setColumn(end.column() + findWordEnd.cap(1).length());

  return KTextEditor::Range(start, end);
}

#include "katecompletionwidget.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
