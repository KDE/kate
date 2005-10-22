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

#include "arbitraryhighlighttest.h"

#include <ktexteditor/document.h>
#include <ktexteditor/smartinterface.h>
#include <ktexteditor/rangefeedback.h>
#include <ktexteditor/attribute.h>

using namespace KTextEditor;

ArbitraryHighlightTest::ArbitraryHighlightTest(Document* parent)
  : QObject(parent)
  , m_topRange(smart()->newSmartRange(parent->documentRange()))
{
  smart()->addHighlightToDocument(m_topRange);
  m_topRange->setInsertBehaviour(SmartRange::ExpandRight);
  connect(m_topRange->notifier(), SIGNAL(contentsChanged(KTextEditor::SmartRange*, KTextEditor::SmartRange*)), SLOT(slotRangeChanged(KTextEditor::SmartRange*, KTextEditor::SmartRange*)));
}

ArbitraryHighlightTest::~ArbitraryHighlightTest()
{
}

Document * ArbitraryHighlightTest::doc( ) const
{
  return qobject_cast<Document*>(const_cast<QObject*>(parent()));
}

KTextEditor::SmartInterface * ArbitraryHighlightTest::smart( ) const
{
  return dynamic_cast<SmartInterface*>(doc());
}

void ArbitraryHighlightTest::slotRangeChanged(SmartRange* range, SmartRange* mostSpecificChild)
{
  static Attribute* ranges[10] = {0,0,0,0,0,0,0,0,0,0};
  static const QChar openBrace = QChar('{');
  static const QChar closeBrace = QChar('}');

  if (!ranges[0]) {
    for (int i = 0; i < 10; ++i) {
      ranges[i] = new Attribute();
      ranges[i]->setBGColor(QColor(0xFF - (i * 0x20), 0xFF, 0xFF));
    }
    ranges[2]->setBold();
    ranges[2]->setTextColor(Qt::red);
    ranges[3]->setUnderline();
    ranges[3]->setSelectedTextColor(Qt::magenta);
    ranges[4]->setStrikeOut();
    ranges[5]->setOutline(Qt::blue);
    ranges[5]->setTextColor(Qt::white);
  }

  SmartRange* currentRange = mostSpecificChild;
  currentRange->deleteChildRanges();

  Cursor current = currentRange->start();
  QStringList text;

  Range textNeeded = *currentRange;
  if (range != currentRange) {
    if (textNeeded.start() >= textNeeded.end() - Cursor(0,2)) {
      outputRange(range, mostSpecificChild);
      return;
    }

    textNeeded.start() += Cursor(0,1);
    textNeeded.end() -= Cursor(0,1);

    current += Cursor(0,1);
  }

  text = currentRange->document()->textLines(textNeeded);

  foreach (QString string, text) {
    for (int i = 0; i < string.length(); ++i) {
      if (string.at(i) == openBrace) {
        currentRange = smart()->newSmartRange(current, currentRange->end(), currentRange);
        if (currentRange->depth() < 10)
          currentRange->setAttribute(ranges[currentRange->depth()]);

      } else if (string.at(i) == closeBrace && currentRange->parentRange()) {
        currentRange->end() = current + Cursor(0,1);
        currentRange = currentRange->parentRange();
      }
      current.setColumn(current.column() + 1);
    }
    current.setPosition(current.line() + 1, 0);
  }

  outputRange(range, mostSpecificChild);
}

void ArbitraryHighlightTest::outputRange( KTextEditor::SmartRange * range, KTextEditor::SmartRange * mostSpecific )
{
  kdDebug() << (mostSpecific == range ? "==> " : "       ") << QString(range->depth(), ' ') << *range << endl;
  foreach (SmartRange* child, range->childRanges())
    outputRange(child, mostSpecific);
}


#include "arbitraryhighlighttest.moc"
