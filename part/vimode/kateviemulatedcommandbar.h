#ifndef KATEVIEMULATEDCOMMANDBAR_H
#define KATEVIEMULATEDCOMMANDBAR_H

#include "kateviewhelpers.h"

class KATEPART_TESTS_EXPORT KateViEmulatedCommandBar : public KateViewBarWidget
{
  Q_OBJECT
public:
  explicit KateViEmulatedCommandBar(QWidget* parent = 0);
  void init();
private:
  QLineEdit *m_edit;
  virtual bool eventFilter(QObject* object, QEvent* event);
};

#endif