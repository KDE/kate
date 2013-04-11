#ifndef KATEVIEMULATEDCOMMANDBAR_H
#define KATEVIEMULATEDCOMMANDBAR_H

#include "kateviewhelpers.h"
#include <ktexteditor/cursor.h>
#include <ktexteditor/attribute.h>
#include <ktexteditor/movingrange.h>

class KateView;
class QLabel;

class KATEPART_TESTS_EXPORT KateViEmulatedCommandBar : public KateViewBarWidget
{
  Q_OBJECT
public:
  explicit KateViEmulatedCommandBar(KateView *view, QWidget* parent = 0);
  virtual ~KateViEmulatedCommandBar();
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
  bool m_waitingForRegister;

  KTextEditor::Attribute::Ptr m_highlightMatchAttribute;
  KTextEditor::MovingRange* m_highlightedMatch;
  void updateMatchHighlight(const KTextEditor::Range& matchRange);

  virtual bool eventFilter(QObject* object, QEvent* event);
  void deleteSpacesToLeftOfCursor();
  void deleteNonSpacesToLeftOfCursor();
  QString vimRegexToQtRegexPattern(const QString& vimRegexPattern);
  QString toggledEscaped(const QString& string, QChar escapeChar);
  QString ensuredCharEscaped(const QString& string, QChar charToEscape);
private slots:
  void editTextChanged(const QString& newText);
  void updateMatchHighlightAttrib();
};

#endif