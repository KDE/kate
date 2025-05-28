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

class BookmarksPlugin : public KTextEditor::Plugin
{
public:
    explicit BookmarksPlugin(QObject *parent = nullptr, const QVariantList & = QVariantList());
    ~BookmarksPlugin() override;
    QObject *createView(KTextEditor::MainWindow *mainWindow) override;

private:
    BookmarksModel m_model;
};

class BookmarksPluginView : public QObject, public KXMLGUIClient
{
public:
    BookmarksPluginView(BookmarksPlugin *plugin, KTextEditor::MainWindow *mainWindow, BookmarksModel *model);
    ~BookmarksPluginView() override;

private:
    void appendDocumentMarks(KTextEditor::Document *document);
    KTextEditor::View *openBookmark(const Bookmark &bookmark);

private:
    void onViewChanged(KTextEditor::View *view);
    void onMarkChanged(KTextEditor::Document *document, KTextEditor::Mark mark, KTextEditor::Document::MarkChangeAction action);
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
