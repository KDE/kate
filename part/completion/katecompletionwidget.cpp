/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2005-2006 Hamish Rodda <rodda@kde.org>
 *  Copyright (C) 2007-2008 David Nolden <david.nolden.kdevelop@art-master.de>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
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
#include <QtCore/QMutex>
#include <QtCore/QScopedPointer>

#include <kicon.h>
#include <kdialog.h>

#include <ktexteditor/smartcursornotifier.h>
#include <ktexteditor/codecompletionmodelcontrollerinterface.h>

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

const bool hideAutomaticCompletionOnExactMatch = true;

//If this is true, the completion-list is navigated up/down when 'tab' is pressed, instead of doing partial completion
const bool shellLikeTabCompletion = false;

#define CALLCI(WHAT,WHATELSE,WHAT2,model,FUNC) \
{\
  /*kDebug()<<"CALLCI";*/ \
  KTextEditor::CodeCompletionModelControllerInterface* ret =\
    qobject_cast<KTextEditor::CodeCompletionModelControllerInterface*>(model);\
  if (ret) {\
        kDebug()<<"You are using a deprecated API, this will break in KDE 4.6, please port your module to the new KTextEditor::CodeCompletionModelControllerInterface3";\
    WHAT ret->FUNC;\
    WHATELSE;\
  }\
}\
\
{\
  static KTextEditor::CodeCompletionModelControllerInterface3 defaultIf;\
  KTextEditor::CodeCompletionModelControllerInterface3* ret =\
    qobject_cast<KTextEditor::CodeCompletionModelControllerInterface3*>(model);\
  if (!ret) {\
    WHAT2 defaultIf.FUNC;\
  }else \
  WHAT2 ret->FUNC;\
}


static KTextEditor::Range _completionRange(KTextEditor::CodeCompletionModel *model, KTextEditor::View *view, const KTextEditor::Cursor& cursor){
  CALLCI(return,,return, model,completionRange(view, cursor));
}

static KTextEditor::Range _updateRange(KTextEditor::CodeCompletionModel *model,KTextEditor::View *view, KTextEditor::SmartRange& range) {
  CALLCI(, return range,return, model,updateCompletionRange(view, range));
}

static QString _filterString(KTextEditor::CodeCompletionModel *model,KTextEditor::View *view, const KTextEditor::SmartRange& range, const KTextEditor::Cursor& cursor) {
  CALLCI(return,,return, model,filterString(view, range, cursor));
}

static bool _shouldAbortCompletion(KTextEditor::CodeCompletionModel *model,KTextEditor::View *view, const KTextEditor::SmartRange& range, const QString& currentCompletion) {
      CALLCI(return,,return, model,shouldAbortCompletion(view, range, currentCompletion));
}

static void _aborted(KTextEditor::CodeCompletionModel *model,KTextEditor::View *view) {
      CALLCI(return,,return, model,aborted(view));
}

static bool _shouldStartCompletion(KTextEditor::CodeCompletionModel *model,KTextEditor::View *view, QString m_automaticInvocationLine,bool m_lastInsertionByUser, const KTextEditor::Cursor& cursor) {
      CALLCI(return,,return,model,shouldStartCompletion(view, m_automaticInvocationLine, m_lastInsertionByUser, cursor));
}

KateCompletionWidget::KateCompletionWidget(KateView* parent)
  : QFrame(parent, Qt::ToolTip)
  , m_presentationModel(new KateCompletionModel(this))
  , m_entryList(new KateCompletionTree(this))
  , m_argumentHintModel(new KateArgumentHintModel(this))
  , m_argumentHintTree(new KateArgumentHintTree(this))
  , m_automaticInvocationDelay(100)
  , m_filterInstalled(false)
  , m_configWidget(new KateCompletionConfig(m_presentationModel, view()))
  , m_lastInsertionByUser(false)
  , m_inCompletionList(false)
  , m_isSuspended(false)
  , m_dontShowArgumentHints(false)
  , m_needShow(false)
  , m_hadCompletionNavigation(false)
  , m_noAutoHide(false)
  , m_expandedAddedHeightBase(0)
  , m_lastInvocationType(KTextEditor::CodeCompletionModel::AutomaticInvocation)
{
  connect(parent, SIGNAL(navigateAccept()), SLOT(navigateAccept()));
  connect(parent, SIGNAL(navigateBack()), SLOT(navigateBack()));
  connect(parent, SIGNAL(navigateDown()), SLOT(navigateDown()));
  connect(parent, SIGNAL(navigateLeft()), SLOT(navigateLeft()));
  connect(parent, SIGNAL(navigateRight()), SLOT(navigateRight()));
  connect(parent, SIGNAL(navigateUp()), SLOT(navigateUp()));

  qRegisterMetaType<KTextEditor::Cursor>("KTextEditor::Cursor");

  setFrameStyle( QFrame::Box | QFrame::Plain );
  setLineWidth( 1 );
  //setWindowOpacity(0.8);

  m_entryList->setModel(m_presentationModel);
  m_entryList->setColumnWidth(0, 0); //These will be determined automatically in KateCompletionTree::resizeColumns
  m_entryList->setColumnWidth(1, 0);
  m_entryList->setColumnWidth(2, 0);

  m_entryList->setVerticalScrollMode(QAbstractItemView::ScrollPerItem);

  m_argumentHintTree->setParent(0, Qt::ToolTip);
  m_argumentHintTree->setModel(m_argumentHintModel);

  connect(m_entryList->verticalScrollBar(), SIGNAL(valueChanged(int)), m_presentationModel, SLOT(placeExpandingWidgets()));
  connect(m_argumentHintTree->verticalScrollBar(), SIGNAL(valueChanged(int)), m_argumentHintModel, SLOT(placeExpandingWidgets()));
  connect(view(), SIGNAL(focusOut(KTextEditor::View*)), this, SLOT(viewFocusOut()));

  m_automaticInvocationTimer = new QTimer(this);
  m_automaticInvocationTimer->setSingleShot(true);
  connect(m_automaticInvocationTimer, SIGNAL(timeout()), this, SLOT(automaticInvocation()));

//   QVBoxLayout* vl = new QVBoxLayout(this);
//   vl->addWidget(m_entryList);
//   vl->setMargin(0);

  // Keep branches expanded
  connect(m_presentationModel, SIGNAL(modelReset()), this, SLOT(modelReset()));
  connect(m_presentationModel, SIGNAL(rowsInserted(const QModelIndex&, int, int)), SLOT(rowsInserted(const QModelIndex&, int, int)));
  connect(m_argumentHintModel, SIGNAL(contentStateChanged(bool)), this, SLOT(argumentHintsChanged(bool)));

  // These must be queued connections so that we're not holding the smart lock when we ask for the model to update.
  connect(view(), SIGNAL(cursorPositionChanged(KTextEditor::View*, const KTextEditor::Cursor&)), this, SLOT(cursorPositionChanged()), Qt::QueuedConnection);
  connect(view()->doc()->history(), SIGNAL(editDone(KateEditInfo*)), SLOT(editDone(KateEditInfo*)));
  connect(view(), SIGNAL(verticalScrollPositionChanged (KTextEditor::View*, const KTextEditor::Cursor&)), this, SLOT(updatePositionSlot()), Qt::QueuedConnection);

  // This is a non-focus widget, it is passed keyboard input from the view

  //We need to do this, because else the focus goes to nirvana without any control when the completion-widget is clicked.
  setFocusPolicy(Qt::ClickFocus);
  m_argumentHintTree->setFocusPolicy(Qt::ClickFocus);

  foreach (QWidget* childWidget, findChildren<QWidget*>())
    childWidget->setFocusPolicy(Qt::NoFocus);

  //Position the entry-list so a frame can be drawn around it
  m_entryList->move(frameWidth(), frameWidth());
}

