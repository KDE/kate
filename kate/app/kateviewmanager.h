/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>

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

#ifndef __KATE_VIEWMANAGER_H__
#define __KATE_VIEWMANAGER_H__

#include "katemain.h"

#include <KTextEditor/View>
#include <KTextEditor/Document>

#include <QPointer>
#include <QList>
#include <QModelIndex>

class KConfigGroup;
class KateMainWindow;
class KateViewSpaceContainer;

class QAction;

class QToolButton;

class KateViewManager : public QObject
{
    Q_OBJECT

  public:
    KateViewManager (KateMainWindow *parent);
    ~KateViewManager ();

    KateViewSpaceContainer *activeContainer ()
    {
      return m_currentContainer;
    }

    QList<KateViewSpaceContainer *> *containers()
    {
      return &m_viewSpaceContainerList;
    }

    void updateViewSpaceActions ();

  private:
    /**
     * create all actions needed for the view manager
     */
    void setupActions ();

  public:
    /* This will save the splitter configuration */
    void saveViewConfiguration(KConfigGroup& group);

    /* restore it */
    void restoreViewConfiguration (const KConfigGroup& group);

    KTextEditor::Document *openUrl (const KUrl &url, const QString& encoding, bool activate = true, bool isTempFile = false);

    KTextEditor::View *openUrlWithView (const KUrl &url, const QString& encoding);

  public Q_SLOTS:
    void openUrl (const KUrl &url);

  private:
    void removeViewSpace (KateViewSpace *viewspace);

  public:
    KTextEditor::View* activeView ();
    KateViewSpace* activeViewSpace ();

    uint viewCount ();
    uint viewSpaceCount ();

    void setViewActivationBlocked (bool block);

  public:
    void closeViews(KTextEditor::Document *doc);
    KateMainWindow *mainWindow();

    KTextEditor::View *activateView ( KTextEditor::Document *doc );

  private Q_SLOTS:
    void activateView ( KTextEditor::View *view );
    void activateSpace ( KTextEditor::View* v );

    void tabChanged(QWidget*);

  public Q_SLOTS:
    void activateDocument(const QModelIndex &index);

    void slotDocumentNew ();
    void slotDocumentOpen ();
    void slotDocumentClose ();

    /** Splits the active viewspace horizontally */
    void slotSplitViewSpaceHoriz ();
    /** Splits the active viewspace vertically */
    void slotSplitViewSpaceVert ();

    void slotNewTab();
    void slotCloseTab ();
    void activateNextTab ();
    void activatePrevTab ();

    void slotCloseCurrentViewSpace();

    void setActiveSpace ( KateViewSpace* vs );
    void setActiveView ( KTextEditor::View* view );

    void activateNextView();
    void activatePrevView();

  protected:
    friend class KateViewSpaceContainer;

    QPointer<KTextEditor::View> guiMergedView;

  protected Q_SLOTS:
    void changeMyTitle (QWidget *page);

  Q_SIGNALS:
    void statChanged ();
    void viewChanged ();

  private:
    QList<KateViewSpaceContainer *> m_viewSpaceContainerList;
    KateViewSpaceContainer *m_currentContainer;

    KateMainWindow *m_mainWindow;
    bool m_init;

    QToolButton *m_closeTabButton;
    QAction *m_closeView;
    QAction *m_closeTab;
    QAction *m_activateNextTab;
    QAction *m_activatePrevTab;
    QAction *goNext;
    QAction *goPrev;
};

#endif
// kate: space-indent on; indent-width 2; replace-tabs on;

