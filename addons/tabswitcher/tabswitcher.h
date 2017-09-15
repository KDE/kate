/* This file is part of the KDE project

   Copyright (C) 2014 Dominik Haumann <dhaumann@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KTEXTEDITOR_TAB_SWITCHER_PLUGIN_H
#define KTEXTEDITOR_TAB_SWITCHER_PLUGIN_H

#include <KTextEditor/Plugin>
#include <KTextEditor/MainWindow>

#include <QList>
#include <QSet>
#include <QVariant>

#include <KXMLGUIClient>

class TabSwitcherPluginView;
class TabSwitcherTreeView;
class QStandardItemModel;
class QModelIndex;

class TabSwitcherPlugin : public KTextEditor::Plugin
{
    Q_OBJECT

    friend TabSwitcherPluginView;

public:
    /**
     * Plugin constructor.
     */
    explicit TabSwitcherPlugin(QObject *parent = nullptr, const QList<QVariant> & = QList<QVariant>());

    /**
     * Create a new tab switcher for @p mainWindow.
     */
    QObject *createView(KTextEditor::MainWindow *mainWindow) override;

private:
    QList<TabSwitcherPluginView *> m_views;
};

class TabSwitcherPluginView : public QObject, public KXMLGUIClient
{
    Q_OBJECT

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

public Q_SLOTS:
    /**
     * Adds @p document to the model.
     */
    void registerDocument(KTextEditor::Document * document);

    /**
     * Removes @p document from the model.
     */
    void unregisterDocument(KTextEditor::Document * document);

    /**
     * Update the name in the model for @p document.
     */
    void updateDocumentName(KTextEditor::Document * document);

    /**
     * Raise @p view in a lru fasion.
     */
    void raiseView(KTextEditor::View * view);

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
    void switchToClicked(const QModelIndex& index);

    /**
     * Show the document for @p index.
     */
    void activateView(const QModelIndex& index);

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
    TabSwitcherPlugin *m_plugin;
    KTextEditor::MainWindow *m_mainWindow;
    QStandardItemModel * m_model;
    QSet<KTextEditor::Document *> m_documents;
    TabSwitcherTreeView * m_treeView;
};

#endif // KTEXTEDITOR_TAB_SWITCHER_PLUGIN_H
