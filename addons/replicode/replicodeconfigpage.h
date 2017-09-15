/* This file is part of the KDE project
   Copyright (C) 2014 Martin Sandsmark <martin.sandsmark@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
 
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
 
   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#ifndef REPLICODEPLUGINCONFIGPAGE_H
#define REPLICODEPLUGINCONFIGPAGE_H
#include <KTextEditor/ConfigPage>

class KUrlRequester;
class ReplicodeConfig;

class ReplicodeConfigPage : public KTextEditor::ConfigPage
{
    Q_OBJECT

public:
    ReplicodeConfigPage(QWidget *parent = nullptr);

    QString name() const override;
    QString fullName() const override;

public Q_SLOTS:
    void apply() override;
    void reset() override;
    void defaults() override;

private:
    KUrlRequester *m_requester;
    ReplicodeConfig *m_config;
};

#endif//REPLICODEPLUGINCONFIGPAGE_H
