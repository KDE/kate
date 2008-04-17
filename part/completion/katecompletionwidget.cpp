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
  , m_automaticInvocation(true)
  , m_automaticInvocationDelay(300)
  , m_filterInstalled(false)
  , m_configWidget(new KateCompletionConfig(m_presentationModel, view()))
  , m_inCompletionList(false)
  , m_isSuspended(false)
  , m_dontShowArgumentHints(false)
  , m_expandedAddedHeightBase(0)
  , m_expandingAddedHeight(0)
{
  //new ModelTest(m_presentationModel, this);

  setFrameStyle( QFrame::Box | QFrame::Plain );
  setLineWidth( 1 );
  //setWindowOpacity(0.8);

  m_entryList->setModel(m_presentationModel);
  m_argumentHintTree->setModel(m_argumentHintModel);

  m_automaticInvocationTimer = new QTimer(this);
  m_automaticInvocationTimer->setSingleShot(true);
  connect(m_automaticInvocationTimer, SIGNAL(timeout()), this, SLOT(automaticInvocation()));

  m_updateFocusTimer = new QTimer(this);
  m_updateFocusTimer->setSingleShot(true);
  connect(m_updateFocusTimer, SIGNAL(timeout()), this, SLOT(updateFocus()));

  QVBoxLayout* vl = new QVBoxLayout(this);
  vl->addWidget(m_entryList);
  vl->setMargin(0);

  // Keep branches expanded
  connect(m_presentationModel, SIGNAL(modelReset()), this, SLOT(modelReset()), Qt::QueuedConnection);
  connect(m_presentationModel, SIGNAL(rowsInserted(const QModelIndex&, int, int)), SLOT(rowsInserted(const QModelIndex&, int, int)));
  connect(m_argumentHintModel, SIGNAL(contentStateChanged(bool)), this, SLOT(argumentHintsChanged(bool)));

  connect(view(), SIGNAL(cursorPositionChanged(KTextEditor::View*, const KTextEditor::Cursor&)), SLOT(cursorPositionChanged()));
  connect(view()->doc()->history(), SIGNAL(editDone(KateEditInfo*)), SLOT(editDone(KateEditInfo*)), Qt::QueuedConnection);
  connect(view(), SIGNAL(focusOut(KTextEditor::View*)), this, SLOT(viewFocusOut()));
  connect(view(), SIGNAL(focusIn(KTextEditor::View*)), this, SLOT(viewFocusIn()));
  connect(view(), SIGNAL(verticalScrollPositionChanged (KTextEditor::View*, const KTextEditor::Cursor&)), this, SLOT(updatePositionSlot()));

  // This is a non-focus widget, it is passed keyboard input from the view

  //We need to do this, because else the focus goes to nirvana without any control when the completion-widget is clicked.
  setFocusPolicy(Qt::ClickFocus);

  foreach (QWidget* childWidget, findChildren<QWidget*>())
    childWidget->setFocusPolicy(Qt::NoFocus);
}

KateCompletionWidget::~KateCompletionWidget() {
}

void KateCompletionWidget::modelContentChanged() {
  int realItemCount = 0;
  foreach (KTextEditor::CodeCompletionModel* model, m_presentationModel->completionModels())
    realItemCount += model->rowCount();
  if( !m_isSuspended && !isVisible() && realItemCount != 0 ) {
    kDebug() << "showing";
    updateAndShow();
  }
  //With each filtering items can be added or removed, so we have to reset the current index here so we always have a selected item
  m_entryList->setCurrentIndex(model()->index(0,0));
  if(!model()->indexIsItem(m_entryList->currentIndex()))
    m_entryList->setCurrentIndex(model()->index(0,0, m_entryList->currentIndex()));
}

void KateCompletionWidget::focusInEvent ( QFocusEvent * event ) {
  //Instantly give the focus back
  view()->activateWindow(); //When the completion-widget is clicked, the application-window is deactivated, so re-activate it
  view()->setFocus(Qt::OtherFocusReason);
  event->accept();
  QFrame::focusInEvent(event);
}

