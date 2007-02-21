/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright (C) 2006 Dominik Haumann <dhdev@gmx.de>

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

#ifndef __KATE_VIEWSPACE_CONTAINER_H__
#define __KATE_VIEWSPACE_CONTAINER_H__

#include "katemain.h"

#include <KTextEditor/View>
#include <KTextEditor/Document>

#include <QSplitter>

#include <QList>
#include <QHash>

class KConfigGroup;
class KConfigBase;
class KateMainWindow;

class KateViewSpaceContainer: public QSplitter
{
    Q_OBJECT

    friend class KateViewSpace;
    friend class KateVSStatusBar;

  public:
    KateViewSpaceContainer (KateViewManager *viewManager, QWidget *parent = 0L);

    ~KateViewSpaceContainer ();

    inline QList<KTextEditor::View*> &viewList ()
    {
      return m_viewList;
    };

  public:
    /* This will save the splitter configuration */
    void saveViewConfiguration(KConfigGroup& group);

    /* restore it */
    void restoreViewConfiguration (KConfigGroup& group);

  private:
    /**
     * create and activate a new view for doc, if doc == 0, then
     * create a new document
     */
    bool createView ( KTextEditor::Document *doc = 0L );

    bool deleteView ( KTextEditor::View *view, bool delViewSpace = true);

    void moveViewtoSplit (KTextEditor::View *view);
    void moveViewtoStack (KTextEditor::View *view);

    /* Save the configuration of a single splitter.
     * If child splitters are found, it calls it self with those as the argument.
     * If a viewspace child is found, it is asked to save its filelist.
     */
    void saveSplitterConfig(QSplitter* s, KConfigBase* config, const QString& viewConfGrp);

    /** Restore a single splitter.
     * This is all the work is done for @see saveSplitterConfig()
     */
    void restoreSplitter ( KConfigBase* config, const QString &group, QSplitter* parent, const QString& viewConfGrp);

    void removeViewSpace (KateViewSpace *viewspace);

  public:
    KTextEditor::View* activeView ();
    KateViewSpace* activeViewSpace ();

    int viewCount () const;
    int viewSpaceCount () const;

    bool isViewActivationBlocked()
    {
      return m_blockViewCreationAndActivation;
    };

  public:
    void closeViews(KTextEditor::Document *doc);
    KateMainWindow *mainWindow();
    friend class KateViewManager;

  private Q_SLOTS:
    void activateView ( KTextEditor::View *view );
    void activateSpace ( KTextEditor::View* v );
    void slotViewChanged();
    void reactivateActiveView();
    void slotPendingDocumentNameChanged();

    void documentCreated (KTextEditor::Document *doc);
    void documentDeleted (KTextEditor::Document *doc);

  public Q_SLOTS:
    /**
     * Splits a KateViewSpace into two in the following steps:
     * 1. create a QSplitter in the parent of the KateViewSpace to be split
     * 2. move the to-be-splitted KateViewSpace to the new splitter
     * 3. create new KateViewSpace and added to the new splitter
     * 4. create KateView to populate the new viewspace.
     * 5. The new KateView is made the active one, because createView() does that.
     * If no viewspace is provided, the result of activeViewSpace() is used.
     * The orientation of the new splitter is determined by the value of o.
     * Note: horizontal splitter means vertically aligned views.
     */
    void splitViewSpace( KateViewSpace* vs = 0L, Qt::Orientation o = Qt::Horizontal );

    /**
     * activate view for given document
     * @param doc document to activate view for
     */
    void activateView ( KTextEditor::Document *doc );

    /** Splits the active viewspace horizontally */
    void slotSplitViewSpaceHoriz ()
    {
      splitViewSpace(0L, Qt::Vertical);
    }
    /** Splits the active viewspace vertically */
    void slotSplitViewSpaceVert ()
    {
      splitViewSpace();
    }

    void slotCloseCurrentViewSpace();

    void setActiveSpace ( KateViewSpace* vs );
    void setActiveView ( KTextEditor::View* view );

    void activateNextView();
    void activatePrevView();

  private Q_SLOTS:
    void titleMayChange ();

  Q_SIGNALS:
    void viewChanged ();
    void changeMyTitle (QWidget *page);

  private:
    KateViewManager *m_viewManager;
    QList<KateViewSpace*> m_viewSpaceList;
    QList<KTextEditor::View*> m_viewList;
    QHash<KTextEditor::View*, bool> m_activeStates;

    bool m_blockViewCreationAndActivation;

    bool m_activeViewRunning;

    bool m_pendingViewCreation;
    QPointer<KTextEditor::Document> m_pendingDocument;

    int m_splitterIndex; // used during saving splitter config.
};

#endif
// kate: space-indent on; indent-width 2; replace-tabs on;

