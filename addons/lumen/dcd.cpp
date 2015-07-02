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

#include "dcd.h"
#include <QtCore/QDebug>
#include <QtCore/QProcess>
#include <QtCore/QFile>
#include <QtCore/QRegularExpression>
#include <QtGui/QIcon>


char DCDCompletionItemType::toChar(DCDCompletionItemType e)
{
    switch (e) {
        case Invalid: return 0;
        case Calltip: return 1;
        case ClassName: return 'c';
        case InterfaceName: return 'i';
        case StructName: return 's';
        case UnionName: return 'u';
        case VariableName: return 'v';
        case MemberVariableName: return 'm';
        case Keyword: return 'k';
        case FunctionName: return 'f';
        case EnumName: return 'g';
        case EnumMember: return 'e';
        case PackageName: return 'p';
        case ModuleName: return 'M';
    }

    return 0;
}

DCDCompletionItemType::DCDCompletionItemType DCDCompletionItemType::fromChar(char c)
{
    switch (c) {
        case 0: return Invalid;
        case 1: return Calltip;
        case 'c': return ClassName;
        case 'i': return InterfaceName;
        case 's': return StructName;
        case 'u': return UnionName;
        case 'v': return VariableName;
        case 'm': return MemberVariableName;
        case 'k': return Keyword;
        case 'f': return FunctionName;
        case 'g': return EnumName;
        case 'e': return EnumMember;
        case 'p': return PackageName;
        case 'M': return ModuleName;
    }

    return Invalid;
}



DCDCompletionItem::DCDCompletionItem(DCDCompletionItemType::DCDCompletionItemType t, QString s): type(t), name(s)
{

}

#define RETURN_CACHED_ICON(name) {static QIcon icon(QIcon::fromTheme(QStringLiteral(name)).pixmap(QSize(16, 16))); return icon;}
QIcon DCDCompletionItem::icon() const
{
    using namespace DCDCompletionItemType;
    switch (type)
    {
        case Invalid: break;
        case Calltip: RETURN_CACHED_ICON("code-function")
        case ClassName: RETURN_CACHED_ICON("code-class")
        case InterfaceName: RETURN_CACHED_ICON("code-class")
        case StructName: RETURN_CACHED_ICON("struct")
        case UnionName: RETURN_CACHED_ICON("union")
        case VariableName: RETURN_CACHED_ICON("code-variable")
        case MemberVariableName: RETURN_CACHED_ICON("field")
        case Keyword: RETURN_CACHED_ICON("field")
        case FunctionName: RETURN_CACHED_ICON("code-function")
        case EnumName: RETURN_CACHED_ICON("enum")
        case EnumMember: RETURN_CACHED_ICON("enum")
        case PackageName: RETURN_CACHED_ICON("field")
        case ModuleName: RETURN_CACHED_ICON("field")
    }

    return QIcon();
}

QString DCDCompletionItem::typeLong() const
{
    using namespace DCDCompletionItemType;
    switch (type)
    {
        case Invalid: return QStringLiteral("invalid");
        case Calltip: return QStringLiteral("calltip");
        case ClassName: return QStringLiteral("class");
        case InterfaceName: return QStringLiteral("interface");
        case StructName: return QStringLiteral("struct");
        case UnionName: return QStringLiteral("union");
        case VariableName: return QStringLiteral("variable");
        case MemberVariableName: return QStringLiteral("member");
        case Keyword: return QStringLiteral("keyword");
        case FunctionName: return QStringLiteral("function");
        case EnumName: return QStringLiteral("enum");
        case EnumMember: return QStringLiteral("enum member");
        case PackageName: return QStringLiteral("package");
        case ModuleName: return QStringLiteral("module");
    }

    return QStringLiteral("completion");
}


static const int TIMEOUT_START_SERVER = 200;
static const int TIMEOUT_COMPLETE = 200;
static const int TIMEOUT_IMPORTPATH = 200;
static const int TIMEOUT_SHUTDOWN = 350;
static const int TIMEOUT_SHUTDOWN_SERVER = 200;


DCD::DCD(int port, const QString& server, const QString& client)
{
    m_port = port;
    m_server = server;
    m_client = client;
}

int DCD::port() const
{
    return m_port;
}

bool DCD::running()
{
    return m_sproc.state() == QProcess::Running;
}


bool DCD::startServer()
{
    m_sproc.setProcessChannelMode(QProcess::MergedChannels);
    m_sproc.start(m_server, QStringList(QStringLiteral("-p%1").arg(m_port)));
    bool started = m_sproc.waitForStarted(TIMEOUT_START_SERVER);
    bool finished = m_sproc.waitForFinished(TIMEOUT_START_SERVER);

    if (!started || finished || m_sproc.state() == QProcess::NotRunning) {
        qWarning() << "unable to start completion-server:" << m_sproc.exitCode();
        qWarning() << m_sproc.readAllStandardOutput();
        return false;
    }
    qDebug() << "started completion-server";
    return true;
}