void KateCompletionWidget::updateFocus() {
  if( isCompletionActive() ) {
    bool childHasFocus = QApplication::focusWidget() ? isAncestorOf(QApplication::focusWidget()) : false;
    if(!childHasFocus && !view()->hasFocus()) {
      if(isVisible()) {
        hide();
        m_isSuspended = true;
      }
    }else if(childHasFocus) {
      //Whenever the completion-widget gets the focus by clicking, move it back to the text-view.
      view()->activateWindow(); //When the completion-widget is clicked, the application-window is deactivated, so re-activate it
      view()->setFocus(Qt::OtherFocusReason);
    }
  }
}

void KateCompletionWidget::viewFocusOut() {
    //Check whether the view is still visible in a short time. If it isn't, hide the completion-widget.
    m_updateFocusTimer->start(100);
}

void KateCompletionWidget::viewFocusIn() {
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
  if (!parent.isValid())
    for (int i = rowFrom; i <= rowEnd; ++i)
      m_entryList->expand(m_presentationModel->index(i, 0, parent));
}

KateView * KateCompletionWidget::view( ) const
{
  return static_cast<KateView*>(const_cast<QObject*>(parent()));
}

void KateCompletionWidget::argumentHintsChanged(bool hasContent)
{
  m_dontShowArgumentHints = !hasContent;
  
  if( m_dontShowArgumentHints )
    m_argumentHintTree->hide();
  else
    updateArgumentHintGeometry();
}

void KateCompletionWidget::startCompletion( const KTextEditor::Range & word, KTextEditor::CodeCompletionModel * model, KTextEditor::CodeCompletionModel::InvocationType invocationType)
{
  m_isSuspended = false;
  m_inCompletionList = true; //Always start at the top of the completion-list
  
  disconnect(this->model(), SIGNAL(contentGeometryChanged()), this, SLOT(modelContentChanged()));

  m_dontShowArgumentHints = true;

  if (!word.isValid()) {
    kWarning(13035) << "Invalid range given to start code completion!";
    return;
  }

  kDebug(13035) << word << " " << model;

  if (!m_filterInstalled) {
    if (!QApplication::activeWindow()) {
      kWarning(13035) << "No active window to install event filter on!!";
      return;
    }
    // Enable the cc box to move when the editor window is moved
    QApplication::activeWindow()->installEventFilter(this);
    m_filterInstalled = true;
  }

  if (isCompletionActive())
    abortCompletion();

  m_completionRange = view()->doc()->smartManager()->newSmartRange(word);
  m_completionRange->setInsertBehavior(KTextEditor::SmartRange::ExpandRight | KTextEditor::SmartRange::ExpandLeft);
  if(!m_completionRange->isValid()) {
    kWarning(13035) << "Could not construct valid smart-range from" << word << "instead got" << *m_completionRange;
    abortCompletion();
    return;
  }

  connect(m_completionRange->smartStart().notifier(), SIGNAL(characterDeleted(KTextEditor::SmartCursor*, bool)), SLOT(startCharacterDeleted(KTextEditor::SmartCursor*, bool)));

  cursorPositionChanged();

  if (model)
    model->completionInvoked(view(), word, invocationType);
  else
    foreach (KTextEditor::CodeCompletionModel* model, m_sourceModels)
      model->completionInvoked(view(), word, invocationType);

  kDebug () << "msdofjdsoifdsflkdsjf";
  if (model)
    m_presentationModel->setCompletionModel(model);
  else
    m_presentationModel->setCompletionModels(m_sourceModels);

  if (!m_presentationModel->completionModels().isEmpty()) {
    m_presentationModel->setCurrentCompletion(view()->doc()->text(KTextEditor::Range(m_completionRange->start(), view()->cursorPosition())));
  }
  
  connect(this->model(), SIGNAL(contentGeometryChanged()), this, SLOT(modelContentChanged()));
  //Now that all models have been notified, check whether the widget should be displayed instantly
  modelContentChanged();
}

