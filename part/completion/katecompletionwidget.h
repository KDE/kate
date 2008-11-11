/* This file is part of the KDE libraries
   Copyright (C) 2005-2006 Hamish Rodda <rodda@kde.org>
   Copyright (C) 2007-2008 David Nolden <david.nolden.kdevelop@art-master.de>

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

namespace KTextEditor {
  class EmbeddedWidgetInterface;
}

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
        //Callbacks for keyboard-input, they return true when the event was handled, and should not be reached on.
    bool cursorLeft( bool shift );
    bool cursorRight( bool shift );
    void cursorDown(bool shift);
    void cursorUp(bool shift);
    
    void toggleExpanded();

    const KateCompletionModel* model() const;
    KateCompletionModel* model();
    
    void registerCompletionModel(KTextEditor::CodeCompletionModel* model);
    void unregisterCompletionModel(KTextEditor::CodeCompletionModel* model);
    
    int automaticInvocationDelay() const;
    void setAutomaticInvocationDelay(int delay);
    
    KateSmartRange* completionRange() const;

    // Navigation
    void pageDown();
    void pageUp();
    void top();
    void bottom();
    
    bool embeddedWidgetUp();
    bool embeddedWidgetDown();
    bool embeddedWidgetLeft();
    bool embeddedWidgetRight();
    bool embeddedWidgetAccept();
    bool embeddedWidgetBack();

    QWidget* currentEmbeddedWidget();
    
    bool canExpandCurrentItem() const;

    bool canCollapseCurrentItem() const;

    void setCurrentItemExpanded( bool );
    
    //Returns true if a screen border has been hit
    bool updatePosition(bool force = false);

    virtual bool eventFilter( QObject * watched, QEvent * event );

    KateArgumentHintTree* argumentHintTree() const;
    
    KateArgumentHintModel* argumentHintModel() const;

    ///Called by KateViewInternal, because we need the specific information from the event.
    
    void updateHeight();
    
  public Q_SLOTS:
    void abortCompletion();
    void showConfig();
/*    void viewFocusIn();
    void viewFocusOut();*/
    void updatePositionSlot();
    void automaticInvocation();

/*    void updateFocus();*/
    void argumentHintsChanged(bool hasContent);
    
  protected:
    virtual void showEvent ( QShowEvent * event );
    virtual void resizeEvent ( QResizeEvent * event );
    virtual void hideEvent ( QHideEvent * event );
//    virtual void focusInEvent ( QFocusEvent * event );

  private Q_SLOTS:
    void modelContentChanged();
    void cursorPositionChanged();
    void editDone(KateEditInfo* edit);
    void modelReset();
    void startCharacterDeleted(KTextEditor::SmartCursor* cursor, bool deletedBefore);
    void rowsInserted(const QModelIndex& parent, int row, int rowEnd);
    void viewFocusOut();
  private:
    void updateAndShow();
    void updateArgumentHintGeometry();
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
    //QTimer* m_updateFocusTimer;
    QWidget* m_statusBar;
    QToolButton* m_sortButton;
    QLabel* m_sortText;
    QToolButton* m_filterButton;
    QLabel* m_filterText;
    QPushButton* m_configButton;

    QString m_automaticInvocationLine;
    int m_automaticInvocationDelay;
    bool m_filterInstalled;

    class KateCompletionConfig* m_configWidget;
    bool m_inCompletionList; //Are we in the completion-list? If not, we're in the argument-hint list
    bool m_isSuspended;
    bool m_dontShowArgumentHints; //Used temporarily to prevent flashing
    bool m_needShow;

    int m_expandedAddedHeightBase;
    int m_expandingAddedHeight;
};

#endif