DCDCompletion DCD::complete(const QString& file, int offset)
{
    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start(m_client,
        QStringList()
            << QStringLiteral("-p%1").arg(m_port)
            << QStringLiteral("-c%1").arg(offset)
            << file
    );
    proc.waitForFinished(TIMEOUT_COMPLETE);
    proc.terminate();

    if (proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0) {
        qWarning() << "unable to complete:" << proc.exitCode();
        qWarning() << proc.readAllStandardOutput();
        return DCDCompletion();
    }

    return processCompletion(QString::fromUtf8(proc.readAllStandardOutput()));
}

DCDCompletion DCD::complete(const QByteArray& data, int offset)
{
    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start(m_client,
        QStringList()
            << QStringLiteral("-p%1").arg(m_port)
            << QStringLiteral("-c%1").arg(offset)
    );
    proc.write(data);
    proc.closeWriteChannel();
    if (!proc.waitForFinished(TIMEOUT_COMPLETE)) {
        qWarning() << "unable to complete: client didn't finish in time";
        proc.close();
    } else if (proc.exitCode() != 0) {
        qWarning() << "unable to complete:" << proc.exitCode();
        qWarning() << proc.readAllStandardOutput();
    } else {
        // everything Ok
        return processCompletion(QString::fromUtf8(proc.readAllStandardOutput()));
    }

    return DCDCompletion();
}

QString DCD::doc(const QByteArray& data, int offset)
{
    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start(m_client,
        QStringList()
            << QStringLiteral("-p%1").arg(m_port)
            << QStringLiteral("-c%1").arg(offset)
            << QStringLiteral("--doc")
    );

    proc.write(data);
    proc.closeWriteChannel();
    if (!proc.waitForFinished(TIMEOUT_COMPLETE)) {
        qWarning() << "unable to lookup documentation: client didn't finish in time";
        proc.close();
    } else if (proc.exitCode() != 0) {
        qWarning() << "unable to lookup documentation:" << proc.exitCode();
        qWarning() << proc.readAllStandardOutput();
    } else {
        return QString::fromUtf8(proc.readAllStandardOutput());
    }

    return QString();
}


DCDCompletion DCD::processCompletion(const QString& data)
{
    DCDCompletion completion;

    QStringList lines = data.split(QRegularExpression(QStringLiteral("[\r\n]")), QString::SkipEmptyParts);
    if (lines.length() == 0) {
        return completion;
    }

    QString type = lines.front();
    if (type == QStringLiteral("identifiers")) { completion.type = DCDCompletionType::Identifiers; }
    else if (type == QStringLiteral("calltips")) { completion.type = DCDCompletionType::Calltips; }
    else {
        qWarning() << "Invalid type:" << type;
        return completion;
    }
    lines.pop_front();

    foreach(QString line, lines) {
        if (line.trimmed().length() == 0) {
            continue;
        }

        QStringList kv = line.split(QRegularExpression(QStringLiteral("\\s+")), QString::SkipEmptyParts);
        if (kv.length() != 2 && completion.type != DCDCompletionType::Calltips) {
            qWarning() << "invalid completion data:" << kv.length() << completion.type;
            continue;
        }

        if (completion.type == DCDCompletionType::Identifiers) {
            completion.completions.append(DCDCompletionItem(
                DCDCompletionItemType::fromChar(kv[1].at(0).toLatin1()), kv[0]
            ));
        } else {
            completion.completions.append(DCDCompletionItem(
                DCDCompletionItemType::Calltip, line
            ));
        }
    }

    return completion;
}


void DCD::addImportPath(const QString& path)
{
    addImportPath(QStringList(path));
}

void DCD::addImportPath(const QStringList& paths)
{
    if (paths.isEmpty()) {
        return;
    }

    QStringList arguments = QStringList(QStringLiteral("-p%1").arg(m_port));
    foreach(QString path, paths) {
        if (QFile::exists(path))
            arguments << QStringLiteral("-I%1").arg(path);
    }

    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start(m_client, arguments);
    proc.waitForFinished(TIMEOUT_IMPORTPATH);

    if (proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0) {
        qWarning() << "unable to add importpath(s)" << paths << ":" << proc.exitCode();
        qWarning() << proc.readAll();
    }
}

void DCD::shutdown()
{
    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);
    proc.start(m_client,
        QStringList()
            << QStringLiteral("-p%1").arg(m_port)
            << QStringLiteral("--shutdown")
    );
    proc.waitForFinished(TIMEOUT_SHUTDOWN);

    if (proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0) {
        qWarning() << "unable to shutdown dcd:" << proc.exitCode();
        qWarning() << proc.readAllStandardOutput();
    }
}


bool DCD::stopServer()
{
    if (m_sproc.state() == QProcess::Running) {
        qDebug() << "shutting down dcd";
        shutdown();
        if(!m_sproc.waitForFinished(TIMEOUT_SHUTDOWN_SERVER))
            m_sproc.terminate();
        if(!m_sproc.waitForFinished(TIMEOUT_SHUTDOWN_SERVER))
            m_sproc.kill();

        return true;
    }
    return false;
}



DCD::~DCD()
{
    if (running()) {
        stopServer();
    }
}