void KateCompletionWidget::updateAndShow()
{
  setUpdatesEnabled(false);

  updatePosition(true);

  if (!m_presentationModel->completionModels().isEmpty()) {
    show();
    m_entryList->resizeColumns(false, true);
    
    m_argumentHintModel->buildRows();
    if( m_argumentHintModel->rowCount(QModelIndex()) != 0 )
      argumentHintsChanged(true);
  }

  setUpdatesEnabled(true);
}

void KateCompletionWidget::updatePositionSlot()
{
  updatePosition();
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
  int x = p.x() - m_entryList->columnViewportPosition(m_presentationModel->translateColumn(KTextEditor::CodeCompletionModel::Name)) - 2;
  int y = p.y();
  //We do not need to move the widget up, because updateHeight will resize the widget to fit the screen
/*  if ( y + height() + view()->renderer()->config()->fontMetrics().height() > QApplication::desktop()->screenGeometry(this).bottom() )
    y -= height();
  else*/
  y += view()->renderer()->config()->fontMetrics().height();

  if (x + width() > QApplication::desktop()->screenGeometry(view()).right())
    x = QApplication::desktop()->screenGeometry(view()).right() - width();

  if( x < QApplication::desktop()->screenGeometry(view()).left() )
    x = QApplication::desktop()->screenGeometry(view()).left();

  move( QPoint(x,y) );

  updateHeight();

  updateArgumentHintGeometry();
}

void KateCompletionWidget::updateArgumentHintGeometry()
{
  if( !m_dontShowArgumentHints ) {
    //Now place the argument-hint widget
    QRect geom = m_argumentHintTree->geometry();
    geom.moveTo(pos());
    geom.setWidth(width());
    geom.moveBottom(pos().y() - view()->renderer()->config()->fontMetrics().height()*2);
    m_argumentHintTree->updateGeometry(geom);
  }
}

void KateCompletionWidget::updateHeight()
{
  kDebug() << "updateHeight(), height: " << geometry().height() << " current added-height: " << m_expandingAddedHeight;

  QRect geom = geometry();

  int baseHeight = geom.height() - m_expandingAddedHeight;

  if( m_expandedAddedHeightBase != baseHeight && m_expandedAddedHeightBase - baseHeight > -2 && m_expandedAddedHeightBase - baseHeight < 2  )
  {
    //Re-use the stored base-height if it only slightly differs from the current one.
    //Reason: Qt seems to apply slightly wrong sizes when the completion-widget is moved out of the screen at the bottom,
    //        which completely breaks this algorithm. Solution: re-use the old base-size if it only slightly differs from the computed one.
    baseHeight = m_expandedAddedHeightBase;
  }

  if( baseHeight < 300 ) {
    baseHeight = 300; //Here we enforce a minimum desirable height
    m_expandingAddedHeight = 0;
//     kDebug() << "Resetting baseHeight and m_expandingAddedHeight";
  }

  int newExpandingAddedHeight = 0;

//   kDebug() << "baseHeight: " << baseHeight;

  newExpandingAddedHeight = model()->expandingWidgetsHeight();

//   kDebug() << "new newExpandingAddedHeight: " << newExpandingAddedHeight;

  int screenBottom = QApplication::desktop()->screenGeometry(view()).bottom();

  int bottomPosition = baseHeight + newExpandingAddedHeight + geometry().top();
//  int targetHeight = baseHeight + newExpandingAddedHeight;
//   kDebug() << "targetHeight: " << targetHeight;

//   kDebug() << "screen-bottom: " << screenBottom << " bottomPosition: " << bottomPosition;

  if( bottomPosition > screenBottom-50 ) {
    newExpandingAddedHeight -= bottomPosition - (screenBottom-50);
//     kDebug() << "Too high, moved bottomPosition to: " << baseHeight + newExpandingAddedHeight + geometry().top() << " changed newExpandingAddedHeight to " << newExpandingAddedHeight;
  }

  int finalHeight = baseHeight+newExpandingAddedHeight;
//   kDebug() << "finalHeight: " << finalHeight;
  if( finalHeight < 50 ) {
    return;
  }

  m_expandingAddedHeight = baseHeight;
  m_expandedAddedHeightBase = geometry().height();

  geom.setHeight(finalHeight);

  setGeometry(geom);
}


