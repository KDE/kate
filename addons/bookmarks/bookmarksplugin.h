/*
    SPDX-FileCopyrightText: 2025 Leo Ruggeri <leo5t@yahoo.it>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "bookmarksmodel.h"

#include <KTextEditor/Document>
#include <KTextEditor/Plugin>
#include <KTextEditor/View>

#include <QItemSelectionModel>
#include <QSortFilterProxyModel>
#include <QUrl>

class BookmarksPlugin : public KTextEditor::Plugin
{
public:
    explicit BookmarksPlugin(QObject *parent);
    ~BookmarksPlugin() override;

public:
    QObject *createView(KTextEditor::MainWindow *mainWindow) override;

private:
    void registerDocument(KTextEditor::Document *document);
    void syncDocumentBookmarks(KTextEditor::Document *document);
    void onDocumentUrlChanged(KTextEditor::Document *document);
    void onDocumentAboutToClose(KTextEditor::Document *document);
    void onDocumentModifiedOnDisk(KTextEditor::Document *doc, bool changed, KTextEditor::Document::ModifiedOnDiskReason reason);

private:
    BookmarksModel m_model;
    QHash<KTextEditor::Document *, QUrl> m_urls;
};

class BookmarksPluginView : public QObject, public KXMLGUIClient
{
public:
    BookmarksPluginView(BookmarksPlugin *plugin, KTextEditor::MainWindow *mainWindow, BookmarksModel *model);
    ~BookmarksPluginView() override;

private:
    KTextEditor::View *openBookmark(const Bookmark &bookmark);

private:
    void onBookmarkClicked(const QModelIndex &index);
    void onBackBtnClicked();
    void onNextBtnClicked();
    void onRemoveBtnClicked();

private:
    BookmarksModel *m_model;
    QSortFilterProxyModel m_proxyModel;
    QItemSelectionModel m_selectionModel;
    KTextEditor::MainWindow *m_mainWindow;
    std::unique_ptr<QWidget> m_toolView;
    class QTreeView *m_treeView;
};
