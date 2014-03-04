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

#include "dcd.h"
#include <kprocess.h>
#include <kdebug.h>
#include <QtCore/QFile>


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

#define RETURN_CACHED_ICON(name) {static QIcon icon(KIcon(name).pixmap(QSize(16, 16))); return icon;}
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

    return KIcon();
}

QString DCDCompletionItem::typeLong() const
{
    using namespace DCDCompletionItemType;
    switch (type)
    {
        case Invalid: return "invalid";
        case Calltip: return "calltip";
        case ClassName: return "class";
        case InterfaceName: return "interface";
        case StructName: return "struct";
        case UnionName: return "union";
        case VariableName: return "variable";
        case MemberVariableName: return "member";
        case Keyword: return "keyword";
        case FunctionName: return "function";
        case EnumName: return "enum";
        case EnumMember: return "enum member";
        case PackageName: return "package";
        case ModuleName: return "module";
    }

    return "completion";
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

int DCD::port()
{
    return m_port;
}

bool DCD::running()
{
    return m_sproc.state() == KProcess::Running;
}


bool DCD::startServer()
{
    m_sproc.setOutputChannelMode(KProcess::MergedChannels);
    m_sproc.setProgram(m_server, QStringList(QString("-p%1").arg(m_port)));
    m_sproc.start();
    bool started = m_sproc.waitForStarted(TIMEOUT_START_SERVER);
    bool finished = m_sproc.waitForFinished(TIMEOUT_START_SERVER);

    if (!started || finished || m_sproc.state() == KProcess::NotRunning) {
        kWarning() << "unable to start completion-server:" << m_sproc.exitCode();
        kWarning() << m_sproc.readAll();
        return false;
    }
    kDebug() << "started completion-server";
    return true;
}


DCDCompletion DCD::complete(QString file, int offset)
{
    KProcess proc;
    proc.setOutputChannelMode(KProcess::MergedChannels);
    proc.setProgram(m_client,
        QStringList()
            << QString("-p%1").arg(m_port)
            << QString("-c%1").arg(offset)
            << file
    );
    int ret = proc.execute(TIMEOUT_COMPLETE);

    if (ret != 0) {
        kWarning() << "unable to complete:" << ret;
        kWarning() << proc.readAll();
        return DCDCompletion();
    }

    return processCompletion(proc.readAllStandardOutput());
}

DCDCompletion DCD::complete(QByteArray data, int offset)
{
    KProcess proc;
    proc.setOutputChannelMode(KProcess::MergedChannels);
    proc.setProgram(m_client,
        QStringList()
            << QString("-p%1").arg(m_port)
            << QString("-c%1").arg(offset)
    );

    proc.start();
    proc.write(data);
    proc.closeWriteChannel();
    if (!proc.waitForFinished(TIMEOUT_COMPLETE)) {
        kWarning() << "unable to complete: client didn't finish in time";
        proc.close();
    } else if (proc.exitCode() != 0) {
        kWarning() << "unable to complete:" << proc.exitCode();
        kWarning() << proc.readAll();
    } else {
        // everything Ok
        return processCompletion(proc.readAllStandardOutput());
    }

    return DCDCompletion();
}

QString DCD::doc(QByteArray data, int offset)
{
    KProcess proc;
    proc.setOutputChannelMode(KProcess::MergedChannels);
    proc.setProgram(m_client,
        QStringList()
            << QString("-p%1").arg(m_port)
            << QString("-c%1").arg(offset)
            << QString("--doc")
    );

    proc.start();
    proc.write(data);
    proc.closeWriteChannel();
    if (!proc.waitForFinished(TIMEOUT_COMPLETE)) {
        kWarning() << "unable to lookup documentation: client didn't finish in time";
        proc.close();
    } else if (proc.exitCode() != 0) {
        kWarning() << "unable to lookup documentation:" << proc.exitCode();
        kWarning() << proc.readAll();
    } else {
        return proc.readAllStandardOutput();
    }

    return QString("");
}


DCDCompletion DCD::processCompletion(QString data)
{
    DCDCompletion completion;

    QStringList lines = data.split(QRegExp("[\r\n]"), QString::SkipEmptyParts);
    if (lines.length() == 0) {
        return completion;
    }

    QString type = lines.front();
    if (type == "identifiers") { completion.type = DCDCompletionType::Identifiers; }
    else if (type == "calltips") { completion.type = DCDCompletionType::Calltips; }
    else {
        kWarning() << "Invalid type:" << type;
        return completion;
    }
    lines.pop_front();

    foreach(QString line, lines) {
        if (line.trimmed().length() == 0) {
            continue;
        }

        QStringList kv = line.split(QRegExp("\\s+"), QString::SkipEmptyParts);
        if (kv.length() != 2 && completion.type != DCDCompletionType::Calltips) {
            kWarning() << "invalid completion data:" << kv.length() << completion.type;
            continue;
        }

        if (completion.type == DCDCompletionType::Identifiers) {
            completion.completions.append(DCDCompletionItem(
                DCDCompletionItemType::fromChar(kv[1].at(0).toAscii()), kv[0]
            ));
        } else {
            completion.completions.append(DCDCompletionItem(
                DCDCompletionItemType::Calltip, line
            ));
        }
    }

    return completion;
}


void DCD::addImportPath(QString path)
{
    addImportPath(QStringList(path));
}

void DCD::addImportPath(QStringList paths)
{
    if (paths.isEmpty()) {
        return;
    }

    QStringList arguments = QStringList(QString("-p%1").arg(m_port));
    foreach(QString path, paths) {
        if (QFile::exists(path))
            arguments << QString("-I%1").arg(path);
    }

    kDebug() << "ARGUMENTS:" << arguments;

    KProcess proc;
    proc.setOutputChannelMode(KProcess::MergedChannels);
    proc.setProgram(m_client, arguments);
    int ret = proc.execute(TIMEOUT_IMPORTPATH);

    if (ret != 0) {
        kWarning() << "unable to add importpath(s)" << paths << ":" << ret;
        kWarning() << proc.readAll();
    }
}

void DCD::shutdown()
{
    KProcess proc;
    proc.setOutputChannelMode(KProcess::MergedChannels);
    proc.setProgram(m_client,
        QStringList()
            << QString("-p%1").arg(m_port)
            << QString("--shutdown")
    );
    int ret = proc.execute(TIMEOUT_SHUTDOWN);

    if (ret != 0) {
        kWarning() << "unable to shutdown dcd:" << ret;
        kWarning() << proc.readAll();
    }
}


bool DCD::stopServer()
{
    if (m_sproc.state() == KProcess::Running) {
        kDebug() << "shutting down dcd";
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


#include "dcd.moc"
