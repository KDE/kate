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
#ifndef KATECOMPLETIONWIDGET_H
#define KATECOMPLETIONWIDGET_H

#include <QFrame>
#include <QModelIndex>

#include <ktexteditor/range.h>

class QTreeView;
class QStatusBar;
class QSortFilterProxyModel;

class KateView;
class KateSmartRange;
class KateCompletionModel;
class KateCompletionTree;

namespace KTextEditor {
  class CodeCompletionModel;
}

/**
 * @author Hamish Rodda <rodda@kde.org>
 */
class KateCompletionWidget : public QFrame
{
  Q_OBJECT

  public:
    KateCompletionWidget(KateView* parent);

    KateView* view() const;

    bool isCompletionActive() const;
    void startCompletion(const KTextEditor::Range& word, KTextEditor::CodeCompletionModel* model);
    void execute();

    KateSmartRange* completionRange() const;

    // Navigation
    void nextCompletion();
    void previousCompletion();
    void pageDown();
    void pageUp();
    void top();
    void bottom();

    void updatePosition();

    virtual bool eventFilter( QObject * watched, QEvent * event );

  public slots:
    void abortCompletion();

  protected:
    virtual void resizeEvent ( QResizeEvent * event );
    virtual void hideEvent ( QHideEvent * event );

  private slots:
    void cursorPositionChanged();
    void modelReset();
    void startCharactedDeleted(KTextEditor::SmartCursor* cursor, bool deletedBefore);

  private:
    KTextEditor::CodeCompletionModel* m_sourceModel;
    KateCompletionModel* m_presentationModel;

    KateSmartRange* m_completionRange;
    KTextEditor::Cursor m_lastCursorPosition;

    KateCompletionTree* m_entryList;
    QStatusBar* m_statusBar;
    QTreeView* m_testTree;
};

#endif
