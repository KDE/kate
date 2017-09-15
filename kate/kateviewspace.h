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

#ifndef KATE_VIEWSPACE_H
#define KATE_VIEWSPACE_H

#include <ktexteditor/view.h>
#include <ktexteditor/document.h>
#include <ktexteditor/modificationinterface.h>

#include <QHash>
#include <QWidget>

class KConfigBase;
class KateViewManager;
class QStackedWidget;
class QToolButton;
class KateTabBar;

class KateViewSpace : public QWidget
{
    Q_OBJECT

public:
    explicit KateViewSpace(KateViewManager *, QWidget *parent = nullptr, const char *name = nullptr);

    /**
     * Returns \e true, if this view space is currently the active view space.
     */
    bool isActiveSpace();

    /**
     * Depending on @p active, mark this view space as active or inactive.
     * Called from the view manager.
     */
    void setActive(bool active);

    /**
     * Create new view for given document
     * @param doc document to create view for
     * @return new created view
     */
    KTextEditor::View *createView(KTextEditor::Document *doc);
    void removeView(KTextEditor::View *v);

    bool showView(KTextEditor::View *view) {
        return showView(view->document());
    }
    bool showView(KTextEditor::Document *document);

    // might be nullptr, if there is no view
    KTextEditor::View *currentView();

    void saveConfig(KConfigBase *config, int myIndex, const QString &viewConfGrp);
    void restoreConfig(KateViewManager *viewMan, const KConfigBase *config, const QString &group);

    /**
     * Returns the document LRU list of this view space.
     */
    QVector<KTextEditor::Document*> lruDocumentList() const;

    /**
     * Called by the view manager if a viewspace was closed.
     * The documents of the closed are merged into this viewspace
     */
    void mergeLruList(const QVector<KTextEditor::Document*> & lruList);

    /**
     * Called by the view manager to notify that new documents were created
     * while this view space was active. If @p append is @e true, the @p doc
     * is appended to the lru document list, otherwise, it is prepended.
     */
    void registerDocument(KTextEditor::Document *doc, bool append = true);

    /**
     * Event filter to catch events from view space tool buttons.
     */
    bool eventFilter(QObject *obj, QEvent *event) override;

    /**
     * Focus the previous tab in the tabbar.
     */
    void focusPrevTab();

    /**
     * Focus the next tab in the tabbar.
     */
    void focusNextTab();

public Q_SLOTS:
    void documentDestroyed(QObject *doc);
    void updateDocumentName(KTextEditor::Document *doc);
    void updateDocumentUrl(KTextEditor::Document *doc);
    void updateDocumentState(KTextEditor::Document *doc);

private Q_SLOTS:
    void statusBarToggled();
    void tabBarToggled();
    void changeView(int buttonId);

    /**
     * Calls this slot to make this view space the currently active view space.
     * Making it active goes through the KateViewManager.
     * @param focusCurrentView if @e true, the current view will get focus
     */
    void makeActive(bool focusCurrentView = true);

    /**
     * Add a tab for @p doc at position @p index.
     */
    void insertTab(int index, KTextEditor::Document * doc);

    /**
     * Remove tab for @p doc, and return the index (position)
     * of the removed tab.
     */
    int removeTab(KTextEditor::Document * doc, bool documentDestroyed);

    /**
     * Remove @p count tabs, since the tab bar shrinked.
     */
    void removeTabs(int count);

    /**
     * Add @p count tabs, since the tab bar grew.
     */
    void addTabs(int count);

    /**
     * This slot is called by the tabbar, if tab @p id was closed through the
     * context menu.
     */
    void closeTabRequest(int id);

    /**
     * This slot is called when the context menu is requested for button
     * @p id at position @p globalPos.
     * @param id the button, or -1 if the context menu was requested on
     *        at a place where no tab exists
     * @param globalPos the position of the context menu in global coordinates
     */
    void showContextMenu(int id, const QPoint & globalPos);

    /**
     * Called to create a new empty document.
     */
    void createNewDocument();

    /**
     * Update the quick open button to reflect the currently hidden tabs.
     */
    void updateQuickOpen();

private:
    /**
     * Returns the amount of documents in KateDocManager that currently
     * have no tab in this tab bar.
     */
    int hiddenDocuments() const;

private:
    // Kate's view manager
    KateViewManager *m_viewManager;

    // config group string, used for restoring View session configuration
    QString m_group;

    // flag that indicates whether this view space is the active one.
    // correct setter: m_viewManager->setActiveSpace(this);
    bool m_isActiveSpace;

    // widget stack that contains all KTE::Views
    QStackedWidget *stack;

    // document's in the view space, sorted in LRU mode:
    // the most recently used Document is at the end of the list
    QVector<KTextEditor::Document *> m_lruDocList;

    // the list of views that are contained in this view space,
    // mapped through a hash from Document to View.
    // note: the number of entries match stack->count();
    QHash<KTextEditor::Document*, KTextEditor::View*> m_docToView;

    // tab bar that contains viewspace tabs
    KateTabBar *m_tabBar;
    
    // split action
    QToolButton *m_split;
    
    // quick open action
    QToolButton *m_quickOpen;

    // map from Document to button id
    QHash<KTextEditor::Document *, int> m_docToTabId;
};

#endif
