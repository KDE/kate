/* This file is part of the KDE project
   Copyright 2008-2014 Dominik Haumann <dhaumann kde org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "btparser.h"

#include <QStringList>
#include <QDebug>


static QString eolDelimiter(const QString &str)
{
    // find the split character
    QString separator(QLatin1Char('\n'));
    if (str.indexOf(QStringLiteral("\r\n")) != -1) {
        separator = QStringLiteral("\r\n");
    } else if (str.indexOf(QLatin1Char('\r')) != -1) {
        separator = QLatin1Char('\r');
    }
    return separator;
}

static bool lineNoLessThan(const QString &lhs, const QString &rhs)
{
    QRegExp rx(QStringLiteral("^#(\\d+)"));
    int ilhs = rx.indexIn(lhs);
    int lhsLn = rx.cap(1).toInt();
    int irhs = rx.indexIn(rhs);
    int rhsLn = rx.cap(1).toInt();
    if (ilhs != -1 && irhs != -1) {
        return lhsLn < rhsLn;
    } else {
        return lhs < rhs;
    }
}

static QStringList normalizeBt(const QStringList &l)
{
    QStringList normalized;

    bool append = false;

    for (int i = 0; i < l.size(); ++i) {
        QString str = l[i].trimmed();
        if (str.length()) {
            if (str[0] == QLatin1Char('#')) {
                normalized << str;
                append = true;
            } else if (append) {
                normalized.last() += QLatin1Char(' ') + str;
            }
        } else {
            append = false;
        }
    }

    qSort(normalized.begin(), normalized.end(), lineNoLessThan);

    // now every single line contains a whole backtrace info
    return normalized;
}

static BtInfo parseBtLine(const QString &line)
{
    int index;

    // the syntax types we support are
    // a) #24 0xb688ff8e in QApplication::notify (this=0xbf997e8c, receiver=0x82607e8, e=0xbf997074) at kernel/qapplication.cpp:3115
    // b) #39 0xb634211c in g_main_context_dispatch () from /usr/lib/libglib-2.0.so.0
    // c) #41 0x0805e690 in ?? ()
    // d) #5  0xffffe410 in __kernel_vsyscall ()


    // try a) cap #number(1), address(2), function(3), filename(4), linenumber(5)
    static QRegExp rxa(QStringLiteral("^#(\\d+)\\s+(0x\\w+)\\s+in\\s+(.+)\\s+at\\s+(.+):(\\d+)$"));
    index = rxa.indexIn(line);
    if (index == 0) {
        BtInfo info;
        info.original = line;
        info.filename = rxa.cap(4);
        info.function = rxa.cap(3);
        info.address = rxa.cap(2);
        info.line = rxa.cap(5).toInt();
        info.step = rxa.cap(1).toInt();
        info.type = BtInfo::Source;
        return info;
    }

    // try b) cap #number(1), address(2), function(3), lib(4)
    static QRegExp rxb(QStringLiteral("^#(\\d+)\\s+(0x\\w+)\\s+in\\s+(.+)\\s+from\\s+(.+)$"));
    index = rxb.indexIn(line);
    if (index == 0) {
        BtInfo info;
        info.original = line;
        info.filename = rxb.cap(4);
        info.function = rxb.cap(3);
        info.address = rxb.cap(2);
        info.line = -1;
        info.step = rxb.cap(1).toInt();
        info.type = BtInfo::Lib;
        return info;
    }

    // try c) #41 0x0805e690 in ?? ()
    static QRegExp rxc(QStringLiteral("^#(\\d+)\\s+(0x\\w+)\\s+in\\s+\\?\\?\\s+\\(\\)$"));
    index = rxc.indexIn(line);
    if (index == 0) {
        BtInfo info;
        info.original = line;
        info.filename = QString();
        info.function = QString();
        info.address = rxc.cap(2);
        info.line = -1;
        info.step = rxc.cap(1).toInt();
        info.type = BtInfo::Unknown;
        return info;
    }

    // try d) #5  0xffffe410 in __kernel_vsyscall ()
    static QRegExp rxd(QStringLiteral("^#(\\d+)\\s+(0x\\w+)\\s+in\\s+(.+)$"));
    index = rxd.indexIn(line);
    if (index == 0) {
        BtInfo info;
        info.original = line;
        info.filename = QString();
        info.function = rxd.cap(3);
        info.address = rxd.cap(2);
        info.line = -1;
        info.step = rxd.cap(1).toInt();
        info.type = BtInfo::Unknown;
        return info;
    }

    qDebug() << "Unknown backtrace line:" << line;

    BtInfo info;
    info.type = BtInfo::Invalid;
    return info;
}

QList<BtInfo>  KateBtParser::parseBacktrace(const QString &bt)
{
    QStringList l = bt.split(eolDelimiter(bt), QString::SkipEmptyParts);

    l = normalizeBt(l);

    QList<BtInfo> btList;
    for (int i = 0; i < l.size(); ++i) {
        BtInfo info = parseBtLine(l[i]);
        if (info.type != BtInfo::Invalid) {
            btList.append(parseBtLine(l[i]));
        }
    }

    return btList;
}

// kate: space-indent on; indent-width 4; replace-tabs on;
