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

#include <QStatusBar>
#include <QVBoxLayout>
#include <QApplication>
#include <QDesktopWidget>
#include <QHeaderView>
#include <QTimer>

#include <QTreeWidget>
#include <QSortFilterProxyModel>

#include <ktexteditor/codecompletion2.h>
#include <ktexteditor/cursorfeedback.h>

#include "kateview.h"
#include "katesmartmanager.h"
#include "katerenderer.h"
#include "kateconfig.h"
#include "katefont.h"
#include "katecompletionmodel.h"
#include "katecompletiontree.h"

KateCompletionWidget::KateCompletionWidget(KateView* parent)
  : QFrame(parent, Qt::ToolTip)
  , m_sourceModel(0L)
  , m_completionRange(0L)
{
  setFrameStyle( QFrame::Box | QFrame::Plain );
  setLineWidth( 1 );
  //setWindowOpacity(0.8);

  m_entryList = new KateCompletionTree(this);

  m_statusBar = new QStatusBar(this);
  m_statusBar->setSizeGripEnabled(true);

  QVBoxLayout* vl = new QVBoxLayout(this);
  vl->addWidget(m_entryList);
  vl->addWidget(m_statusBar);
  vl->setMargin(0);

  m_presentationModel = new KateCompletionModel(this);
  m_entryList->setModel(m_presentationModel);

  // Keep branches expanded
  connect(m_presentationModel, SIGNAL(modelReset()), SLOT(modelReset()));
  connect(m_presentationModel, SIGNAL(rowsInserted(const QModelIndex&, int, int)), m_entryList, SLOT(expand(const QModelIndex&)));

  connect(view(), SIGNAL(cursorPositionChanged(KTextEditor::View*, const KTextEditor::Cursor&)), SLOT(cursorPositionChanged()));

  // This is a non-focus widget, it is passed keyboard input from the view
  setFocusPolicy(Qt::NoFocus);
  foreach (QWidget* childWidget, findChildren<QWidget*>())
    childWidget->setFocusPolicy(Qt::NoFocus);

  // Enable the cc box to move when the editor window is moved
  QApplication::activeWindow()->installEventFilter(this);
}

KateView * KateCompletionWidget::view( ) const
{
  return static_cast<KateView*>(const_cast<QObject*>(parent()));
}

void KateCompletionWidget::startCompletion( const KTextEditor::Range & word, KTextEditor::CodeCompletionModel * model )
{
  kdDebug() << k_funcinfo << word << " " << model << endl;

  if (!isCompletionActive())
    abortCompletion();

  m_sourceModel = model;

  m_completionRange = view()->doc()->smartManager()->newSmartRange(word);
  m_completionRange->setInsertBehaviour(KTextEditor::SmartRange::ExpandRight);

  connect(m_completionRange->smartStart().notifier(), SIGNAL(characterDeleted(KTextEditor::SmartCursor*, bool)), SLOT(startCharactedDeleted(KTextEditor::SmartCursor*, bool)));

  cursorPositionChanged();

  m_presentationModel->setSourceModel(m_sourceModel);

  updatePosition();

  if (isCompletionActive()) {
    show();
    m_entryList->expandAll();
    m_entryList->hideColumn(KTextEditor::CodeCompletionModel::Scope);
    m_entryList->resizeColumns();
  }
}

void KateCompletionWidget::updatePosition( )
{
  if (!isCompletionActive())
    return;

  QPoint p = view()->mapToGlobal( view()->cursorToCoordinate(m_completionRange->start()) );
  int x = p.x() - m_entryList->header()->sectionPosition(m_entryList->header()->visualIndex(KTextEditor::CodeCompletionModel::Name)) - 2;
  int y = p.y();
  if ( y + height() + view()->renderer()->config()->fontMetrics()->height() > QApplication::desktop()->height() )
    y -= height();
  else
    y += view()->renderer()->config()->fontMetrics()->height();

  if (x + width() > QApplication::desktop()->width())
    x = QApplication::desktop()->width() - width();

  move( QPoint(x,y) );
}

void KateCompletionWidget::cursorPositionChanged( )
{
  if (!isCompletionActive())
    return;

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
  return m_sourceModel && m_completionRange && m_completionRange->isValid();
}

void KateCompletionWidget::abortCompletion( )
{
  m_sourceModel = 0L;

  hide();

  m_presentationModel->setSourceModel(0L);

  delete m_completionRange;
  m_completionRange = 0L;
}

void KateCompletionWidget::execute()
{
  if (!isCompletionActive())
    return;

  if (!m_entryList->selectionModel()->currentIndex().isValid())
    return abortCompletion();

  m_sourceModel->executeCompletionItem(view()->document(), *m_completionRange, m_presentationModel->mapToSource(m_entryList->selectionModel()->currentIndex()).row());

  hide();
}

void KateCompletionWidget::resizeEvent( QResizeEvent * event )
{
  QWidget::resizeEvent(event);

  m_entryList->resizeColumns(true);
}

void KateCompletionWidget::hideEvent( QHideEvent * event )
{
  QWidget::hideEvent(event);

  //m_entryList->showColumn(KTextEditor::CodeCompletionModel::Scope);

  if (isCompletionActive())
    abortCompletion();
}

KateSmartRange * KateCompletionWidget::completionRange( ) const
{
  return m_completionRange;
}

void KateCompletionWidget::modelReset( )
{
  m_entryList->expandAll();
}

void KateCompletionWidget::startCharactedDeleted( KTextEditor::SmartCursor*, bool deletedBefore )
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

void KateCompletionWidget::nextCompletion( )
{
  m_entryList->nextCompletion();
}

void KateCompletionWidget::previousCompletion( )
{
  m_entryList->previousCompletion();
}

void KateCompletionWidget::pageDown( )
{
  m_entryList->pageDown();
}

void KateCompletionWidget::pageUp( )
{
  m_entryList->pageUp();
}

void KateCompletionWidget::top( )
{
  m_entryList->top();
}

void KateCompletionWidget::bottom( )
{
  m_entryList->bottom();
}

#include "katecompletionwidget.moc"
