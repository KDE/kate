/* This file is part of the KDE libraries
 * Copyright (C) 2008 Erlend Hamberg <ehamberg@gmail.com>
 * Copyright (C) 2008 Evgeniy Ivanov <powerfox@kde.ru>
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

#include <QChar>
#include <QString>
#include <QHash>
class QKeyEvent;

#ifndef KATE_VI_KEYSEQUENCE_PARSER_H_INCLUDED
#define KATE_VI_KEYSEQUENCE_PARSER_H_INCLUDED

/**
 * for encoding keypresses w/ modifiers into an internal QChar representation and back again to a
 * descriptive text string
 */

class KateViKeySequenceParser {
  public:
  KateViKeySequenceParser();

  const QString encodeKeySequence( const QString &keys ) const;
  const QString decodeKeySequence( const QString &keys ) const;
  QString qt2vi( int key ) const;
  char scanCodeToChar(quint32 code, Qt::KeyboardModifiers modifiers, bool isLetter) const;
  const QChar KeyEventToQChar(int keyCode, QString text, Qt::KeyboardModifiers mods, quint32 nativeScanCode) const;

  private:
  void initKeyTables();

  QHash<int, QString> *m_qt2katevi;
  QHash<QString, int> *m_nameToKeyCode;
  QHash<int, QString> *m_keyCodeToName;
};

#endif
