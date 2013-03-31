#ifndef KATEVIEMULATEDCOMMANDBAR_H
#define KATEVIEMULATEDCOMMANDBAR_H

#include "kateviewhelpers.h"
#include <ktexteditor/cursor.h>

class KateView;

class KATEPART_TESTS_EXPORT KateViEmulatedCommandBar : public KateViewBarWidget
{
  Q_OBJECT
public:
  explicit KateViEmulatedCommandBar(KateView *view, QWidget* parent = 0);
  void init();
  virtual void closed();
  bool handleKeyPress(const QKeyEvent* keyEvent);
private:
  KateView *m_view;
  QLineEdit *m_edit;
  KTextEditor::Cursor m_startingCursorPos;
  bool m_doNotResetCursorOnClose;
  bool m_suspendEditEventFiltering;
  virtual bool eventFilter(QObject* object, QEvent* event);
private slots:
  void editTextChanged(const QString& newText);
};

#endif