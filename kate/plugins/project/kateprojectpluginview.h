/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2010 Christoph Cullmann <cullmann@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#ifndef _PLUGIN_KATE_PROJECTVIEW_H_
#define _PLUGIN_KATE_PROJECTVIEW_H_

#include "plugin_kateproject.h"
#include "kateproject.h"

#include <QToolBox>

class KateProjectPluginView : public Kate::PluginView, public Kate::XMLGUIClient
{
    Q_OBJECT

  public:
    KateProjectPluginView( KateProjectPlugin *plugin, Kate::MainWindow *mainWindow );
    ~KateProjectPluginView();

    virtual void readSessionConfig( KConfigBase* config, const QString& groupPrefix );
    virtual void writeSessionConfig( KConfigBase* config, const QString& groupPrefix );

  public slots:
    void slotInsertHello();
    
  private:
    /**
     * our plugin
     */
    KateProjectPlugin *m_plugin;
    
    /**
     * our projects toolview
     */
    QWidget *m_toolView;
    
    /**
     * the toolbox for the projects
     */
    QToolBox *m_toolBox;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;

