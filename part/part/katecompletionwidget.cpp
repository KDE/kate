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

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>
#include <QDesktopWidget>
#include <QHeaderView>
#include <QTimer>
#include <QLabel>
#include <QToolButton>
#include <QSizeGrip>
#include <QPushButton>

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

KateCompletionWidget::KateCompletionWidget(KateView* parent)
  : QFrame(parent, Qt::ToolTip)
  , m_presentationModel(new KateCompletionModel(this))
  , m_completionRange(0L)
  , m_automaticInvocation(false)
  , m_filterInstalled(false)
{
  setFrameStyle( QFrame::Box | QFrame::Plain );
  setLineWidth( 1 );
  //setWindowOpacity(0.8);

  m_entryList = new KateCompletionTree(this);
  m_entryList->setModel(m_presentationModel);

  m_statusBar = new QWidget(this);
  m_statusBar->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed));

  m_sortButton = new QToolButton(m_statusBar);
  m_sortButton->setIcon(KIcon("sort"));
  m_sortButton->setCheckable(true);
  connect(m_sortButton, SIGNAL(toggled(bool)), m_presentationModel, SLOT(setSortingEnabled(bool)));

  m_sortText = new QLabel(i18n("Sort: None"), m_statusBar);

  m_filterButton = new QToolButton(m_statusBar);
  m_filterButton->setIcon(KIcon("filter"));
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
  connect(m_presentationModel, SIGNAL(modelReset()), SLOT(modelReset()));
  connect(m_presentationModel, SIGNAL(rowsInserted(const QModelIndex&, int, int)), m_entryList, SLOT(expand(const QModelIndex&)));

  connect(view(), SIGNAL(cursorPositionChanged(KTextEditor::View*, const KTextEditor::Cursor&)), SLOT(cursorPositionChanged()));
  connect(view()->doc()->history(), SIGNAL(editDone(KateEditInfo*)), SLOT(editDone(KateEditInfo*)));

  // This is a non-focus widget, it is passed keyboard input from the view
  setFocusPolicy(Qt::NoFocus);
  foreach (QWidget* childWidget, findChildren<QWidget*>())
    childWidget->setFocusPolicy(Qt::NoFocus);
}

KateView * KateCompletionWidget::view( ) const
{
  return static_cast<KateView*>(const_cast<QObject*>(parent()));
}

void KateCompletionWidget::startCompletion( const KTextEditor::Range & word, KTextEditor::CodeCompletionModel * model, KTextEditor::CodeCompletionModel::InvocationType invocationType)
{
  kDebug(13035) << k_funcinfo << word << " " << model << endl;

  if (!m_filterInstalled) {
    if (!QApplication::activeWindow()) {
      kWarning(13035) << k_funcinfo << "No active window to install event filter on!!" << endl;
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

  connect(m_completionRange->smartStart().notifier(), SIGNAL(characterDeleted(KTextEditor::SmartCursor*, bool)), SLOT(startCharactedDeleted(KTextEditor::SmartCursor*, bool)));

  cursorPositionChanged();

  if (model)
    m_presentationModel->setCompletionModel(model);
  else
    m_presentationModel->setCompletionModels(m_sourceModels);

  updatePosition();

  if (!m_presentationModel->completionModels().isEmpty()) {
    show();
    m_entryList->expandAll();
    m_entryList->resizeColumns();

    foreach (KTextEditor::CodeCompletionModel* model, m_presentationModel->completionModels())
      model->completionInvoked(view(), word, invocationType);
  }
}

void KateCompletionWidget::updatePosition( )
{
  if (!isCompletionActive())
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
  bool ret = !isHidden();
  if (ret)
    Q_ASSERT(m_completionRange && m_completionRange->isValid());

  return ret;
}

void KateCompletionWidget::abortCompletion( )
{
  kDebug(13035) << k_funcinfo << endl;

  hide();

  m_presentationModel->clearCompletionModels();

  delete m_completionRange;
  m_completionRange = 0L;
}

void KateCompletionWidget::execute()
{
  if (!isCompletionActive())
    return;

  QModelIndex toExecute = m_entryList->selectionModel()->currentIndex();

  if (!toExecute.isValid())
    return abortCompletion();

  toExecute = m_presentationModel->mapToSource(toExecute);

  // encapsulate all editing as being from the code completion, and undo-able in one step.
  view()->doc()->editStart(Kate::CodeCompletionEdit);

  static_cast<const KTextEditor::CodeCompletionModel*>(toExecute.model())->executeCompletionItem(view()->document(), *m_completionRange, toExecute.row());

  view()->doc()->editEnd();

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
      kWarning(13035) << k_funcinfo << "Completion range was invalid even though it was expected to be valid." << endl;
  }
}

void KateCompletionWidget::userInvokedCompletion()
{
  startCompletion(determineRange(), 0, KTextEditor::CodeCompletionModel::UserInvocation);
}

KTextEditor::Range KateCompletionWidget::determineRange() const
{
  KTextEditor::Cursor end = view()->cursorPosition();

  if (!end.column())
    return KTextEditor::Range::invalid();

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
