/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2002 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2007 Mirko Stocker <me@misto.ch>
   Copyright (C) 2008 Rafael Fernández López <ereslibre@kde.org>
 
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

#include <kpluginselector.h>

#include "kateconfigplugindialogpage.h"
#include "kateconfigplugindialogpage.moc"

#include "katepluginmanager.h"
#include "kateconfigdialog.h"

KateConfigPluginPage::KateConfigPluginPage(QList<KPluginInfo> &pluginInfoList, QWidget *parent, KateConfigDialog *dialog)
    : KVBox(parent)
    , myDialog(dialog)
    , pluginSelector(new KPluginSelector(this))
{
  connect(pluginSelector, SIGNAL(changed(bool)), this, SIGNAL(changed()));
  connect(pluginSelector, SIGNAL(changed(bool)), this, SLOT(stateChange()));

  if (pluginInfoList.count()) {
    pluginSelector->addPlugins(pluginInfoList, KPluginSelector::IgnoreConfigFile, i18n("Kate Plugins"));
  }
}

void KateConfigPluginPage::stateChange()
{
  pluginSelector->updatePluginsState();
}
