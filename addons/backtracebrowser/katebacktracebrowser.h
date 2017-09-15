/* This file is part of the KDE project
   Copyright 2008-2014 Dominik Haumann <dhaumann kde org>

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

#ifndef KATE_BACKTRACEBROWSER_H
#define KATE_BACKTRACEBROWSER_H

#include <KTextEditor/Plugin>
#include <ktexteditor/mainwindow.h>
#include <ktexteditor/configpage.h>

#include "ui_btbrowserwidget.h"
#include "ui_btconfigwidget.h"
#include "btdatabase.h"
#include "btfileindexer.h"

#include <QString>
#include <QTimer>
#include <QDialog>


class KateBtConfigWidget;
class KateBtBrowserWidget;

class KateBtBrowserPlugin : public KTextEditor::Plugin
{
    Q_OBJECT

public:
    explicit KateBtBrowserPlugin(QObject *parent = nullptr, const QList<QVariant> & = QList<QVariant>());
    ~KateBtBrowserPlugin() override;

    static KateBtBrowserPlugin &self();

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;

    KateBtDatabase &database();
    BtFileIndexer &fileIndexer();

    void startIndexer();

Q_SIGNALS:
    void newStatus(const QString &);

public:
    int configPages() const override;
    KTextEditor::ConfigPage *configPage(int number, QWidget *parent = nullptr) override;

    //
    // private data
    //
private:
    KateBtDatabase db;
    BtFileIndexer indexer;
    static KateBtBrowserPlugin *s_self;
};

class KateBtBrowserPluginView : public QObject
{
    Q_OBJECT

public:
    KateBtBrowserPluginView(KateBtBrowserPlugin *plugin, KTextEditor::MainWindow *mainWindow);

    /**
    * Virtual destructor.
    */
    ~KateBtBrowserPluginView() override;

private:
    KateBtBrowserPlugin *m_plugin;
    KateBtBrowserWidget *m_widget;
};

class KateBtBrowserWidget : public QWidget, public Ui::BtBrowserWidget
{
    Q_OBJECT

public:
    KateBtBrowserWidget(KTextEditor::MainWindow *mainwindow, QWidget *parent);

    ~KateBtBrowserWidget() override;

    void loadBacktrace(const QString &bt);

public Q_SLOTS:
    void loadFile();
    void loadClipboard();
    void configure();
    void clearStatus();
    void setStatus(const QString &status);

private Q_SLOTS:
    void itemActivated(QTreeWidgetItem *item, int column);

private:
    KTextEditor::MainWindow *mw;
    QTimer timer;
};

class KateBtConfigWidget : public KTextEditor::ConfigPage, private Ui::BtConfigWidget
{
    Q_OBJECT
public:
    explicit KateBtConfigWidget(QWidget *parent = nullptr);
    ~KateBtConfigWidget() override;

    QString name() const override;
    QString fullName() const override;
    QIcon icon() const override;

public Q_SLOTS:
    void apply() override;
    void reset() override;
    void defaults() override;

private Q_SLOTS:
    void add();
    void remove();
    void textChanged();

private:
    bool m_changed;
};

class KateBtConfigDialog : public QDialog
{
    Q_OBJECT
public:
    KateBtConfigDialog(QWidget *parent = nullptr);
    ~KateBtConfigDialog() override;

private:
    KateBtConfigWidget *m_configWidget;
};

#endif //KATE_BACKTRACEBROWSER_H

// kate: space-indent on; indent-width 4; replace-tabs on;
