/***************************************************************************
 *   Copyright (C) 2015 by Eike Hein <hein@kde.org>                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#ifndef KTERUSTCOMPLETIONPLUGINVIEW_H
#define KTERUSTCOMPLETIONPLUGINVIEW_H

#include <QObject>
#include <QSet>

#include <KXMLGUIClient>

class KTERustCompletionPlugin;

namespace KTextEditor {
    class Document;
    class MainWindow;
    class View;
}

class KTERustCompletionPluginView : public QObject, public KXMLGUIClient
{
    Q_OBJECT

    public:
        KTERustCompletionPluginView(KTERustCompletionPlugin *plugin, KTextEditor::MainWindow *mainWindow);
        ~KTERustCompletionPluginView() override;

    private Q_SLOTS:
        void goToDefinition();
        void viewChanged();
        void viewCreated(KTextEditor::View *view);
        void viewDestroyed(QObject *view);
        void documentChanged(KTextEditor::Document *document);

    private:
        void registerCompletion(KTextEditor::View *view);

        static bool isRustView(const KTextEditor::View *view);

        KTERustCompletionPlugin *m_plugin;
        KTextEditor::MainWindow *m_mainWindow;
        QSet<KTextEditor::View *> m_completionViews;
};

#endif

