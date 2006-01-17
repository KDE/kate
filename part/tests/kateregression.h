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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KATEREGRESSION_H
#define KATEREGRESSION_H

#include <QObject>
#include <QMap>

#include <ktexteditor/range.h>

class CursorExpectation;
class RangeExpectation;

namespace KTextEditor {
  class Document;
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

  private Q_SLOTS:
    void testAll();
    void testRange();
    void testSmartRange();
    void testRangeTree();

  private:
    void checkRange(KTextEditor::Range& valid);
    void checkSmartManager();
    void checkSignalExpectations();

    static KateRegression* s_self;
    KTextEditor::Document* m_doc;
    QList<CursorExpectation*> m_cursorExpectations;
    QList<RangeExpectation*> m_rangeExpectations;
};

#endif
