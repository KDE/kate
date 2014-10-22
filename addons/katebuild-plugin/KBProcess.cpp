// Description: QProcess with setShellCommand
//
// Copyright (C) 2014 Kåre Särs <kare.sars@iki.fi>
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Library General Public
//  License as published by the Free Software Foundation; either
//   version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Library General Public License for more details.
//
//  You should have received a copy of the GNU Library General Public License
//  along with this library; see the file COPYING.LIB.  If not, write to
//  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
//  Boston, MA 02110-1301, USA.
//
//  Most of the code is copied from KProcess::setShellCommand() from KProcess
//  in KDE Libs 4.x
//  Copyright (C) 2007 Oswald Buddenhagen <ossi@kde.org>
//


#include "KBProcess.h"
#include <QStringList>
#include <QStandardPaths>
#include <QFile>

void KBProcess::startShellCommand(const QString &cmd)
{
    QString prog;
    QStringList args;

    #ifdef Q_OS_UNIX
    // if NON_FREE ... as they may ship non-POSIX /bin/sh
    # if !defined(__linux__) && !defined(__FreeBSD__) && !defined(__NetBSD__) && !defined(__OpenBSD__) && !defined(__DragonFly__) && !defined(__GNU__)
    // If /bin/sh is a symlink, we can be pretty sure that it points to a
    // POSIX shell - the original bourne shell is about the only non-POSIX
    // shell still in use and it is always installed natively as /bin/sh.
    prog = QFile::symLinkTarget(QString::fromLatin1("/bin/sh"));
    if (prog.isEmpty()) {
        // Try some known POSIX shells.
        prog = QStandardPaths::findExecutable(QString::fromLatin1("ksh"));
        if (prog.isEmpty()) {
            prog = QStandardPaths::findExecutable(QString::fromLatin1("ash"));
            if (prog.isEmpty()) {
                prog = QStandardPaths::findExecutable(QString::fromLatin1("bash"));
                if (prog.isEmpty()) {
                    prog = QStandardPaths::findExecutable(QString::fromLatin1("zsh"));
                    if (prog.isEmpty())
                        // We're pretty much screwed, to be honest ...
                        prog = QString::fromLatin1("/bin/sh");
                }
            }
        }
    }
    # else
    prog = QString::fromLatin1("/bin/sh");
    # endif

    args << QString::fromLatin1("-c") << cmd;
    #else // Q_OS_UNIX
    setEnv(PERCENT_VARIABLE, QLatin1String("%"));

    #ifndef _WIN32_WCE
    WCHAR sysdir[MAX_PATH + 1];
    UINT size = GetSystemDirectoryW(sysdir, MAX_PATH + 1);
    prog = QString::fromUtf16((const ushort *) sysdir, size);
    prog += QLatin1String("\\cmd.exe");
    setNativeArguments(QLatin1String("/V:OFF /S /C \"") + cmd + QLatin1Char('"'));
    #else
    prog = QLatin1String("\\windows\\cmd.exe");
    setNativeArguments(QLatin1String("/S /C \"") + cmd + QLatin1Char('"'));
    #endif
    #endif

    QProcess::start(prog, args);
}