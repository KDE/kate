/* This file is part of the KDE libraries
   Copyright (C) 2005 Hamish Rodda <rodda@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef KATEREGRESSION_H
#define KATEREGRESSION_H

#include <QObject>
#include <QMap>

class KateDocument;
class CursorExpectation;
class RangeExpectation;

namespace KTextEditor {
  class SmartInterface;
}

class KateRegression : public QObject
{
  Q_OBJECT

  public:
    KateRegression();

    static KateRegression* self();

    void addCursorExpectation(CursorExpectation* expectation);
    void addRangeExpectation(RangeExpectation* expectation);

    KTextEditor::SmartInterface* smart() const;

  private slots:
    void testAll();
    void testRangeTree();

  private:
    void checkSmartManager();
    void checkSignalExpectations();

    static KateRegression* s_self;
    KateDocument* m_doc;
    QList<CursorExpectation*> m_cursorExpectations;
    QList<RangeExpectation*> m_rangeExpectations;
};

#endif
