/*
   Copyright (C) 2002 by Roberto Raggi <roberto@kdevelop.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   version 2, License as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef KATE_ARGHINT_H
#define KATE_ARGHINT_H

#include <qframe.h>
#include <qmap.h>

class KateView;
class KDevArgHintData;

class KDevArgHint: public QFrame
{
    Q_OBJECT
public:
    KDevArgHint( KateView* =0, const char* =0 );
    virtual ~KDevArgHint();

    virtual void setCurrentFunction( int );
    virtual int currentFunction() const { return m_currentFunction; }

    void setArgMarkInfos( const QString&, const QString& );

    virtual void addFunction( int, const QString& );
    QString functionAt( int id ) const { return m_functionMap[ id ]; }

    virtual void show();
    virtual void adjustSize();
    virtual bool eventFilter( QObject*, QEvent* );

signals:
    void argHintHidden();

public slots:
    virtual void reset( int, int );
    virtual void cursorPositionChanged( KateView*, int, int );

private slots:
    void slotDone();

private:
    QMap<int, QString> m_functionMap;
    int m_currentFunction;
    QString m_wrapping;
    QString m_delimiter;
    bool m_markCurrentFunction;
    int m_currentLine;
    int m_currentCol;
    KDevArgHintData* d;
};

#endif
