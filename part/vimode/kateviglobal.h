/* This file is part of the KDE libraries
 * Copyright (C) 2008 Erlend Hamberg <ehamberg@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) version 3.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef KATE_VI_GLOBAL_H_INCLUDED
#define KATE_VI_GLOBAL_H_INCLUDED

#include <QMap>
#include <QList>
#include <QString>
#include <QChar>

class KateViGlobal
{
public:
    KateViGlobal();
    ~KateViGlobal();
    // registers
public:
    QString getRegisterContent( const QChar &reg ) const;
    void addToNumberedRegister( const QString &text );
    void fillRegister( const QChar &reg, const QString &text);
private:
    QList<QString> *m_numberedRegisters;
    QMap<QChar, QString> *m_registers;
    QChar m_defaultRegister;
    QString m_registerTemp;
};

#endif
