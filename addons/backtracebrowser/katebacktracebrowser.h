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
    explicit KateBtBrowserPlugin(QObject *parent = 0, const QList<QVariant> & = QList<QVariant>());
    virtual ~KateBtBrowserPlugin();

    static KateBtBrowserPlugin &self();

    QObject *createView(KTextEditor::MainWindow *mainWindow) Q_DECL_OVERRIDE;

    KateBtDatabase &database();
    BtFileIndexer &fileIndexer();

    void startIndexer();

Q_SIGNALS:
    void newStatus(const QString &);

public:
    int configPages() const Q_DECL_OVERRIDE;
    KTextEditor::ConfigPage *configPage(int number, QWidget *parent = 0) Q_DECL_OVERRIDE;

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
    ~KateBtBrowserPluginView();

private:
    KateBtBrowserPlugin *m_plugin;
    KateBtBrowserWidget *m_widget;
};

class KateBtBrowserWidget : public QWidget, public Ui::BtBrowserWidget
{
    Q_OBJECT

public:
    KateBtBrowserWidget(KTextEditor::MainWindow *mainwindow, QWidget *parent);

    ~KateBtBrowserWidget();

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
    explicit KateBtConfigWidget(QWidget *parent = 0);
    virtual ~KateBtConfigWidget();

    QString name() const Q_DECL_OVERRIDE;
    QString fullName() const Q_DECL_OVERRIDE;
    QIcon icon() const Q_DECL_OVERRIDE;

public Q_SLOTS:
    void apply() Q_DECL_OVERRIDE;
    void reset() Q_DECL_OVERRIDE;
    void defaults() Q_DECL_OVERRIDE;

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
    KateBtConfigDialog(QWidget *parent = 0);
    ~KateBtConfigDialog();

private:
    KateBtConfigWidget *m_configWidget;
};

#endif //KATE_BACKTRACEBROWSER_H

// kate: space-indent on; indent-width 4; replace-tabs on;
