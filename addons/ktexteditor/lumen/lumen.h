/*
 * Copyright 2014  David Herberth kde@dav1d.de
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
**/

#ifndef LUMEN_H
#define LUMEN_H

#include <ktexteditor/plugin.h>
#include <ktexteditor/range.h>

#include <kpluginfactory.h>

#include <QVariantList>

#include <kaction.h>
#include <kxmlguiclient.h>
#include "dcd.h"
#include "completion.h"

using namespace KTextEditor;

class LumenPlugin;

class LumenPluginView: public QObject, public KXMLGUIClient
{
    Q_OBJECT
    public:
        LumenPluginView(LumenPlugin *plugin, KTextEditor::View *view);
        virtual ~LumenPluginView();
        void registerCompletion();
        void registerTextHints();
    private slots:
        void urlChanged(KTextEditor::Document*);
        void getTextHint(const KTextEditor::Cursor&, QString&);
    private:
        LumenPlugin *m_plugin;
        QPointer<KTextEditor::View> m_view;
        LumenCompletionModel *m_model;
        bool m_registered;
};

class LumenPlugin: public Plugin
{
    Q_OBJECT
    public:
        LumenPlugin(QObject *parent, const QVariantList & = QVariantList());
        ~LumenPlugin();
        DCD* dcd();
        void addView(View *view);
        void removeView(View *view);
        virtual void readConfig(KConfig*) {}
        virtual void writeConfig(KConfig*) {}
    private:
        QMap<KTextEditor::View*,LumenPluginView*> m_views;
        DCD* m_dcd;
};

K_PLUGIN_FACTORY_DECLARATION(LumenPluginFactory)

#endif