KateCompletionWidget::~KateCompletionWidget() {
}

void KateCompletionWidget::viewFocusOut() {
  abortCompletion();
}

void KateCompletionWidget::modelContentChanged() {
  kDebug()<<">>>>>>>>>>>>>>>>";
  if(m_completionRanges.isEmpty()) {
    kDebug( 13035 ) << "content changed, but no completion active";
    abortCompletion();
    return;
  }

  if(!view()->hasFocus()) {
    kDebug( 13035 ) << "view does not have focus";
    return;
  }

  if(!m_waitingForReset.isEmpty()) {
    kDebug( 13035 ) << "waiting for" << m_waitingForReset.size() << "completion-models to reset";
    return;
  }

  int realItemCount = 0;
  foreach (KTextEditor::CodeCompletionModel* model, m_presentationModel->completionModels())
    realItemCount += model->rowCount();
  if( !m_isSuspended && ((isHidden() && m_argumentHintTree->isHidden()) || m_needShow) && realItemCount != 0 ) {
    m_needShow = false;
    updateAndShow();
  }

  if(m_argumentHintModel->rowCount(QModelIndex()) == 0)
    m_argumentHintTree->hide();

  if(m_presentationModel->rowCount(QModelIndex()) == 0)
    hide();


  //With each filtering items can be added or removed, so we have to reset the current index here so we always have a selected item
  m_entryList->setCurrentIndex(model()->index(0,0));
  if(!model()->indexIsItem(m_entryList->currentIndex())) {
    QModelIndex firstIndex = model()->index(0,0, m_entryList->currentIndex());
    m_entryList->setCurrentIndex(firstIndex);
    //m_entryList->scrollTo(firstIndex, QAbstractItemView::PositionAtTop);
  }

  updateHeight();

  //New items for the argument-hint tree may have arrived, so check whether it needs to be shown
  if( m_argumentHintTree->isHidden() && !m_dontShowArgumentHints && m_argumentHintModel->rowCount(QModelIndex()) != 0 )
    m_argumentHintTree->show();

  if(!m_noAutoHide && hideAutomaticCompletionOnExactMatch && !isHidden() &&
     m_lastInvocationType == KTextEditor::CodeCompletionModel::AutomaticInvocation &&
     m_presentationModel->shouldMatchHideCompletionList())
    hide();
  else if(isHidden() && !m_presentationModel->shouldMatchHideCompletionList() &&
                         m_presentationModel->rowCount(QModelIndex()))
    show();
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
  m_entryList->setAnimated(false);
  if(!model()->isGroupingEnabled())
    return;

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

void KateCompletionWidget::startCompletion(KTextEditor::CodeCompletionModel::InvocationType invocationType, const QList<KTextEditor::CodeCompletionModel*>& models)
{
  if(invocationType == KTextEditor::CodeCompletionModel::UserInvocation)
  {
    abortCompletion();
  }
  startCompletion(KTextEditor::Range(KTextEditor::Cursor(-1, -1), KTextEditor::Cursor(-1, -1)), models, invocationType);
}

void KateCompletionWidget::deleteCompletionRanges()
{
  //kDebug();
  foreach(const CompletionRange &r, m_completionRanges)
    delete r.range;
  m_completionRanges.clear();
}

void KateCompletionWidget::startCompletion(const KTextEditor::Range& word, KTextEditor::CodeCompletionModel* model, KTextEditor::CodeCompletionModel::InvocationType invocationType)
{
  QList<KTextEditor::CodeCompletionModel*> models;
  if (model) {
    models << model;
  } else {
    models = m_sourceModels;
  }
  startCompletion(word, models, invocationType);
}

void KateCompletionWidget::startCompletion(const KTextEditor::Range& word, const QList<KTextEditor::CodeCompletionModel*>& modelsToStart, KTextEditor::CodeCompletionModel::InvocationType invocationType)
{

  kDebug()<<"============";

  m_isSuspended = false;
  m_inCompletionList = true; //Always start at the top of the completion-list
  m_needShow = true;

  if(m_completionRanges.isEmpty())
    m_noAutoHide = false; //Re-enable auto-hide on every clean restart of the completion

  m_lastInvocationType = invocationType;

  disconnect(this->model(), SIGNAL(contentGeometryChanged()), this, SLOT(modelContentChanged()));

  m_dontShowArgumentHints = true;

  QList<KTextEditor::CodeCompletionModel*> models = (modelsToStart.isEmpty() ? m_sourceModels : modelsToStart);

  foreach(KTextEditor::CodeCompletionModel* model, m_completionRanges.keys())
    if(!models.contains(model))
      models << model;

  if (!m_filterInstalled) {
    if (!QApplication::activeWindow()) {
      kWarning(13035) << "No active window to install event filter on!!";
      return;
    }
    // Enable the cc box to move when the editor window is moved
    QApplication::activeWindow()->installEventFilter(this);
    m_filterInstalled = true;
  }

  m_presentationModel->clearCompletionModels();

  if(invocationType == KTextEditor::CodeCompletionModel::UserInvocation) {
    QMutexLocker lock(view()->doc()->smartMutex());
    deleteCompletionRanges();
  }

  foreach (KTextEditor::CodeCompletionModel* model, models) {
    KTextEditor::Range range;
    if (word.isValid()) {
      range = word;
      kDebug()<<"word is used";
    } else {
      range=_completionRange(model,view(), view()->cursorPosition());
      kDebug()<<"completionRange has been called, cursor pos is"<<view()->cursorPosition();
    }
    kDebug()<<"range is"<<range;
    if(!range.isValid()) {
      if(m_completionRanges.contains(model)) {
        QMutexLocker lock(view()->doc()->smartMutex());
        KTextEditor::SmartRange *oldRange = m_completionRanges[model].range;
        kDebug()<<"removing completion range 1";
        m_completionRanges.remove(model);
        delete oldRange;
      }
      models.removeAll(model);
      continue;
    }
    if(m_completionRanges.contains(model)) {
      if(*m_completionRanges[model].range == range) {
        continue; //Leave it running as it is
      }
      else { // delete the range that was used previously
        QMutexLocker lock(view()->doc()->smartMutex());
        KTextEditor::SmartRange *oldRange = m_completionRanges[model].range;
        kDebug()<<"removing completion range 2";
        m_completionRanges.remove(model);
        delete oldRange;
      }
    }

    connect(model, SIGNAL(waitForReset()), this, SLOT(waitForModelReset()));

    kDebug()<<"Before completin invoke: range:"<<range;
    model->completionInvoked(view(), range, invocationType);

    disconnect(model, SIGNAL(waitForReset()), this, SLOT(waitForModelReset()));

    QMutexLocker lock(view()->doc()->smartMutex());

    m_completionRanges[model] = view()->doc()->smartManager()->newSmartRange(range);
    m_completionRanges[model].range->setInsertBehavior(KTextEditor::SmartRange::ExpandRight | KTextEditor::SmartRange::ExpandLeft);

    //In automatic invocation mode, hide the completion widget as soon as the position where the completion was started is passed to the left
    m_completionRanges[model].leftBoundary = view()->cursorPosition();

    //In manual invocation mode, bound the activity either the the point from where completion was invoked, or to the start of the range
    if(invocationType != KTextEditor::CodeCompletionModel::AutomaticInvocation)
      if(range.start() < m_completionRanges[model].leftBoundary)
        m_completionRanges[model].leftBoundary = range.start();

    if(!m_completionRanges[model].range->isValid()) {
      kWarning(13035) << "Could not construct valid smart-range from" << range << "instead got" << *m_completionRanges[model].range;
      lock.unlock();
      abortCompletion();
      return;
    }

    lock.unlock();
  }

  m_presentationModel->setCompletionModels(models);

  cursorPositionChanged();

  if (!m_completionRanges.isEmpty()) {
    connect(this->model(), SIGNAL(contentGeometryChanged()), this, SLOT(modelContentChanged()));
    //Now that all models have been notified, check whether the widget should be displayed instantly
    modelContentChanged();
  }
  else {
    abortCompletion();
  }
}

void KateCompletionWidget::waitForModelReset()
{
  KTextEditor::CodeCompletionModel* senderModel = qobject_cast<KTextEditor::CodeCompletionModel*>(sender());
  if(!senderModel) {
    kWarning() << "waitForReset signal from bad model";
    return;
  }
  m_waitingForReset.insert(senderModel);
}

void KateCompletionWidget::updateAndShow()
{
  kDebug()<<"*******************************************";
  if(!view()->hasFocus()) {
    kDebug( 13035 ) << "view does not have focus";
    return;
  }

  setUpdatesEnabled(false);

  modelReset();

    m_argumentHintModel->buildRows();
    if( m_argumentHintModel->rowCount(QModelIndex()) != 0 )
      argumentHintsChanged(true);
//   }

  //We do both actions twice here so they are stable, because they influence each other:
  //updatePosition updates the height, resizeColumns needs the correct height to decide over
  //how many rows it computs the column-width
  updatePosition(true);
  m_entryList->resizeColumns(true, true);
  updatePosition(true);
  m_entryList->resizeColumns(true, true);

  setUpdatesEnabled(true);

  if(m_argumentHintModel->rowCount(QModelIndex())) {
    updateArgumentHintGeometry();
    m_argumentHintTree->show();
  } else
    m_argumentHintTree->hide();

  if (m_presentationModel->rowCount() && (!m_presentationModel->shouldMatchHideCompletionList() ||
                                           !hideAutomaticCompletionOnExactMatch  ||
                                           m_lastInvocationType != KTextEditor::CodeCompletionModel::AutomaticInvocation) )
    show();
  else
    hide();
}

void KateCompletionWidget::updatePositionSlot()
{
  updatePosition();
}

bool KateCompletionWidget::updatePosition(bool force)
{
  if (!force && !isCompletionActive())
    return false;

  if (!completionRange()) {
    return false;
  }
  QPoint cursorPosition = view()->cursorToCoordinate(completionRange()->start());
  if (cursorPosition == QPoint(-1,-1)) {
    // Start of completion range is now off-screen -> abort
    abortCompletion();
    return false;
  }

  QPoint p = view()->mapToGlobal( cursorPosition );
  int x = p.x() - m_entryList->columnTextViewportPosition(m_presentationModel->translateColumn(KTextEditor::CodeCompletionModel::Name)) - 4 - (m_entryList->viewport()->pos().x());
  int y = p.y();

  y += view()->renderer()->config()->fontMetrics().height() + 4;

  bool borderHit = false;

  if (x + width() > QApplication::desktop()->screenGeometry(view()).right()) {
    x = QApplication::desktop()->screenGeometry(view()).right() - width();
    borderHit = true;
  }

  if( x < QApplication::desktop()->screenGeometry(view()).left() ) {
    x = QApplication::desktop()->screenGeometry(view()).left();
    borderHit = true;
  }

  move( QPoint(x,y) );

  updateHeight();

  updateArgumentHintGeometry();

//   kDebug() << "updated to" << geometry() << m_entryList->geometry() << borderHit;

  return borderHit;
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

//Checks whether the given model has at least "rows" rows, also searching the second level of the tree.
bool hasAtLeastNRows(int rows, QAbstractItemModel* model) {
  int count = 0;
  for(int row = 0; row < model->rowCount(); ++row) {
    ++count;

    QModelIndex index(model->index(row, 0));
    if(index.isValid())
      count += model->rowCount(index);

    if(count > rows)
      return true;
  }
  return false;
}

void KateCompletionWidget::updateHeight()
{
  QRect geom = geometry();

  int minBaseHeight = 10;
  int maxBaseHeight = 300;

  int baseHeight = 0;
  int calculatedCustomHeight = 0;

  if(hasAtLeastNRows(15, m_presentationModel)) {
    //If we know there is enough rows, always use max-height, we don't need to calculate size-hints
    baseHeight = maxBaseHeight;
  }else{
    //Calculate size-hints to determine the best height
    for(int row = 0; row < m_presentationModel->rowCount(); ++row) {
      baseHeight += treeView()->sizeHintForRow(row);

      QModelIndex index(m_presentationModel->index(row, 0));
      if(index.isValid()) {
        for(int row2 = 0; row2 < m_presentationModel->rowCount(index); ++row2) {
          int h = 0;
          for(int a = 0; a < m_presentationModel->columnCount(index); ++a) {
            int localHeight = treeView()->sizeHintForIndex(index.child(row2, a)).height();
            if(localHeight > h)
              h = localHeight;
          }
          baseHeight += h;
          if(baseHeight > maxBaseHeight)
            break;
        }

        if(baseHeight > maxBaseHeight)
          break;
      }
    }

    calculatedCustomHeight = baseHeight;
  }

  baseHeight += 2*frameWidth();

  if(m_entryList->horizontalScrollBar()->isVisible())
    baseHeight += m_entryList->horizontalScrollBar()->height();


  if(baseHeight < minBaseHeight)
    baseHeight = minBaseHeight;
  if(baseHeight > maxBaseHeight) {
    baseHeight = maxBaseHeight;
    m_entryList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  }else{
    //Somewhere there seems to be a bug that makes QTreeView add a scroll-bar
    //even if the content exactly fits in. So forcefully disable the scroll-bar in that case
    m_entryList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

  int newExpandingAddedHeight = 0;

  if(baseHeight == maxBaseHeight && model()->expandingWidgetsHeight()) {
    //Eventually add some more height
    if(calculatedCustomHeight && calculatedCustomHeight > baseHeight && calculatedCustomHeight < (maxBaseHeight + model()->expandingWidgetsHeight()))
      newExpandingAddedHeight = calculatedCustomHeight - baseHeight;
    else
      newExpandingAddedHeight = model()->expandingWidgetsHeight();
  }

  if( m_expandedAddedHeightBase != baseHeight && m_expandedAddedHeightBase - baseHeight > -2 && m_expandedAddedHeightBase - baseHeight < 2  )
  {
    //Re-use the stored base-height if it only slightly differs from the current one.
    //Reason: Qt seems to apply slightly wrong sizes when the completion-widget is moved out of the screen at the bottom,
    //        which completely breaks this algorithm. Solution: re-use the old base-size if it only slightly differs from the computed one.
    baseHeight = m_expandedAddedHeightBase;
  }

  int screenBottom = QApplication::desktop()->screenGeometry(view()).bottom();

  //Limit the height to the bottom of the screen
  int bottomPosition = baseHeight + newExpandingAddedHeight + geometry().top();

  if( bottomPosition > screenBottom ) {
    newExpandingAddedHeight -= bottomPosition - (screenBottom);
  }

  int finalHeight = baseHeight+newExpandingAddedHeight;

  if( finalHeight < 10 ) {
    m_entryList->resize(m_entryList->width(), height() - 2*frameWidth());
    return;
  }

  m_expandedAddedHeightBase = geometry().height();

  geom.setHeight(finalHeight);

  //Work around a crash deep within the Qt 4.5 raster engine
  m_entryList->setScrollingEnabled(false);

  if(geometry() != geom)
    setGeometry(geom);

  QSize entryListSize = QSize(m_entryList->width(), finalHeight - 2*frameWidth());
  if(m_entryList->size() != entryListSize)
    m_entryList->resize(entryListSize);


  m_entryList->setScrollingEnabled(true);
}

void KateCompletionWidget::cursorPositionChanged( )
{
  //kDebug();
  if (m_completionRanges.isEmpty())
    return;

  QModelIndex oldCurrentSourceIndex;
  if(m_inCompletionList && m_entryList->currentIndex().isValid())
    oldCurrentSourceIndex = m_presentationModel->mapToSource(m_entryList->currentIndex());

  KTextEditor::Cursor cursor = view()->cursorPosition();

  QList<KTextEditor::CodeCompletionModel*> checkCompletionRanges = m_completionRanges.keys();

  QMutexLocker lock(view()->doc()->smartMutex());
  lock.unlock();

  //Check the models and eventuall abort some
  for(QList<KTextEditor::CodeCompletionModel*>::iterator it = checkCompletionRanges.begin();
      it != checkCompletionRanges.end(); ++it) {
      if(!m_completionRanges.contains(*it))
        continue;

      KTextEditor::CodeCompletionModel *model = *it;
      KateSmartRange* range = m_completionRanges[*it].range;

      kDebug()<<"range before _updateRange:"<< *range;
      *range=_updateRange(model, view(),*range);
      kDebug()<<"range after _updateRange:"<< *range;
      QString currentCompletion = _filterString(model,view(), *range, view()->cursorPosition());
      kDebug()<<"after _filterString, currentCompletion="<< currentCompletion;
      bool abort = _shouldAbortCompletion(model,view(), *range, currentCompletion);
      kDebug()<<"after _shouldAbortCompletion:abort="<<abort;
      if(view()->cursorPosition() < m_completionRanges[*it].leftBoundary) {
        kDebug() << "aborting because of boundary: cursor:"<<view()->cursorPosition()<<"completion_Range_left_boundary:"<<m_completionRanges[*it].leftBoundary;
        abort = true;
      }

      if(!m_completionRanges.contains(*it))
        continue;

      if (abort) {
        if (m_completionRanges.count() == 1) {
          //last model - abort whole completion
          abortCompletion();
          return;
        } else {
          {
            lock.relock();
            delete m_completionRanges[*it].range;
            kDebug()<<"removing completion range 3";
            m_completionRanges.remove(*it);
            lock.unlock();
          }

          _aborted(model,view());
          m_presentationModel->removeCompletionModel(model);
        }
      } else {
        m_presentationModel->setCurrentCompletion(model, currentCompletion);
      }
  }

  if(oldCurrentSourceIndex.isValid()) {
    QModelIndex idx = m_presentationModel->mapFromSource(oldCurrentSourceIndex);
    if(idx.isValid()) {
      kDebug() << "setting" << idx;
      m_entryList->setCurrentIndex(idx.sibling(idx.row(), 0));
//       m_entryList->nextCompletion();
//       m_entryList->previousCompletion();
    }else{
      kDebug() << "failed to map from source";
    }
  }

  m_entryList->scheduleUpdate();
}

bool KateCompletionWidget::isCompletionActive( ) const
{
  return !m_completionRanges.isEmpty() && ((!isHidden() && isVisible()) || (!m_argumentHintTree->isHidden() && m_argumentHintTree->isVisible()));
}

void KateCompletionWidget::abortCompletion( )
{
  //kDebug(13035) ;

  m_isSuspended = false;

  bool wasActive = isCompletionActive();

  clear();

  if(!isHidden())
    hide();
  if(!m_argumentHintTree->isHidden())
    m_argumentHintTree->hide();

  if (wasActive)
    view()->sendCompletionAborted();
}

void KateCompletionWidget::clear() {
  m_presentationModel->clearCompletionModels();
  m_argumentHintTree->clearCompletion();
  m_argumentHintModel->clear();

  foreach(KTextEditor::CodeCompletionModel* model, m_completionRanges.keys())
    _aborted(model,view());

  QMutexLocker lock(view()->doc()->smartMutex());

  deleteCompletionRanges();
}

bool KateCompletionWidget::navigateAccept() {
  m_hadCompletionNavigation = true;

  if(currentEmbeddedWidget())
    QMetaObject::invokeMethod(currentEmbeddedWidget(), "embeddedWidgetAccept");

  QModelIndex index = selectedIndex();
  if( index.isValid() ) {
    index.data(KTextEditor::CodeCompletionModel::AccessibilityAccept);
    return true;
  }
  return false;
}

void KateCompletionWidget::execute()
{
  //kDebug(13035) ;

  if (!isCompletionActive())
    return;

  QModelIndex index = selectedIndex();

  if (!index.isValid())
    return abortCompletion();

  QModelIndex toExecute;

  if(index.model() == m_presentationModel)
    toExecute = m_presentationModel->mapToSource(index);
  else
    toExecute = m_argumentHintModel->mapToSource(index);

  if (!toExecute.isValid()) {
    kWarning() << k_funcinfo << "Could not map index" << m_entryList->selectionModel()->currentIndex() << "to source index.";
    return abortCompletion();
  }

  // encapsulate all editing as being from the code completion, and undo-able in one step.
  view()->doc()->editStart(Kate::CodeCompletionEdit);

  // create scoped pointer, to ensure deletion of cursor
  QScopedPointer<KTextEditor::MovingCursor> oldPos (view()->doc()->newMovingCursor(view()->cursorPosition(), KTextEditor::MovingCursor::StayOnInsert));

  KTextEditor::CodeCompletionModel* model = static_cast<KTextEditor::CodeCompletionModel*>(const_cast<QAbstractItemModel*>(toExecute.model()));
  Q_ASSERT(model);

  KTextEditor::CodeCompletionModel2* model2 = qobject_cast<KTextEditor::CodeCompletionModel2*>(model);

  Q_ASSERT(m_completionRanges.contains(model));
  KTextEditor::Cursor start = m_completionRanges[model].range->start();

  //editStart locks the smart-mutex, but it must not be locked when calling external functions,
  //else we may get deadlock-issues.
  view()->doc()->smartMutex()->unlock();

  if(model2)
    model2->executeCompletionItem2(view()->document(), *m_completionRanges[model].range, toExecute);
  else if(toExecute.parent().isValid())
    //The normale CodeCompletionInterface cannot handle feedback for hierarchical models, so just do the replacement
    view()->document()->replaceText(*m_completionRanges[model].range, model->data(toExecute.sibling(toExecute.row(), KTextEditor::CodeCompletionModel::Name)).toString());
  else
    model->executeCompletionItem(view()->document(), *m_completionRanges[model].range, toExecute.row());

  //Relock, because editEnd expects it to be locked
  view()->doc()->smartMutex()->lock();

  view()->doc()->editEnd();

  abortCompletion();

  view()->sendCompletionExecuted(start, model, toExecute);

  KTextEditor::Cursor newPos = view()->cursorPosition();

  if(newPos > *oldPos) {
    m_automaticInvocationAt = newPos;
    m_automaticInvocationLine = view()->doc()->text(KTextEditor::Range(*oldPos, newPos));
    kDebug() << "executed, starting automatic invocation with line" << m_automaticInvocationLine;
    m_lastInsertionByUser = false;
    m_automaticInvocationTimer->start();
  }
}

void KateCompletionWidget::resizeEvent( QResizeEvent * event )
{
  QFrame::resizeEvent(event);
}

void KateCompletionWidget::showEvent ( QShowEvent * event )
{
  m_isSuspended = false;

  QFrame::showEvent(event);

  if( !m_dontShowArgumentHints && m_argumentHintModel->rowCount(QModelIndex()) != 0 )
    m_argumentHintTree->show();
}

KateSmartRange * KateCompletionWidget::completionRange(KTextEditor::CodeCompletionModel* model) const
{
  if (!model) {
    if (m_completionRanges.isEmpty()) return 0;

    KateSmartRange* ret = m_completionRanges.begin()->range;
    
    foreach(CompletionRange range, m_completionRanges)
      if(range.range->start() > ret->start())
        ret = range.range;
    return ret;
  }
  if(m_completionRanges.contains(model))
    return m_completionRanges[model].range;
  else
    return 0;
}

QMap<KTextEditor::CodeCompletionModel*, KateCompletionWidget::CompletionRange> KateCompletionWidget::completionRanges( ) const
{
  return m_completionRanges;
}

void KateCompletionWidget::modelReset( )
{
  setUpdatesEnabled(false);
  m_entryList->setAnimated(false);
  m_argumentHintTree->setAnimated(false);
  ///We need to do this by hand, because QTreeView::expandAll is very inefficient.
  ///It creates a QPersistentModelIndex for every single item in the whole tree..
  for(int row = 0; row < m_argumentHintModel->rowCount(QModelIndex()); ++row) {
    QModelIndex index(m_argumentHintModel->index(row, 0, QModelIndex()));
    if(!m_argumentHintTree->isExpanded(index)) {
      m_argumentHintTree->expand(index);
    }
  }

  for(int row = 0; row < m_entryList->model()->rowCount(QModelIndex()); ++row) {
    QModelIndex index(m_entryList->model()->index(row, 0, QModelIndex()));
    if(!m_entryList->isExpanded(index)) {
      m_entryList->expand(index);
    }
  }
  setUpdatesEnabled(true);
}

KateCompletionTree* KateCompletionWidget::treeView() const {
  return m_entryList;
}

QModelIndex KateCompletionWidget::selectedIndex() const {
  if(!isCompletionActive())
    return QModelIndex();

  if( m_inCompletionList )
    return m_entryList->currentIndex();
  else
    return m_argumentHintTree->currentIndex();
}

bool KateCompletionWidget::navigateLeft() {
  m_hadCompletionNavigation = true;
  if(currentEmbeddedWidget())
    QMetaObject::invokeMethod(currentEmbeddedWidget(), "embeddedWidgetLeft");

  QModelIndex index = selectedIndex();

  if( index.isValid() ) {
    index.data(KTextEditor::CodeCompletionModel::AccessibilityPrevious);

    return true;
  }
  return false;
}

bool KateCompletionWidget::navigateRight() {
  m_hadCompletionNavigation = true;
  if(currentEmbeddedWidget()) ///@todo post 4.2: Make these slots public interface, or create an interface using virtual functions
    QMetaObject::invokeMethod(currentEmbeddedWidget(), "embeddedWidgetRight");

  QModelIndex index = selectedIndex();

  if( index.isValid() ) {
    index.data(KTextEditor::CodeCompletionModel::AccessibilityNext);
    return true;
  }

  return false;
}

bool KateCompletionWidget::hadNavigation() const {
  return m_hadCompletionNavigation;
}

void KateCompletionWidget::resetHadNavigation() {
  m_hadCompletionNavigation = false;
}


bool KateCompletionWidget::navigateBack() {
  m_hadCompletionNavigation = true;
  if(currentEmbeddedWidget())
    QMetaObject::invokeMethod(currentEmbeddedWidget(), "embeddedWidgetBack");
  return false;
}

bool KateCompletionWidget::toggleExpanded(bool forceExpand, bool forceUnExpand) {
  if ( (canExpandCurrentItem() || forceExpand ) && !forceUnExpand) {
    bool ret = canExpandCurrentItem();
    setCurrentItemExpanded(true);
    return ret;
  } else if (canCollapseCurrentItem() || forceUnExpand) {
    bool ret = canCollapseCurrentItem();
    setCurrentItemExpanded(false);
    return ret;
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

bool KateCompletionWidget::eventFilter( QObject * watched, QEvent * event )
{
  bool ret = QFrame::eventFilter(watched, event);

  if (watched != this)
    if (event->type() == QEvent::Move)
      updatePosition();

  return ret;
}

bool KateCompletionWidget::navigateDown() {
  m_hadCompletionNavigation = true;
  if(currentEmbeddedWidget()) {
    QMetaObject::invokeMethod(currentEmbeddedWidget(), "embeddedWidgetDown");
  }
  return false;
}

bool KateCompletionWidget::navigateUp() {
  m_hadCompletionNavigation = true;
  if(currentEmbeddedWidget())
    QMetaObject::invokeMethod(currentEmbeddedWidget(), "embeddedWidgetUp");
  return false;
}

QWidget* KateCompletionWidget::currentEmbeddedWidget() {
  QModelIndex index = selectedIndex();
  if(!index.isValid())
    return 0;
  if( qobject_cast<const ExpandingWidgetModel*>(index.model()) ) {
    const ExpandingWidgetModel* model = static_cast<const ExpandingWidgetModel*>(index.model());
    if( model->isExpanded(index) )
      return model->expandingWidget(index);
  }
  return 0;
}

void KateCompletionWidget::cursorDown()
{
  bool wasPartiallyExpanded = model()->partiallyExpandedRow().isValid();

  if( m_inCompletionList )
    m_entryList->nextCompletion();
  else {
    if( !m_argumentHintTree->nextCompletion() )
      switchList();
  }

  if(wasPartiallyExpanded != model()->partiallyExpandedRow().isValid())
    updateHeight();
}

void KateCompletionWidget::cursorUp()
{
  bool wasPartiallyExpanded = model()->partiallyExpandedRow().isValid();

  if( m_inCompletionList ) {
    if( !m_entryList->previousCompletion() )
      switchList();
  }else{
    m_argumentHintTree->previousCompletion();
  }

  if(wasPartiallyExpanded != model()->partiallyExpandedRow().isValid())
    updateHeight();
}

void KateCompletionWidget::pageDown( )
{
  bool wasPartiallyExpanded = model()->partiallyExpandedRow().isValid();

  if( m_inCompletionList )
    m_entryList->pageDown();
  else {
    if( !m_argumentHintTree->pageDown() )
      switchList();
  }

  if(wasPartiallyExpanded != model()->partiallyExpandedRow().isValid())
    updateHeight();
}

void KateCompletionWidget::pageUp( )
{
  bool wasPartiallyExpanded = model()->partiallyExpandedRow().isValid();

  if( m_inCompletionList ) {
    if( !m_entryList->pageUp() )
      switchList();
  }else{
    m_argumentHintTree->pageUp();
  }

  if(wasPartiallyExpanded != model()->partiallyExpandedRow().isValid())
    updateHeight();
}

void KateCompletionWidget::top( )
{
  bool wasPartiallyExpanded = model()->partiallyExpandedRow().isValid();

  if( m_inCompletionList )
    m_entryList->top();
  else
    m_argumentHintTree->top();

  if(wasPartiallyExpanded != model()->partiallyExpandedRow().isValid())
    updateHeight();
}

void KateCompletionWidget::bottom( )
{
  bool wasPartiallyExpanded = model()->partiallyExpandedRow().isValid();

  if( m_inCompletionList )
    m_entryList->bottom();
  else
    m_argumentHintTree->bottom();

  if(wasPartiallyExpanded != model()->partiallyExpandedRow().isValid())
    updateHeight();
}

void KateCompletionWidget::switchList() {
  if( m_inCompletionList ) {
      if( m_argumentHintModel->rowCount(QModelIndex()) != 0 ) {
        m_entryList->setCurrentIndex(QModelIndex());
        m_argumentHintTree->setCurrentIndex(m_argumentHintModel->index(m_argumentHintModel->rowCount(QModelIndex())-1, 0));
      }
  } else {
      if( m_presentationModel->rowCount(QModelIndex()) != 0 ) {
        m_argumentHintTree->setCurrentIndex(QModelIndex());
        m_entryList->setCurrentIndex(m_presentationModel->index(0, 0));
        if(model()->hasGroups()) //If we have groups we have to move on, because the first item is a label
          m_entryList->nextCompletion();
      }
  }
  m_inCompletionList = !m_inCompletionList;
}

void KateCompletionWidget::showConfig( )
{
  abortCompletion();

  m_configWidget->exec();
}

void KateCompletionWidget::completionModelReset()
{
  KTextEditor::CodeCompletionModel* model = qobject_cast<KTextEditor::CodeCompletionModel*>(sender());
  if(!model) {
    kWarning() << "bad sender";
    return;
  }

  if(!m_waitingForReset.contains(model))
    return;

  m_waitingForReset.remove(model);

  if(m_waitingForReset.isEmpty()) {
    if(!isCompletionActive()) {
      kDebug() << "all completion-models we waited for are ready. Last one: " << model->objectName();
      //Eventually show the completion-list if this was the last model we were waiting for
      //Use a queued connection once again to make sure that KateCompletionModel is notified before we are
      QMetaObject::invokeMethod(this, "modelContentChanged", Qt::QueuedConnection);
    }
  }
}

void KateCompletionWidget::modelDestroyed(QObject* model) {
  unregisterCompletionModel(static_cast<KTextEditor::CodeCompletionModel*>(model));
}

void KateCompletionWidget::registerCompletionModel(KTextEditor::CodeCompletionModel* model)
{
  connect(model, SIGNAL(destroyed(QObject*)), SLOT(modelDestroyed(QObject*)));
  //This connection must not be queued
  connect(model, SIGNAL(modelReset()), SLOT(completionModelReset()));

  m_sourceModels.append(model);

  if (isCompletionActive()) {
    m_presentationModel->addCompletionModel(model);
  }
}

void KateCompletionWidget::unregisterCompletionModel(KTextEditor::CodeCompletionModel* model)
{
  disconnect(model, SIGNAL(destroyed(QObject*)), this, SLOT(modelDestroyed(QObject*)));
  disconnect(model, SIGNAL(modelReset()), this, SLOT(completionModelReset()));

  m_sourceModels.removeAll(model);
  abortCompletion();
}

int KateCompletionWidget::automaticInvocationDelay() const {
  return m_automaticInvocationDelay;
}

void KateCompletionWidget::setAutomaticInvocationDelay(int delay) {
  m_automaticInvocationDelay = delay;
}

void KateCompletionWidget::editDone(KateEditInfo * edit)
{
  if(!edit->newText().join("\n").trimmed().isEmpty())
  m_lastInsertionByUser = edit->editSource() == Kate::UserInputEdit;

  if (!view()->config()->automaticCompletionInvocation()
       || (edit->editSource() != Kate::UserInputEdit)
       || edit->isRemoval()
       || (edit->editSource() != Kate::UserInputEdit && edit->editSource() != Kate::CodeCompletionEdit)
       || edit->newText().isEmpty() )
  {
    m_automaticInvocationLine.clear();
    m_automaticInvocationTimer->stop();
    return;
  }

  if(m_automaticInvocationAt != edit->newRange().start()) {
    m_automaticInvocationLine.clear();
    m_lastInsertionByUser = edit->editSource() == Kate::UserInputEdit;
  }

  m_automaticInvocationLine += edit->newText().last();
  m_automaticInvocationAt = edit->newRange().end();

  if (m_automaticInvocationLine.isEmpty()) {
    m_automaticInvocationTimer->stop();
    return;
  }

  m_automaticInvocationTimer->start(m_automaticInvocationDelay);
}

void KateCompletionWidget::automaticInvocation()
{
  kDebug()<<"m_automaticInvocationAt:"<<m_automaticInvocationAt;
  kDebug()<<view()->cursorPosition();
  if(m_automaticInvocationAt != view()->cursorPosition())
    return;

  bool start = false;
  QList<KTextEditor::CodeCompletionModel*> models;

  kDebug()<<"checking models";
  foreach (KTextEditor::CodeCompletionModel *model, m_sourceModels) {
      kDebug()<<"m_completionRanges contains model?:"<<m_completionRanges.contains(model);
      if(m_completionRanges.contains(model))
        continue;

      start=_shouldStartCompletion(model,view(), m_automaticInvocationLine, m_lastInsertionByUser, view()->cursorPosition());
      kDebug()<<"start="<<start;
      if (start)
      {
        models << model;
      }
  }
  kDebug()<<"models found:"<<!models.isEmpty();
  if (!models.isEmpty()) {
    // Start automatic code completion
    startCompletion(KTextEditor::CodeCompletionModel::AutomaticInvocation, models);
  }
}

void KateCompletionWidget::userInvokedCompletion()
{
  startCompletion(KTextEditor::CodeCompletionModel::UserInvocation);
}

void KateCompletionWidget::tab(bool shift)
{
  m_noAutoHide = true;
  if(!shift) {
    QString prefix = m_presentationModel->commonPrefix((m_inCompletionList && !shellLikeTabCompletion) ? m_entryList->currentIndex() : QModelIndex());
    if(!prefix.isEmpty()) {
      view()->insertText(prefix);
    }else if(shellLikeTabCompletion) {
      cursorDown();
      return;
    }
  }else{
    if(shellLikeTabCompletion) {
      cursorUp();
      return;
    }

    //Reset left boundaries, so completion isn't stopped
    typedef QMap<KTextEditor::CodeCompletionModel*, CompletionRange> CompletionRangeMap;
    for(CompletionRangeMap::iterator it = m_completionRanges.begin(); it != m_completionRanges.end(); ++it)
      (*it).leftBoundary = (*it).range->start();

    //Remove suffix until the completion-list filter is widened again
    uint itemCount = m_presentationModel->filteredItemCount();

    while(view()->cursorPosition().column() > 0 && m_presentationModel->filteredItemCount() == itemCount) {
      KTextEditor::Range lastcharRange = KTextEditor::Range(view()->cursorPosition()-KTextEditor::Cursor(0,1), view()->cursorPosition());
      QString cursorText = view()->document()->text(lastcharRange);
      if(!cursorText[0].isSpace())
      {
        view()->document()->removeText(lastcharRange);
        QApplication::sendPostedEvents();
      }else{
        break;
      }
    }
  }
}

#include "katecompletionwidget.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
