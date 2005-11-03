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

#include <QTimer>

using namespace KTextEditor;

ArbitraryHighlightTest::ArbitraryHighlightTest(Document* parent)
  : QObject(parent)
  , m_topRange(0L)
{
  QTimer::singleShot(0, this, SLOT(slotCreateTopRange()));
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
      ranges[i]->setBackground(QColor(0xFF - (i * 0x20), 0xFF, 0xFF));
    }
    //ranges[2]->setFontBold();
    //ranges[2]->setForeground(Qt::red);

    Attribute* dyn = new Attribute();
    dyn->setBackground(Qt::blue);
    dyn->setForeground(Qt::white);
    ranges[2]->setDynamicAttribute(Attribute::ActivateMouseIn, dyn, true);
    ranges[2]->setEffects(Attribute::EffectFadeIn | Attribute::EffectFadeOut);

    Attribute* dyn2 = new Attribute();
    dyn2->setBackground(Qt::green);
    dyn2->setForeground(Qt::white);
    ranges[2]->setDynamicAttribute(Attribute::ActivateCaretIn, dyn2, true);

    //ranges[3]->setFontUnderline(true);
    //ranges[3]->setSelectedForeground(Qt::magenta);
    //ranges[4]->setFontStrikeOut(true);
    //ranges[5]->setOutline(Qt::blue);
    //ranges[5]->setForeground(Qt::white);
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

  //outputRange(range, mostSpecificChild);
}

void ArbitraryHighlightTest::outputRange( KTextEditor::SmartRange * range, KTextEditor::SmartRange * mostSpecific )
{
  kdDebug() << (mostSpecific == range ? "==> " : "       ") << QString(range->depth(), ' ') << *range << endl;
  foreach (SmartRange* child, range->childRanges())
    outputRange(child, mostSpecific);
}

void ArbitraryHighlightTest::slotRangeDeleted( KTextEditor::SmartRange * )
{
  m_topRange = 0L;
  QTimer::singleShot(0, this, SLOT(slotCreateTopRange()));
}

void ArbitraryHighlightTest::slotCreateTopRange( )
{
  m_topRange = smart()->newSmartRange(static_cast<Document*>(parent())->documentRange());
  smart()->addHighlightToDocument(m_topRange, true);
  m_topRange->setInsertBehaviour(SmartRange::ExpandRight);
  connect(m_topRange->notifier(), SIGNAL(contentsChanged(KTextEditor::SmartRange*, KTextEditor::SmartRange*)), SLOT(slotRangeChanged(KTextEditor::SmartRange*, KTextEditor::SmartRange*)));
  connect(m_topRange->notifier(), SIGNAL(deleted(KTextEditor::SmartRange*)), SLOT(slotRangeDeleted(KTextEditor::SmartRange*)));

  slotRangeChanged(m_topRange, m_topRange);
}

#include "arbitraryhighlighttest.moc"
