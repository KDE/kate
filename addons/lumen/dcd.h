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

#ifndef LUMEN_DCD_H
#define LUMEN_DCD_H

#include <QtCore/QDebug>
#include <QtCore/QProcess>
#include <QtCore/QFile>
#include <QtCore/QObject>
#include <QtCore/QMap>
#include <QtGui/QIcon>


namespace DCDCompletionType { enum DCDCompletionType { Identifiers, Calltips }; }
namespace DCDCompletionItemType {
    enum DCDCompletionItemType {
        Invalid,

        Calltip,

        ClassName,
        InterfaceName,
        StructName,
        UnionName,
        VariableName,
        MemberVariableName,
        Keyword,
        FunctionName,
        EnumName,
        EnumMember,
        PackageName,
        ModuleName,
    };

    char toChar(DCDCompletionItemType e);
    DCDCompletionItemType fromChar(char c);
}

struct DCDCompletionItem {
    DCDCompletionItem(DCDCompletionItemType::DCDCompletionItemType, QString);

    DCDCompletionItemType::DCDCompletionItemType type;
    QString name;

    QIcon icon() const;
    QString typeLong() const;
};

struct DCDCompletion {
    DCDCompletionType::DCDCompletionType type;
    QList<DCDCompletionItem> completions;
};

class DCD
{
    public:
        DCD(int, const QString&, const QString&);
        virtual ~DCD();
        int port() const;
        bool running();
        bool startServer();
        bool stopServer();
        DCDCompletion complete(const QString&, int);
        DCDCompletion complete(const QByteArray&, int);
        QString doc(const QByteArray&, int);
        void shutdown();
        void addImportPath(const QString&);
        void addImportPath(const QStringList&);
    private:
        DCDCompletion processCompletion(const QString&);
        int m_port;
        QString m_server;
        QString m_client;
        QProcess m_sproc;
};

#endif
