#include "katevikeymapper.h"
#include "kateglobal.h"
#include "kateviglobal.h"

#include <QtCore/QTimer>

KateViKeyMapper::KateViKeyMapper(KateViInputModeManager* kateViInputModeManager, KateDocument* doc )
    : m_viInputModeManager(kateViInputModeManager),
      m_doc(doc)
{
  m_mappingKeyPress = false; // temporarily set to true when an aborted mapping sends key presses
  m_mappingTimer = new QTimer( this );
  m_doNotExpandFurtherMappings = false;
  m_timeoutlen = 1000; // FIXME: make configurable
  m_doNotMapNextKeypress = false;
  connect(m_mappingTimer, SIGNAL(timeout()), this, SLOT(mappingTimerTimeOut()));
}

void KateViKeyMapper::executeMapping()
{
  m_mappingKeys.clear();
  m_mappingTimer->stop();
  const QString mappedKeypresses = KateGlobal::self()->viInputModeGlobal()->getMapping(m_viInputModeManager->getCurrentViMode(), m_fullMappingMatch);
  if (!KateGlobal::self()->viInputModeGlobal()->isMappingRecursive(m_viInputModeManager->getCurrentViMode(), m_fullMappingMatch))
  {
    kDebug(13070) << "Non-recursive: " << mappedKeypresses;
    m_doNotExpandFurtherMappings = true;
  }
  m_doc->editBegin();
  m_viInputModeManager->feedKeyPresses(mappedKeypresses);
  m_doNotExpandFurtherMappings = false;
  m_doc->editEnd();
}

void KateViKeyMapper::setMappingTimeout(int timeoutMS)
{
  m_timeoutlen = timeoutMS;
}

void KateViKeyMapper::mappingTimerTimeOut()
{
  kDebug( 13070 ) << "timeout! key presses: " << m_mappingKeys;
  m_mappingKeyPress = true;
  if (!m_fullMappingMatch.isNull())
  {
    executeMapping();
  }
  else
  {
    m_viInputModeManager->feedKeyPresses( m_mappingKeys );
  }
  m_mappingKeyPress = false;
  m_mappingKeys.clear();
}

bool KateViKeyMapper::handleKeypress(QChar key)
{
  if ( !m_doNotExpandFurtherMappings && !m_mappingKeyPress && !m_doNotMapNextKeypress) {
    m_mappingKeys.append( key );

    bool isPartialMapping = false;
    bool isFullMapping = false;
    m_fullMappingMatch.clear();
    foreach ( const QString &mapping, KateGlobal::self()->viInputModeGlobal()->getMappings(m_viInputModeManager->getCurrentViMode()) ) {
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
    // this cannot be a mapping, restore them. The current key will be appended further down.
    Q_ASSERT(!isPartialMapping && !isFullMapping);
     //currentKeys += m_mappingKeys.mid(0, m_mappingKeys.length() - 1);
    m_doNotExpandFurtherMappings = true;
    m_viInputModeManager->feedKeyPresses(m_mappingKeys.mid(0, m_mappingKeys.length() - 1));
    m_doNotExpandFurtherMappings = false;
    m_mappingKeys.clear();
  } else {
    // FIXME:
    //m_mappingKeyPress = false; // key press ignored wrt mappings, re-set m_mappingKeyPress
  }
  m_doNotMapNextKeypress = false;
  return false;
}

void KateViKeyMapper::setDoNotMapNextKeypress()
{
  m_doNotMapNextKeypress = true;
}
