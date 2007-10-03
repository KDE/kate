/**
  * This file is part of the KDE libraries
  * Copyright (C) 2007 Rafael Fernández López <ereslibre@kde.org>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Library General Public
  * License version 2 as published by the Free Software Foundation.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Library General Public License for more details.
  *
  * You should have received a copy of the GNU Library General Public License
  * along with this library; see the file COPYING.LIB.  If not, write to
  * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  * Boston, MA 02110-1301, USA.
  */

#include "timedate.h"
#include "timedate_config.h"

#include <ktexteditor/document.h>

#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <klocale.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <kdatetime.h>

TimeDatePlugin *TimeDatePlugin::plugin = 0;

K_PLUGIN_FACTORY_DEFINITION(TimeDatePluginFactory,
        registerPlugin<TimeDatePlugin>();
        registerPlugin<TimeDateConfig>();
        )
K_EXPORT_PLUGIN(TimeDatePluginFactory("ktexteditor_timedate", "ktexteditor_plugins"))

TimeDatePlugin::TimeDatePlugin(QObject *parent, const QVariantList &args)
    : KTextEditor::Plugin(parent)
{
    Q_UNUSED(args);

    plugin = this;
    readConfig();
}

TimeDatePlugin::~TimeDatePlugin()
{
    plugin = 0;
}

void TimeDatePlugin::addView(KTextEditor::View *view)
{
    TimeDatePluginView *nview = new TimeDatePluginView(m_string, view);
    m_views.append(nview);
}

void TimeDatePlugin::removeView(KTextEditor::View *view)
{
    for (int z = 0; z < m_views.size(); z++)
    {
        if (m_views.at(z)->parentClient() == view)
        {
            TimeDatePluginView *nview = m_views.at(z);
            m_views.removeAll(nview);
            delete nview;
        }
    }
}

void TimeDatePlugin::readConfig()
{
    KConfigGroup cg(KGlobal::config(), "TimeDate Plugin");
    m_string = cg.readEntry("string", localizedTimeDate);
}

void TimeDatePlugin::writeConfig()
{
    KConfigGroup cg(KGlobal::config(), "TimeDate Plugin" );
    cg.writeEntry("string", m_string );
}

void TimeDatePlugin::setFormat(const QString &format)
{
    m_string = format;

    // If the property has been set for the plugin in general, let's set that
    // property to that value on all views where the plugin has been loaded.
    foreach (TimeDatePluginView *pluginView, m_views)
    {
        pluginView->setFormat(format);
    }
}

QString TimeDatePlugin::format() const
{
    return m_string;
}

TimeDatePluginView::TimeDatePluginView(const QString &string,
                                       KTextEditor::View *view)
  : QObject(view)
  , KXMLGUIClient(view)
  , m_view(view)
  , m_string(string)
{
    view->insertChildClient(this);
    setComponentData(TimeDatePluginFactory::componentData());

    KAction *action = new KAction(i18n("Insert Time && Date"), this);
    actionCollection()->addAction("tools_insert_timedate", action);
    action->setShortcut(Qt::CTRL + Qt::Key_D);
    connect(action, SIGNAL(triggered()), this, SLOT(slotInsertTimeDate()));

    setXMLFile("timedateui.rc");
}

TimeDatePluginView::~TimeDatePluginView()
{
}

void TimeDatePluginView::setFormat(const QString &format)
{
    m_string = format;
}

QString TimeDatePluginView::format() const
{
    return m_string;
}

void TimeDatePluginView::slotInsertTimeDate()
{
    KDateTime dt(QDateTime::currentDateTime());
    m_view->document()->insertText(m_view->cursorPosition(), dt.toString(m_string));
}

#include "timedate.moc"
