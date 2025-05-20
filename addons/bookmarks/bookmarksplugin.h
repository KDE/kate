/*
    SPDX-FileCopyrightText: 2025 Leo Ruggeri <leo5t@yahoo.it>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <KConfig>
#include <KTextEditor/Document>
#include <KTextEditor/Plugin>
#include <KTextEditor/View>

#include <QItemSelectionModel>
#include <QStandardItemModel>
#include <QVariant>

class BookmarksPlugin : public KTextEditor::Plugin
{
public:
    explicit BookmarksPlugin(QObject *parent = nullptr, const QVariantList & = QVariantList());
    ~BookmarksPlugin() override;
    QObject *createView(KTextEditor::MainWindow *mainWindow) override;
};

class BookmarksPluginView : public QObject, public KXMLGUIClient
{
public:
    BookmarksPluginView(BookmarksPlugin *plugin, KTextEditor::MainWindow *mainWindow);
    ~BookmarksPluginView() override;

private:
    int getBookmarkId(int line, QString filepath);
    void appendBookmark(int line, QString filepath);
    void removeBookmark(int line, QString filepath);
    void appendDocumentMarks(KTextEditor::Document *document);
    KTextEditor::View *openBookmark(QModelIndex index);

private Q_SLOTS:
    void onViewChanged(KTextEditor::View *view);
    void onMarkChanged(KTextEditor::Document *document, KTextEditor::Mark mark, KTextEditor::Document::MarkChangeAction action);
    void onBackBtnClicked();
    void onNextBtnClicked();
    void onRemoveBtnClicked();

private:
    KConfig m_metaInfos;
    class QTreeView *m_treeView;
    QStandardItemModel m_model;
    QItemSelectionModel m_selectionModel;
    KTextEditor::MainWindow *m_mainWindow;
    std::unique_ptr<QWidget> m_toolView;
    QSet<int> m_marks;
};
