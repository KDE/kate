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

#ifndef ARBITRARYHIGHLIGHTTEST_H
#define ARBITRARYHIGHLIGHTTEST_H

#include <QObject>

namespace KTextEditor {
  class Document;
  class SmartInterface;
  class SmartRange;
}

/**
  Uses the arbitrary highlight interface to show off some capabilities.

  @author Hamish Rodda <rodda@kde.org>
*/
class ArbitraryHighlightTest : public QObject
{
  Q_OBJECT

  public:
    ArbitraryHighlightTest(KTextEditor::Document* parent = 0L);
    virtual ~ArbitraryHighlightTest();

    KTextEditor::Document* doc() const;
    KTextEditor::SmartInterface* smart() const;

  private slots:
    void slotRangeChanged(KTextEditor::SmartRange* range, KTextEditor::SmartRange* mostSpecificChild);
    void slotRangeDeleted(KTextEditor::SmartRange* range);
    void slotCreateTopRange();

  private:
    void outputRange(KTextEditor::SmartRange* range, KTextEditor::SmartRange * mostSpecific);

    KTextEditor::SmartRange* m_topRange;
};

#endif
