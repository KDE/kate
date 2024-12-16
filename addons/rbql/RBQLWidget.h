/*
    SPDX-FileCopyrightText: 2024 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <KTextEditor/MainWindow>
#include <QFutureWatcher>
#include <QJSEngine>
#include <QJSValue>
#include <QStandardItemModel>
#include <QTabWidget>

class RBQLWidget : public QWidget
{
public:
    RBQLWidget(KTextEditor::MainWindow *mainWindow, QWidget *parent = nullptr);

    void newTab();

private:
    QTabWidget m_tabWidget;
    KTextEditor::MainWindow *m_mainWindow;
};

class RBQLTab : public QWidget
{
public:
    RBQLTab(KTextEditor::MainWindow *mainWindow, RBQLWidget *parent = nullptr);
    ~RBQLTab();

private:
    void exec();
    QStandardItemModel *execQuery(const QString &sep, QStringList lines, bool includeHeader);
    void onQueryExecuted();
    void initEngine();
    void reportError(const QString &error);
    QString getSeparatorForDocument() const;

    KTextEditor::MainWindow *m_mainWindow;
    class QLineEdit *m_queryLineEdit;
    class QLabel *m_errorLabel;
    class QCheckBox *m_hasHeader;
    class QPushButton *m_newTabBtn;
    class QPushButton *m_execBtn;
    class QTableView *m_tableView;
    std::unique_ptr<QJSEngine> m_engine;
    QFutureWatcher<QStandardItemModel *> m_queryExecutionFuture;
};
