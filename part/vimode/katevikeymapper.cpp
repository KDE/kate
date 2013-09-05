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
#include "katevikeymapper.h"
#include "kateglobal.h"
#include "kateviglobal.h"

#include <QtCore/QTimer>

KateViKeyMapper::KateViKeyMapper(KateViInputModeManager* kateViInputModeManager, KateDocument* doc, KateView *view )
    : m_viInputModeManager(kateViInputModeManager),
      m_doc(doc),
      m_view(view)
{
  m_mappingTimer = new QTimer( this );
  m_doNotExpandFurtherMappings = false;
  m_timeoutlen = 1000; // FIXME: make configurable
  m_doNotMapNextKeypress = false;
  m_numMappingsBeingExecuted = 0;
  m_isPlayingBackRejectedKeys = false;
  connect(m_mappingTimer, SIGNAL(timeout()), this, SLOT(mappingTimerTimeOut()));
}

void KateViKeyMapper::executeMapping()
{
  m_mappingKeys.clear();
  m_mappingTimer->stop();
  m_numMappingsBeingExecuted++;
  const QString mappedKeypresses = KateGlobal::self()->viInputModeGlobal()->getMapping(KateViGlobal::mappingModeForCurrentViMode(m_view), m_fullMappingMatch);
  if (!KateGlobal::self()->viInputModeGlobal()->isMappingRecursive(KateViGlobal::mappingModeForCurrentViMode(m_view), m_fullMappingMatch))
  {
    kDebug(13070) << "Non-recursive: " << mappedKeypresses;
    m_doNotExpandFurtherMappings = true;
  }
  m_doc->editBegin();
  m_viInputModeManager->feedKeyPresses(mappedKeypresses);
  m_doNotExpandFurtherMappings = false;
  m_doc->editEnd();
  m_numMappingsBeingExecuted--;
}

void KateViKeyMapper::playBackRejectedKeys()
{
  m_isPlayingBackRejectedKeys = true;
  const QString mappingKeys = m_mappingKeys;
  m_mappingKeys.clear();
  m_viInputModeManager->feedKeyPresses(mappingKeys);
  m_isPlayingBackRejectedKeys = false;
}

void KateViKeyMapper::setMappingTimeout(int timeoutMS)
{
  m_timeoutlen = timeoutMS;
}

void KateViKeyMapper::mappingTimerTimeOut()
{
  kDebug( 13070 ) << "timeout! key presses: " << m_mappingKeys;
  if (!m_fullMappingMatch.isNull())
  {
    executeMapping();
  }
  else
  {
    playBackRejectedKeys();
  }
  m_mappingKeys.clear();
}

bool KateViKeyMapper::handleKeypress(QChar key)
{
  if ( !m_doNotExpandFurtherMappings && !m_doNotMapNextKeypress && !m_isPlayingBackRejectedKeys) {
    m_mappingKeys.append( key );

    bool isPartialMapping = false;
    bool isFullMapping = false;
    m_fullMappingMatch.clear();
    foreach ( const QString &mapping, KateGlobal::self()->viInputModeGlobal()->getMappings(KateViGlobal::mappingModeForCurrentViMode(m_view)) ) {
      if ( mapping.startsWith( m_mappingKeys ) ) {
        if ( mapping == m_mappingKeys ) {
          isFullMapping = true;
          m_fullMappingMatch = mapping;
        } else {
          isPartialMapping = true;
        }
      }
    }
    if (isFullMapping && !isPartialMapping)
    {
      // Great - m_mappingKeys is a mapping, and one that can't be extended to
      // a longer one - execute it immediately.
      executeMapping();
      return true;
    }
    if (isPartialMapping)
    {
      // Need to wait for more characters (or a timeout) before we decide what to
      // do with this.
      m_mappingTimer->start( m_timeoutlen );
      m_mappingTimer->setSingleShot( true );
      return true;
    }
    // We've been swallowing all the keypresses meant for m_keys for our mapping keys; now that we know
    // this cannot be a mapping, restore them.
    Q_ASSERT(!isPartialMapping && !isFullMapping);
    playBackRejectedKeys();
    return true;
  }
  m_doNotMapNextKeypress = false;
  return false;
}

void KateViKeyMapper::setDoNotMapNextKeypress()
{
  m_doNotMapNextKeypress = true;
}

bool KateViKeyMapper::isExecutingMapping()
{
  return m_numMappingsBeingExecuted > 0;
}

bool KateViKeyMapper::isPlayingBackRejectedKeys()
{
  return m_isPlayingBackRejectedKeys;
}
