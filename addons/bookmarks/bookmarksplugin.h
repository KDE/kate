/*
    SPDX-FileCopyrightText: 2025 Leo Ruggeri <leo5t@yahoo.it>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <KConfig>
#include <KTextEditor/Document>
#include <KTextEditor/Plugin>
#include <KTextEditor/View>

#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QStandardItemModel>
#include <QVariant>

class BookmarksPlugin : public KTextEditor::Plugin
{
public:
    explicit BookmarksPlugin(QObject *parent = nullptr, const QVariantList & = QVariantList());
    ~BookmarksPlugin() override;
    QObject *createView(KTextEditor::MainWindow *mainWindow) override;
    void appendBookmark(int line, QString filepath);
    void removeBookmark(int line, QString filepath);

private:
    int getBookmarkId(int line, QString filepath);

private:
    KConfig m_metaInfos;
    QStandardItemModel m_model;
    QSet<int> m_marks;
};

class BookmarksPluginView : public QObject, public KXMLGUIClient
{
public:
    BookmarksPluginView(BookmarksPlugin *plugin, KTextEditor::MainWindow *mainWindow, QAbstractItemModel *model);
    ~BookmarksPluginView() override;

private:
    void appendDocumentMarks(KTextEditor::Document *document);
    KTextEditor::View *openBookmark(QModelIndex index);

private Q_SLOTS:
    void onViewChanged(KTextEditor::View *view);
    void onMarkChanged(KTextEditor::Document *document, KTextEditor::Mark mark, KTextEditor::Document::MarkChangeAction action);
    void onBackBtnClicked();
    void onNextBtnClicked();
    void onRemoveBtnClicked();

private:
    class QTreeView *m_treeView;
    QAbstractItemModel *m_model;
    QItemSelectionModel m_selectionModel;
    KTextEditor::MainWindow *m_mainWindow;
    std::unique_ptr<QWidget> m_toolView;
    class BookmarksPlugin *m_plugin;
};
