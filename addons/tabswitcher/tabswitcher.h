/* This file is part of the KDE project

   SPDX-FileCopyrightText: 2014 Dominik Haumann <dhaumann@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <KTextEditor/MainWindow>
#include <KTextEditor/Plugin>

#include <doc_or_widget.h>

#include <QList>
#include <QTimer>

#include <KXMLGUIClient>
#include <unordered_set>

class TabSwitcherPluginView;
class TabSwitcherTreeView;
class QStandardItemModel;
class QModelIndex;
namespace detail
{
class TabswitcherFilesModel;
}

class TabSwitcherPlugin : public KTextEditor::Plugin
{
    friend TabSwitcherPluginView;

public:
    explicit TabSwitcherPlugin(QObject *parent);

    /**
     * Create a new tab switcher for @p mainWindow.
     */
    QObject *createView(KTextEditor::MainWindow *mainWindow) override;

private:
    QList<TabSwitcherPluginView *> m_views;
};

class TabSwitcherPluginView : public QObject, public KXMLGUIClient
{
public:
    /**
     * View constructor.
     */
    TabSwitcherPluginView(TabSwitcherPlugin *plugin, KTextEditor::MainWindow *mainWindow);

    /**
     * View destructor.
     */
    ~TabSwitcherPluginView() override;

    /**
     * Setup the shortcut actions.
     */
    void setupActions();

    /**
     * Initial fill of model with documents from the application.
     */
    void setupModel();

public:
    /**
     * Adds @p widget to the model.
     */
    void onWidgetCreated(QWidget *widget);

    /**
     * Removes @p widget from the model.
     */
    void onWidgetRemoved(QWidget *widget);

    /**
     * Adds @p document to the model.
     */
    void registerDocuments(const QList<KTextEditor::Document *> &documents);

    /**
     * Removes @p document from the model.
     */
    void unregisterDocument(KTextEditor::Document *document);

    /**
     * Update the name in the model for @p document.
     */
    void updateDocumentName(KTextEditor::Document *document);

    /**
     * Raise @p view in a lru fashion.
     */
    void raiseView(KTextEditor::View *view);

    /**
     * Focus next item in the treeview.
     */
    void walkForward();

    /**
     * Focus previous item in the treeview.
     */
    void walkBackward();

    /**
     * Activate the document for @p index.
     */
    void switchToClicked(const QModelIndex &index);

    /**
     * Show the document for @p index.
     */
    void activateView(const QModelIndex &index);

    /**
     * Closes the current view
     */
    void closeView();

protected:
    /**
     * Move through the list.
     */
    void walk(const int from, const int to);

    /**
     * Make sure the popup view has a sane size.
     */
    void updateViewGeometry();

private:
    void registerItem(DocOrWidget docOrWidget);
    void unregisterItem(DocOrWidget docOrWidget);

    TabSwitcherPlugin *m_plugin;
    KTextEditor::MainWindow *m_mainWindow;
    detail::TabswitcherFilesModel *m_model;
    std::unordered_set<DocOrWidget> m_documents;
    TabSwitcherTreeView *m_treeView;
    QList<KTextEditor::Document *> m_documentsPendingAdd;
    QTimer m_documentsCreatedTimer;
};
