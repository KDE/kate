#ifndef KATE_VI_KEYMAPPER_H_INCLUDED
#define KATE_VI_KEYMAPPER_H_INCLUDED

#include <QtCore/QObject>
#include "katepartprivate_export.h"

class KateViInputModeManager;
class KateDocument;

class QTimer;

class KATEPART_TESTS_EXPORT KateViKeyMapper : public QObject
{
  Q_OBJECT
public:
  KateViKeyMapper(KateViInputModeManager *kateViInputModeManager, KateDocument *doc );
  bool handleKeypress(QChar key);
  void executeMapping();
  void setMappingTimeout(int timeoutMS);
  void setDoNotMapNextKeypress();
public slots:
  void mappingTimerTimeOut();
private:
  bool m_mappingKeyPress;
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
  int m_timeoutlen; // time to wait for the next keypress of a multi-key mapping (default: 1000 ms)
  bool m_doNotMapNextKeypress;
};

#endif
