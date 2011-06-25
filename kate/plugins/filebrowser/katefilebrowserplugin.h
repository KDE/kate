/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>
   Copyright (C) 2007 Mirko Stocker <me@misto.ch>
   Copyright (C) 2009 Dominik Haumann <dhaumann kde org>

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

#ifndef KATE_FILEBROWSER_PLUGIN_H
#define KATE_FILEBROWSER_PLUGIN_H


#include <ktexteditor/document.h>
#include <kate/plugin.h>
#include <kate/mainwindow.h>
#include <ktexteditor/configpage.h>
#include <kate/pluginconfigpageinterface.h>
#include <kurlcombobox.h>

#include <KVBox>
#include <KFile>
#include <KUrl>

class KActionCollection;
class KActionSelector;
class KDirOperator;
class KFileItem;
class KHistoryComboBox;
class KToolBar;
class QToolButton;
class QCheckBox;
class QSpinBox;

class KateFileBrowser;
class KateFileBrowserPluginView;

class KateFileBrowserPlugin: public Kate::Plugin, public Kate::PluginConfigPageInterface
{
    Q_OBJECT
    Q_INTERFACES(Kate::PluginConfigPageInterface)

  public:
    explicit KateFileBrowserPlugin( QObject* parent = 0, const QList<QVariant>& = QList<QVariant>() );
    virtual ~KateFileBrowserPlugin()
    {}

    Kate::PluginView *createView (Kate::MainWindow *mainWindow);

    virtual uint configPages() const;
    virtual Kate::PluginConfigPage *configPage (uint number = 0, QWidget *parent = 0, const char *name = 0);
    virtual QString configPageName (uint number = 0) const;
    virtual QString configPageFullName (uint number = 0) const;
    virtual KIcon configPageIcon (uint number = 0) const;
    
  public Q_SLOTS:
    void viewDestroyed(QObject* view);

  private:
    QList<KateFileBrowserPluginView *> m_views;
};

class KateFileBrowserPluginView : public Kate::PluginView
{
    Q_OBJECT

  public:
    /**
      * Constructor.
      */
    KateFileBrowserPluginView (Kate::MainWindow *mainWindow);

    /**
     * Virtual destructor.
     */
    ~KateFileBrowserPluginView ();

    virtual void readSessionConfig (KConfigBase* config, const QString& groupPrefix);
    virtual void writeSessionConfig (KConfigBase* config, const QString& groupPrefix);

  private:
    KateFileBrowser *m_fileBrowser;
    friend class KateFileBrowserPlugin;
};

#endif //KATE_FILEBROWSER_PLUGIN_H

// kate: space-indent on; indent-width 2; replace-tabs on;
