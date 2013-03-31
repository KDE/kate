#ifndef KATEVIEMULATEDCOMMANDBAR_H
#define KATEVIEMULATEDCOMMANDBAR_H

#include "kateviewhelpers.h"
#include <ktexteditor/cursor.h>

class KateView;
class QLabel;

class KATEPART_TESTS_EXPORT KateViEmulatedCommandBar : public KateViewBarWidget
{
  Q_OBJECT
public:
  explicit KateViEmulatedCommandBar(KateView *view, QWidget* parent = 0);
  void init(bool backwards);
  virtual void closed();
  bool handleKeyPress(const QKeyEvent* keyEvent);
private:
  KateView *m_view;
  QLineEdit *m_edit;
  QLabel *m_barTypeIndicator;
  bool m_searchBackwards;
  KTextEditor::Cursor m_startingCursorPos;
  bool m_doNotResetCursorOnClose;
  bool m_suspendEditEventFiltering;
  virtual bool eventFilter(QObject* object, QEvent* event);
  void deleteSpacesToLeftOfCursor();
  void deleteNonSpacesToLeftOfCursor();
private slots:
  void editTextChanged(const QString& newText);
};

#endif