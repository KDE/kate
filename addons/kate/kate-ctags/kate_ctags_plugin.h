#ifndef KATE_CTAGS_PLUGIN_H
#define KATE_CTAGS_PLUGIN_H
/* Description : Kate CTags plugin
 *
 * Copyright (C) 2008-2011 by Kare Sars <kare.sars@iki.fi>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <ktexteditor/view.h>
#include <ktexteditor/document.h>

#include <kate/plugin.h>
#include <kate/application.h>
#include <kate/documentmanager.h>
#include <kate/mainwindow.h>
#include <kate/pluginconfigpageinterface.h>

#include "kate_ctags_view.h"
#include "ui_CTagsGlobalConfig.h"

//******************************************************************/
class KateCTagsPlugin : public Kate::Plugin, public Kate::PluginConfigPageInterface
{
    Q_OBJECT
    Q_INTERFACES(Kate::PluginConfigPageInterface)

    public:
        explicit KateCTagsPlugin(QObject* parent = 0, const QList<QVariant> & = QList<QVariant>());
        virtual ~KateCTagsPlugin() {}

        Kate::PluginView *createView(Kate::MainWindow *mainWindow);
   
        // PluginConfigPageInterface
        uint configPages() const { return 1; };
        Kate::PluginConfigPage *configPage (uint number = 0, QWidget *parent = 0, const char *name = 0);
        QString configPageName (uint number = 0) const;
        QString configPageFullName (uint number = 0) const;
        KIcon configPageIcon (uint number = 0) const;
        void readConfig();
        
        KateCTagsView *m_view;
};

//******************************************************************/
class KateCTagsConfigPage : public Kate::PluginConfigPage {
    Q_OBJECT
public:
    explicit KateCTagsConfigPage( QWidget* parent = 0, KateCTagsPlugin *plugin = 0 );
    ~KateCTagsConfigPage() {}
    
    void apply();
    void reset();
    void defaults() {}

private Q_SLOTS:
    void addGlobalTagTarget();
    void delGlobalTagTarget();
    void updateGlobalDB();
    void updateDone(int exitCode, QProcess::ExitStatus status);

private:

    bool listContains(const QString &target);

    KProcess              m_proc;
    KateCTagsPlugin      *m_plugin;
    Ui_CTagsGlobalConfig  m_confUi;
};

#endif

