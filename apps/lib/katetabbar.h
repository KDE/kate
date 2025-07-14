/*
    SPDX-FileCopyrightText: 2014 Dominik Haumann <dhaumann@kde.org>
    SPDX-FileCopyrightText: 2020 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "doc_or_widget.h"
#include "kateprivate_export.h"

#include <QTabBar>

#include <unordered_map>

namespace KTextEditor
{
class Document;
}

/**
 * The \p KateTabBar class provides a tab bar, e.g. for tabbed documents.
 *
 * The API closely follows the API of QTabBar.
 *
 * @author Dominik Haumann
 */
class KATE_PRIVATE_EXPORT KateTabBar : public QTabBar
{
    Q_OBJECT
    Q_PROPERTY(bool isActive READ isActive WRITE setActive)

public:
    explicit KateTabBar(QWidget *parent = nullptr);

    /**
     * Read and apply tab limit as configured
     */
    void readConfig();

    void tabInserted(int idx) override;

    /**
     * Get the ID of the tab that is located left of the current tab.
     * The return value is -1, if there is no previous tab.
     */
    int prevTab() const;

    /**
     * Get the ID of the tab that is located right of the current tab.
     * The return value is -1, if there is no next tab.
     */
    int nextTab() const;

    /**
     * Returns whether a tab with ID \a id exists.
     */
    bool containsTab(int index) const;

    QVariant ensureValidTabData(int idx);

    void setCurrentDocument(DocOrWidget);
    int documentIdx(DocOrWidget);
    void setTabDocument(int idx, DocOrWidget doc);
    DocOrWidget tabDocument(int idx);
    void removeDocument(DocOrWidget doc);
    void setModifiedStateIcon(int idx, KTextEditor::Document *doc);

    /**
     * Marks this tabbar as active. That is, current-tab indicators are
     * properly highlighted, indicating that child widgets of this tabbar
     * will get input.
     *
     * This concept is mostly useful, if your application has multiple tabbars.
     * Inactive tabbars are grayed out.
     */
    void setActive(bool active);

    /**
     * Returns whether this tabbar is active.
     */
    bool isActive() const;

    /**
     * Returns the document list of this tab bar.
     * @return document list in order of tabs
     */
    QList<DocOrWidget> documentList() const;

    /**
     * Returns LRU sorted list of the documents registered for this tab bar.
     * Used for session saving.
     * @return lru sorted list of all registered documents
     */
    QList<KTextEditor::Document *> lruSortedDocuments() const;

    bool openTabsToRightOfCurrent() const
    {
        return m_openNewTabInFrontOfCurrent;
    }
    /**
     * Set whether new tabs open to the right of current tab
     *
     * Disabled during session restore as it can mess up the
     * order of tabs there
     */
    void setOpenTabsToRightOfCurrentEnabled(bool e)
    {
        m_openNewTabInFrontOfCurrent = e;
    }

Q_SIGNALS:
    /**
     * This signal is emitted whenever the context menu is requested for
     * button @p id at position @p globalPos.
     * @param id the button, or -1 if the context menu was requested on
     *        at a place where no tab exists
     * @param globalPos the position of the context menu in global coordinates
     */
    void contextMenuRequest(int id, const QPoint &globalPos);

    /**
     * This signal is emitted whenever the users double clicks on the free
     * space next to the tab bar. Typically, a new document should be
     * created.
     */
    void newTabRequested();

    /**
     * This signal is emitted whenever the tab bar was clicked while the
     * tab bar is not the active view space tab bar.
     */
    void activateViewSpaceRequested();

protected:
    //! Override to avoid requesting a new tab.
    void mouseDoubleClickEvent(QMouseEvent *event) override;

    //! Override to request making the tab bar active.
    void mousePressEvent(QMouseEvent *event) override;

    void mouseMoveEvent(QMouseEvent *) override;

    //! Request context menu
    void contextMenuEvent(QContextMenuEvent *ev) override;

private:
    using QTabBar::addTab;
    using QTabBar::insertTab;

    bool m_isActive = false;
    DocOrWidget m_beingAdded = static_cast<QWidget *>(nullptr);

    /**
     * limit of number of tabs we should keep
     * default unlimited == 0
     */
    int m_tabCountLimit = 0;

    /**
     * next lru counter value for a tab
     */
    quint64 m_lruCounter = 0;

    /**
     * should a double click create a new document?
     */
    bool m_doubleClickNewDocument = false;

    /*
     * should a middle click close a document?
     */
    bool m_middleClickCloseDocument = false;

    /**
     * Should the new tab open in front of current tab
     */
    bool m_openNewTabInFrontOfCurrent = false;

    /**
     * Should we go the first tab on nextTab()
     */
    bool m_cycleTabs = true;

    /**
     * LRU counter storage, to determine which document has which age
     * simple 64-bit counter, worst thing that can happen on 64-bit wraparound
     * is a bit strange tab replacement a few times
     */
    std::unordered_map<DocOrWidget, std::pair<quint64, bool>> m_docToLruCounterAndHasTab;

    QPointF dragStartPos;
    QPointF dragStartGlobalPos;
    QPointF dragHotspotPos;
};
