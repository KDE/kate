/*
 * Copyright 2014-2015  David Herberth kde@dav1d.de
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

#include <KTextEditor/Plugin>
#include <KTextEditor/Range>
#include <KTextEditor/TextHintInterface>

#include <QVariantList>
#include <QSet>

#include <KXMLGUIClient>
#include "dcd.h"
#include "completion.h"

using namespace KTextEditor;

class LumenPlugin;
class LumenHintProvider;

class LumenPluginView: public QObject
{
    Q_OBJECT
    public:
        LumenPluginView(LumenPlugin *plugin, KTextEditor::MainWindow *view);
        ~LumenPluginView() override;
        void registerCompletion(KTextEditor::View *view);
        void registerTextHints(KTextEditor::View *view);
    private Q_SLOTS:
        void urlChanged(KTextEditor::Document*);
        void viewCreated(KTextEditor::View *view);
        void viewDestroyed(QObject *view);
        void documentChanged(KTextEditor::Document *document);
    private:
        LumenPlugin* m_plugin;
        MainWindow* m_mainWin;
        LumenCompletionModel* m_model;
        QSet<View *> m_completionViews;
        bool m_registered;
        LumenHintProvider* m_hinter;
};

class LumenHintProvider : public KTextEditor::TextHintProvider
{
public:
  explicit LumenHintProvider(LumenPlugin* plugin);
  QString textHint(KTextEditor::View* view, const KTextEditor::Cursor& position) override;

private:
  LumenPlugin *m_plugin;
};


class LumenPlugin: public Plugin
{
    Q_OBJECT
    public:
        explicit LumenPlugin(QObject *parent = nullptr, const QList<QVariant> & = QList<QVariant>());
        ~LumenPlugin() override;
        DCD* dcd();
        QObject *createView(MainWindow *mainWindow) override;
    private:
        DCD* m_dcd;
};


#endif
