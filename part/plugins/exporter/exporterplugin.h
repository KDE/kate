/**
 * This file is part of the KDE libraries
 * Copyright (C) 2009 Milian Wolff <mail@milianw.de>
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

#ifndef EXPORTERPLUGIN_H
#define EXPORTERPLUGIN_H

#include <ktexteditor/plugin.h>

#include <kpluginfactory.h>

#include <QtCore/QVariantList>

namespace KTextEditor {
class View;
}

class ExporterPluginView;

class ExporterPlugin
    : public KTextEditor::Plugin
{
  Q_OBJECT

  public:
    explicit ExporterPlugin(QObject *parent = 0, const QVariantList &args = QVariantList());
    virtual ~ExporterPlugin();

    virtual void addView (KTextEditor::View *view);
    virtual void removeView(KTextEditor::View* view);

  private:
    QMap<KTextEditor::View*, ExporterPluginView*> m_views;
};

K_PLUGIN_FACTORY_DECLARATION(ExporterPluginFactory)

#endif // EXPORTERPLUGIN_H

// kate: space-indent on; indent-width 2; replace-tabs on;
