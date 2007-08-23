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

#include <QtGui/QFrame>
#include <QObject>

#include <ktexteditor/range.h>
#include <ktexteditor/codecompletioninterface.h>
#include <ktexteditor/codecompletionmodel.h>

class QToolButton;
class QPushButton;
class QLabel;
class QTimer;

class KateView;
class KateSmartRange;
class KateCompletionModel;
class KateCompletionTree;
class KateEditInfo;
class KateArgumentHintTree;
class KateArgumentHintModel;

/**
 * This is the code completion's main widget, and also contains the
 * core interface logic.
 *
 * @author Hamish Rodda <rodda@kde.org>
 */
class KateCompletionWidget : public QFrame
{
  Q_OBJECT

  public:
    explicit KateCompletionWidget(KateView* parent);
    ~KateCompletionWidget();

    KateView* view() const;
    KateCompletionTree* treeView() const;

    bool isCompletionActive() const;
    void startCompletion(const KTextEditor::Range& word, KTextEditor::CodeCompletionModel* model, KTextEditor::CodeCompletionModel::InvocationType invocationType = KTextEditor::CodeCompletionModel::ManualInvocation);
    void userInvokedCompletion();

    //Executed when return is pressed while completion is active. With shift, only the item selected within the expanding-widget will be executed
    void execute(bool shift);
        //Callbacks for keyboard-input, they return true when the event should be handled exclusively
    bool cursorLeft( bool shift );
    bool cursorRight( bool shift );
    void cursorDown(bool shift);
    void cursorUp(bool shift);

    const KateCompletionModel* model() const;
    KateCompletionModel* model();
    
    void registerCompletionModel(KTextEditor::CodeCompletionModel* model);
    void unregisterCompletionModel(KTextEditor::CodeCompletionModel* model);
    
    bool isAutomaticInvocationEnabled() const;
    void setAutomaticInvocationEnabled(bool enabled = true);

    int automaticInvocationDelay() const;
    void setAutomaticInvocationDelay(int delay);
    
    KateSmartRange* completionRange() const;

    // Navigation
    void pageDown();
    void pageUp();
    void top();
    void bottom();
    
    bool canExpandCurrentItem() const;

    bool canCollapseCurrentItem() const;

    void setCurrentItemExpanded( bool );
    
    void updatePosition(bool force = false);

    virtual bool eventFilter( QObject * watched, QEvent * event );

    KateArgumentHintTree* argumentHintTree() const;
    
    KateArgumentHintModel* argumentHintModel() const;
    
  public Q_SLOTS:
    void abortCompletion();
    void showConfig();
    void focusOut();
    void focusIn();
    void updatePositionSlot();
    void automaticInvocation();

  protected:
    virtual void showEvent ( QShowEvent * event );
    virtual void resizeEvent ( QResizeEvent * event );
    virtual void hideEvent ( QHideEvent * event );

  private Q_SLOTS:
    void cursorPositionChanged();
    void editDone(KateEditInfo* edit);
    void modelReset();
    void startCharacterDeleted(KTextEditor::SmartCursor* cursor, bool deletedBefore);
    void rowsInserted(const QModelIndex& parent, int row, int rowEnd);

  private:
    void updateHeight();
    QModelIndex selectedIndex() const;

    void clear();
    //Switch cursor between argument-hint list / completion-list
    void switchList();
    KTextEditor::Range determineRange() const;

    QList<KTextEditor::CodeCompletionModel*> m_sourceModels;
    KateCompletionModel* m_presentationModel;

    KateSmartRange* m_completionRange;
    KTextEditor::Cursor m_lastCursorPosition;

    KateCompletionTree* m_entryList;
    KateArgumentHintModel* m_argumentHintModel;
    KateArgumentHintTree* m_argumentHintTree;

    QTimer* m_automaticInvocationTimer;
    QWidget* m_statusBar;
    QToolButton* m_sortButton;
    QLabel* m_sortText;
    QToolButton* m_filterButton;
    QLabel* m_filterText;
    QPushButton* m_configButton;

    QString m_automaticInvocationLine;
    bool m_automaticInvocation;
    int m_automaticInvocationDelay;
    bool m_filterInstalled;

    class KateCompletionConfig* m_configWidget;
    bool m_inCompletionList; //Are we in the completion-list? If not, we're in the argument-hint list
    bool m_isSuspended;
    bool m_dontShowArgumentHints; //Used temporarily to prevent flashing

    int m_expandedAddedHeightBase;
    int m_expandingAddedHeight;
};

#endif
