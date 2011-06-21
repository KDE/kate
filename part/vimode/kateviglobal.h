/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2008 Erlend Hamberg <ehamberg@gmail.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#ifndef KATE_VI_GLOBAL_H_INCLUDED
#define KATE_VI_GLOBAL_H_INCLUDED

#include <QMap>
#include <QHash>
#include <QList>
#include <QPair>

#include "katevimodebase.h"
#include "kateviinputmodemanager.h"
#include "katepartprivate_export.h"

class QString;
class QChar;
class KConfigGroup;

namespace KTextEditor {
  class MovingCursor;
}

namespace KateVi {
  const unsigned int EOL = 99999;
}

typedef QPair<QString, OperationMode> KateViRegister;
typedef QPair<int,int> KateViJump;

class KATEPART_TESTS_EXPORT KateViGlobal
{
public:
    KateViGlobal();
    ~KateViGlobal();

    void writeConfig( KConfigGroup &config ) const;
    void readConfig( const KConfigGroup &config );
    QString getRegisterContent( const QChar &reg ) const;
    OperationMode getRegisterFlag( const QChar &reg ) const;
    void addToNumberedRegister( const QString &text, OperationMode flag = CharWise );
    void fillRegister( const QChar &reg, const QString &text, OperationMode flag = CharWise);
    const QMap<QChar, KateViRegister>* getRegisters() const { return m_registers; }

    void clearMappings( ViMode mode );
    void addMapping( ViMode mode, const QString &from, const QString &to );
    const QString getMapping( ViMode mode, const QString &from, bool decode = false ) const;
    const QStringList getMappings( ViMode mode, bool decode = false ) const;

    void addMark( KateDocument* doc, const QChar& mark, const KTextEditor::Cursor& pos );
    KTextEditor::Cursor getMarkPosition( const QChar& mark ) const;

    void addJump(KTextEditor::Cursor cursor);
    KTextEditor::Cursor getNextJump(KTextEditor::Cursor cursor);
    KTextEditor::Cursor getPrevJump(KTextEditor::Cursor cursor);

private:
    // registers
    QList<KateViRegister> *m_numberedRegisters;
    QMap<QChar, KateViRegister> *m_registers;
    QChar m_defaultRegister;
    QString m_registerTemp;
    KateViRegister getRegister( const QChar &reg ) const;

    // marks
    QMap<QChar, KTextEditor::MovingCursor*> m_marks;

    // mappings
    QHash <QString, QString> m_normalModeMappings;

    // jump list
    QList <KateViJump> *jump_list;
    QList<KateViJump>::iterator current_jump;

};

#endif
