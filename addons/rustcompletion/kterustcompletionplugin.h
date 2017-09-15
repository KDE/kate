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

#ifndef KTERUSTCOMPLETIONPLUGIN_H
#define KTERUSTCOMPLETIONPLUGIN_H

#include "kterustcompletion.h"

#include <QUrl>
#include <QVariant>

#include <KTextEditor/Plugin>

class KDirWatch;

class KTERustCompletionPlugin : public KTextEditor::Plugin
{
    Q_OBJECT

    public:
        explicit KTERustCompletionPlugin(QObject *parent = nullptr, const QList<QVariant> & = QList<QVariant>());
        ~KTERustCompletionPlugin() override;

        QObject *createView(KTextEditor::MainWindow *mainWindow) override;

        int configPages() const override;
        KTextEditor::ConfigPage *configPage(int number = 0, QWidget *parent = nullptr) override;

        KTERustCompletion *completion();

        QString racerCmd() const;
        void setRacerCmd(const QString &cmd);

        QUrl rustSrcPath() const;
        void setRustSrcPath(const QUrl &path);

        bool configOk() const;

    private Q_SLOTS:
        void updateConfigOk();

    private:
        void readConfig();
        void writeConfig();

        KTERustCompletion m_completion;

        QString m_racerCmd;
        QUrl m_rustSrcPath;
        KDirWatch *m_rustSrcWatch;

        bool m_configOk;
};

#endif
