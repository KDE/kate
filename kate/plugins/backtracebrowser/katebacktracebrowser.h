/* This file is part of the KDE project
   Copyright 2008 Dominik Haumann <dhaumann kde org>

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

#include <ktexteditor/document.h>
#include <ktexteditor/configpage.h>
#include <kate/plugin.h>
#include <kate/pluginconfigpageinterface.h>
#include <kate/mainwindow.h>

#include "ui_btbrowserwidget.h"
#include "ui_btconfigwidget.h"
#include "btdatabase.h"
#include "btfileindexer.h"

#include <QString>
#include <QTimer>

class KateBtBrowserPlugin: public Kate::Plugin, public Kate::PluginConfigPageInterface
{
    Q_OBJECT
    Q_INTERFACES(Kate::PluginConfigPageInterface)
  public:
    explicit KateBtBrowserPlugin( QObject* parent = 0, const QList<QVariant>& = QList<QVariant>() );
    virtual ~KateBtBrowserPlugin();

    static KateBtBrowserPlugin& self();

    Kate::PluginView *createView (Kate::MainWindow *mainWindow);

    KateBtDatabase& database();
    BtFileIndexer& fileIndexer();

    void startIndexer();

  signals:
    void newStatus(const QString&);

  //
  // PluginConfigPageInterface
  //
  public:
    virtual uint configPages() const;
    virtual Kate::PluginConfigPage* configPage (uint number = 0, QWidget *parent = 0, const char *name = 0);
    virtual QString configPageName(uint number = 0) const;
    virtual QString configPageFullName(uint number = 0) const;
    virtual KIcon configPageIcon(uint number = 0) const;

  //
  // private data
  //
  private:
    KateBtDatabase db;
    BtFileIndexer indexer;
    static KateBtBrowserPlugin* s_self;
};

class KateBtBrowserPluginView : public Kate::PluginView, public Ui::BtBrowserWidget
{
    Q_OBJECT

  public:
    KateBtBrowserPluginView(Kate::MainWindow* mainWindow);

    ~KateBtBrowserPluginView();

    virtual void readSessionConfig (KConfigBase* config, const QString& groupPrefix);
    virtual void writeSessionConfig (KConfigBase* config, const QString& groupPrefix);

    void setStatus(const QString& status);

    void loadBacktrace(const QString& bt);

  public slots:
    void loadFile();
    void loadClipboard();
    void configure();
    void clearStatus();

  private slots:
    void itemActivated(QTreeWidgetItem* item, int column);

  private:
    QWidget* toolView;
    Kate::MainWindow* mw;
    QTimer timer;
};

class KateBtConfigWidget : public Kate::PluginConfigPage, private Ui::BtConfigWidget
{
    Q_OBJECT
  public:
    explicit KateBtConfigWidget(QWidget* parent = 0, const char* name = 0);
    virtual ~KateBtConfigWidget();

  public slots:
    virtual void apply();
    virtual void reset();
    virtual void defaults();

  private slots:
    void add();
    void remove();
    void textChanged();

  private:
    bool m_changed;
};

class KateBtConfigDialog : public KDialog
{
    Q_OBJECT
  public:
    KateBtConfigDialog(QWidget* parent = 0);
    ~KateBtConfigDialog();

  public slots:
    void changed();

private:
    KateBtConfigWidget* m_configWidget;
};

#endif //KATE_BACKTRACEBROWSER_H

// kate: space-indent on; indent-width 2; replace-tabs on;
