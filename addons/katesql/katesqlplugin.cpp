/*
   Copyright (C) 2010  Marco Mentasti  <marcomentasti@gmail.com>

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

#include "katesqlplugin.h"
#include "katesqlconfigpage.h"
#include "katesqlview.h"

#include <ktexteditor/document.h>

#include <kpluginloader.h>
#include <kaboutdata.h>
#include <klocalizedstring.h>

#include <QIcon>

K_PLUGIN_FACTORY_WITH_JSON(KateSQLFactory, "katesql.json", registerPlugin<KateSQLPlugin>();)

//BEGIN KateSQLPLugin
KateSQLPlugin::KateSQLPlugin(QObject *parent, const QList<QVariant>&)
    : KTextEditor::Plugin (parent)
{
}


KateSQLPlugin::~KateSQLPlugin()
{
}


QObject *KateSQLPlugin::createView (KTextEditor::MainWindow *mainWindow)
{
  KateSQLView *view = new KateSQLView(this, mainWindow);

  connect(this, &KateSQLPlugin::globalSettingsChanged, view, &KateSQLView::slotGlobalSettingsChanged);

  return view;
}


KTextEditor::ConfigPage* KateSQLPlugin::configPage(int number, QWidget *parent)
{
  if (number != 0)
    return nullptr;

  KateSQLConfigPage *page = new KateSQLConfigPage(parent);
  connect(page, &KateSQLConfigPage::settingsChanged, this, &KateSQLPlugin::globalSettingsChanged);

  return page;
}

//END KateSQLPlugin

#include "katesqlplugin.moc"