void KateCompletionWidget::cursorPositionChanged( )
{
  if (!isCompletionActive())  {
//    m_presentationModel->setCurrentCompletion(QString());
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
  bool ret = isVisible();
  if (ret)
    Q_ASSERT(m_completionRange && m_completionRange->isValid());

  return ret;
}

void KateCompletionWidget::abortCompletion( )
{
  kDebug(13035) ;

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
  kDebug(13035) ;

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

  if (!toExecute.isValid()) {
    kWarning() << k_funcinfo << "Could not map index" << m_entryList->selectionModel()->currentIndex() << "to source index.";
    return abortCompletion();
  }

  KTextEditor::Cursor start = m_completionRange->start();

  // encapsulate all editing as being from the code completion, and undo-able in one step.
  view()->doc()->editStart(true, Kate::CodeCompletionEdit);

  KTextEditor::CodeCompletionModel* model = static_cast<KTextEditor::CodeCompletionModel*>(const_cast<QAbstractItemModel*>(toExecute.model()));
  Q_ASSERT(model);

  KTextEditor::CodeCompletionModel2* model2 = dynamic_cast<KTextEditor::CodeCompletionModel2*>(model);

  if(model2)
    model2->executeCompletionItem2(view()->document(), *m_completionRange, toExecute);
  else if(toExecute.parent().isValid())
    //The normale CodeCompletionInterface cannot handle feedback for hierarchical models, so just do the replacement
    view()->document()->replaceText(*m_completionRange, model->data(toExecute.sibling(toExecute.row(), KTextEditor::CodeCompletionModel::Name)).toString());
  else
    model->executeCompletionItem(view()->document(), *m_completionRange, toExecute.row());

  view()->doc()->editEnd();

  hide();

  view()->sendCompletionExecuted(start, model, toExecute);
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

  if( !m_dontShowArgumentHints && m_argumentHintModel->rowCount(QModelIndex()) != 0 )
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
    updateHeight();
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

  m_configWidget->exec();
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

int KateCompletionWidget::automaticInvocationDelay() const {
  return m_automaticInvocationDelay;
}

void KateCompletionWidget::setAutomaticInvocationDelay(int delay) {
  m_automaticInvocationDelay = delay;
}


void KateCompletionWidget::editDone(KateEditInfo * edit)
{
  if (!isAutomaticInvocationEnabled()
       || (edit->editSource() != Kate::UserInputEdit)
       || edit->isRemoval()
       || isCompletionActive()
       || edit->newText().isEmpty() )
  {
    m_automaticInvocationTimer->stop();
    return;
  }

  m_automaticInvocationLine = edit->newText().last();

  if (m_automaticInvocationLine.isEmpty()) {
    m_automaticInvocationTimer->stop();
    return;
  }

  m_automaticInvocationTimer->start(m_automaticInvocationDelay);
}

void KateCompletionWidget::automaticInvocation()
{
  if(m_automaticInvocationLine.isEmpty())
    return;

  QString lastLine = m_automaticInvocationLine;
  QChar lastChar = lastLine.at(lastLine.count() - 1);

  if (lastChar.isLetter() || lastChar.isNumber() || lastChar == '.' || lastChar == '_' || lastChar == '>') {
    // Start automatic code completion
    KTextEditor::Range range = determineRange();
    if (range.isValid())
      startCompletion(range, 0, KTextEditor::CodeCompletionModel::AutomaticInvocation);
    else
      kWarning(13035) << "Completion range was invalid even though it was expected to be valid.";
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
