/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2008-2009 Erlend Hamberg <ehamberg@gmail.com>
 *  Copyright (C) 2013 Simon St James <kdedevel@etotheipiplusone.com>
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
#ifndef KATE_VI_KEYMAPPER_H_INCLUDED
#define KATE_VI_KEYMAPPER_H_INCLUDED

#include <QtCore/QObject>
#include "katepartprivate_export.h"

class KateViInputModeManager;
class KateDocument;
class KateView;

class QTimer;

class KATEPART_TESTS_EXPORT KateViKeyMapper : public QObject
{
  Q_OBJECT
public:
  KateViKeyMapper(KateViInputModeManager *kateViInputModeManager, KateDocument *doc, KateView *view );
  bool handleKeypress(QChar key);
  void setMappingTimeout(int timeoutMS);
  void setDoNotMapNextKeypress();
  bool isExecutingMapping();
  bool isPlayingBackRejectedKeys();
public slots:
  void mappingTimerTimeOut();
private:
  // Will be the mapping used if we decide that no extra mapping characters will be
  // typed, either because we have a mapping that cannot be extended to another
  // mapping by adding additional characters, or we have a mapping and timed out waiting
  // for it to be extended to a longer mapping.
  // (Essentially, this allows us to have mappings that extend each other e.g. "'12" and
  // "'123", and to choose between them.)
  QString m_fullMappingMatch;
  QString m_mappingKeys;
  bool m_doNotExpandFurtherMappings;
  QTimer *m_mappingTimer;
  KateViInputModeManager* m_viInputModeManager;
  KateDocument *m_doc;
  KateView *m_view;
  int m_timeoutlen; // time to wait for the next keypress of a multi-key mapping (default: 1000 ms)
  bool m_doNotMapNextKeypress;
  int m_numMappingsBeingExecuted;
  bool m_isPlayingBackRejectedKeys;

  void executeMapping();
  void playBackRejectedKeys();
};

#endif
