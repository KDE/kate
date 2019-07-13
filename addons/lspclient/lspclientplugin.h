/***************************************************************************
 *   Copyright (C) 2015 by Eike Hein <hein@kde.org>                        *
 *   Copyright (C) 2019 by Mark Nauwelaerts <mark.nauwelaerts@gmail.com>   *
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

#ifndef LSPCLIENTPLUGIN_H
#define LSPCLIENTPLUGIN_H

#include <QUrl>
#include <QVariant>
#include <QMap>

#include <KTextEditor/Plugin>

class LSPClientPlugin : public KTextEditor::Plugin
{
    Q_OBJECT

    public:
        explicit LSPClientPlugin(QObject *parent = nullptr, const QList<QVariant> & = QList<QVariant>());
        ~LSPClientPlugin() override;

        QObject *createView(KTextEditor::MainWindow *mainWindow) override;

        int configPages() const override;
        KTextEditor::ConfigPage *configPage(int number = 0, QWidget *parent = nullptr) override;

        void readConfig();
        void writeConfig() const;

        // settings
        bool m_symbolDetails;
        bool m_symbolExpand;
        bool m_symbolTree;
        bool m_symbolSort;
        bool m_complDoc;
        bool m_refDeclaration;
        bool m_diagnostics;
        bool m_diagnosticsHighlight;
        bool m_diagnosticsMark;
        QUrl m_configPath;

private:
    Q_SIGNALS:
        // signal settings update
        void update() const;
};

#endif
