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

#ifndef KTERUSTCOMPLETIONCONFIGPAGE_H
#define KTERUSTCOMPLETIONCONFIGPAGE_H

#include <KTextEditor/ConfigPage>

class KTERustCompletionPlugin;

class QLineEdit;

class KUrlRequester;

class KTERustCompletionConfigPage : public KTextEditor::ConfigPage
{
    Q_OBJECT

    public:
        explicit KTERustCompletionConfigPage(QWidget *parent = nullptr, KTERustCompletionPlugin *plugin = nullptr);
        ~KTERustCompletionConfigPage() {};

        QString name() const Q_DECL_OVERRIDE;
        QString fullName() const Q_DECL_OVERRIDE;
        QIcon icon() const Q_DECL_OVERRIDE;

    public Q_SLOTS:
        void apply() Q_DECL_OVERRIDE;
        void defaults() Q_DECL_OVERRIDE;
        void reset() Q_DECL_OVERRIDE;

    private Q_SLOTS:
        void changedInternal();

    private:
        QLineEdit *m_racerCmd;
        KUrlRequester *m_rustSrcPath;

        bool m_changed;

        KTERustCompletionPlugin *m_plugin;
};

#endif
