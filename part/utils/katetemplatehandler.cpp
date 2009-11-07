/* This file is part of the KDE libraries
  Copyright (C) 2004 Joseph Wenninger <jowenn@kde.org>
  Copyright (C) 2009 Milian Wolff <mail@milianw.de>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License version 2 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#include "katetemplatehandler.h"
#include "katetemplatehandler.moc"
#include "katedocument.h"
#include "katesmartcursor.h"
#include "kateview.h"
#include "kateconfig.h"
#include "katerenderer.h"

#include <ktexteditor/cursor.h>
#include <ktexteditor/smartcursor.h>
#include <ktexteditor/smartrange.h>
#include <ktexteditor/range.h>
#include <ktexteditor/attribute.h>

#include <QtCore/QQueue>

#include <kdebug.h>

using namespace KTextEditor;

/**
 * Debug function to dump the range with all it's children.
 */
void dumpRange(SmartRange* range, int depth = 0) {
  kDebug() << depth << range << *range << range->notifiers().size();
  foreach ( SmartRange* child, range->childRanges() ) {
    dumpRange( child, depth + 1 );
  }
}

/* ####################################### */

KateTemplateHandler::KateTemplateHandler( KateDocument *doc, const Cursor& position,
                                          const QString &templateString, const QMap<QString, QString> &initialValues )
    : QObject(doc)
    , m_doc(doc), m_wholeTemplateRange(0), m_finalCursorPosition(0)
    , m_lastCaretPosition(position), m_isMirroring(false), m_editWithUndo(false)
{
  if ( !initialValues.isEmpty() ) {
    // only do complex stuff when required

    handleTemplateString(position, templateString, initialValues);

    if ( !m_templateRanges.isEmpty() ) {
      foreach ( View* view, m_doc->views() ) {
        setupEventHandler(view);
        connect(view, SIGNAL(cursorPositionChanged(KTextEditor::View*, KTextEditor::Cursor)),
                this, SLOT(slotCursorPositionChanged(KTextEditor::View*, KTextEditor::Cursor)));
      }

      connect(m_doc, SIGNAL(viewCreated(KTextEditor::Document*, KTextEditor::View*)),
              this, SLOT(slotViewCreated(KTextEditor::Document*, KTextEditor::View*)));
      connect(m_doc, SIGNAL(textChanged(KTextEditor::Document*, KTextEditor::Range, KTextEditor::Range)),
              this, SLOT(slotTextChanged(KTextEditor::Document*, KTextEditor::Range)));
      connect(m_doc, SIGNAL(textInserted(KTextEditor::Document*, KTextEditor::Range)),
              this, SLOT(slotTextChanged(KTextEditor::Document*, KTextEditor::Range)));
      connect(m_doc, SIGNAL(textRemoved(KTextEditor::Document*, KTextEditor::Range)),
              this, SLOT(slotTextChanged(KTextEditor::Document*, KTextEditor::Range)));

    } else {
      // when no interesting ranges got added, we can terminate directly
      jumpToFinalCursorPosition();
      cleanupAndExit();
    }
  } else {
    // simple templates just need to be inserted
    insertText(position, templateString);
    cleanupAndExit();
  }
}

KateTemplateHandler::~KateTemplateHandler()
{
}

/// simple wrapper to delete a smartrange
void deleteSmartRange(SmartRange* range, KateDocument* doc) {
  doc->removeHighlightFromDocument(range);
  SmartCursor* start = &range->smartStart();
  SmartCursor* end = &range->smartEnd();
  doc->unbindSmartRange(range);
  delete start;
  delete end;
}

void KateTemplateHandler::cleanupAndExit()
{
  if ( !m_templateRanges.isEmpty() ) {
      foreach ( SmartRange* range, m_templateRanges ) {
        deleteSmartRange(range, m_doc);
      }
      m_templateRanges.clear();
  }
  if ( m_wholeTemplateRange ) {
    deleteSmartRange(m_wholeTemplateRange, m_doc);
  }
  if ( m_finalCursorPosition ) {
    delete m_finalCursorPosition;
  }
  deleteLater();
}

void KateTemplateHandler::jumpToFinalCursorPosition()
{
  if ( m_doc->activeView() ) {
    if ( m_finalCursorPosition ) {
      // jump to user defined end position
      m_doc->activeView()->setCursorPosition(*m_finalCursorPosition);
    } else if ( m_wholeTemplateRange ) {
      // jump to the end of our template
      m_doc->activeView()->setCursorPosition(m_wholeTemplateRange->end());
    }
  }
}

void KateTemplateHandler::setEditWithUndo(const bool &enabled)
{
  m_editWithUndo = enabled;
}

void KateTemplateHandler::slotViewCreated(Document* document, View* view)
{
  Q_ASSERT(document == m_doc);
  setupEventHandler(view);
  connect(view, SIGNAL(cursorPositionChanged(KTextEditor::View*,KTextEditor::Cursor)),
          this, SLOT(slotCursorPositionChanged(KTextEditor::View*,KTextEditor::Cursor)));
}

void KateTemplateHandler::setupEventHandler(View* view)
{
  view->focusProxy()->installEventFilter(this);
}

bool KateTemplateHandler::eventFilter(QObject* object, QEvent* event)
{
  // prevent indenting by eating the keypress event for TAB
  if ( event->type() == QEvent::KeyPress ) {
    QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
    if ( keyEvent->key() == Qt::Key_Tab ) {
      if ( !m_doc->activeKateView()->isCompletionActive() ) {
        return true;
      }
    }
  }
  // actually offer shortcuts for navigation
  if ( event->type() == QEvent::ShortcutOverride ) {
    QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
    if ( keyEvent->key() == Qt::Key_Return && keyEvent->modifiers() & Qt::AltModifier ) {
      // terminate
      jumpToFinalCursorPosition();
      cleanupAndExit();
      return true;
    } else if ( keyEvent->key() == Qt::Key_Escape ) {
      if ( !m_doc->activeView() || !m_doc->activeView()->selection() ) {
        // terminate
        cleanupAndExit();
        return true;
      }
    } else if ( keyEvent->key() == Qt::Key_Tab ) {
      if ( keyEvent->modifiers() & Qt::Key_Shift ) {
        jumpToPreviousRange();
      } else {
        jumpToNextRange();
      }
      return true;
    }
  }
  return QObject::eventFilter(object, event);
}

void KateTemplateHandler::jumpToPreviousRange()
{
  const Cursor & cursor = m_doc->activeView()->cursorPosition();
  SmartRange* previousRange = 0;
  foreach ( SmartRange* range, m_templateRanges ) {
    if ( range->start() >= cursor ) {
      continue;
    }
    if ( !previousRange || range->start() > previousRange->start() ) {
      previousRange = range;
    }
  }
  kDebug() << previousRange << *previousRange;
  if ( previousRange ) {
    setCurrentRange(previousRange);
  } else {
    // wrap and jump to last range
    setCurrentRange(m_templateRanges.last());
  }
}

void KateTemplateHandler::jumpToNextRange()
{
  const Cursor & cursor = m_doc->activeView()->cursorPosition();
  SmartRange* nextRange = 0;
  foreach ( SmartRange* range, m_templateRanges ) {
    if ( range->start() <= cursor ) {
      continue;
    }
    if ( !nextRange || range->start() < nextRange->start() ) {
      nextRange = range;
    }
  }
  if ( nextRange ) {
    setCurrentRange(nextRange);
  } else {
    // wrap and jump to first range
    setCurrentRange(m_templateRanges.first());
  }
}

void KateTemplateHandler::setCurrentRange(SmartRange* range)
{
  if ( !range->childRanges().isEmpty() ) {
    // jump to first mirrored range
    range = range->childRanges().first();
  }

  if ( m_doc->activeView() ) {
    //TODO: only select when range has not been changed yet
    m_doc->activeView()->setSelection(*range);
    m_doc->activeView()->setCursorPosition(range->start());
  }

  m_lastCaretPosition = range->start();
}

/**
 * Returns an attribute with \p color as background with 0x88 alpha value.
 */
Attribute::Ptr getAttribute(QColor color)
{
  Attribute::Ptr attribute(new Attribute());
  color.setAlpha(0x88);
  attribute->setBackground(QBrush(color));
  return attribute;
}

void KateTemplateHandler::insertText(const Cursor &position, const QString& text)
{
  ///TODO: maybe use Kate::CutCopyPasteEdit or similar?
  m_doc->editStart();
  if ( m_doc->insertText(position, text) ) {
    m_doc->undoSafePoint();
  }
  m_doc->editEnd();
}

void KateTemplateHandler::handleTemplateString(const Cursor& position, QString templateString,
                                               const QMap< QString, QString >& initialValues)
{
  KateRendererConfig *config = m_doc->activeKateView()->renderer()->config();

  Attribute::Ptr editableAttribute = getAttribute(config->templateEditablePlaceholderColor());
  editableAttribute->setDynamicAttribute(
      Attribute::ActivateCaretIn, getAttribute(config->templateFocusedEditablePlaceholderColor())
  );

  Attribute::Ptr mirroredAttribute = getAttribute(config->templateNotEditablePlaceholderColor());

  int line = position.line();
  int column = position.column();

  // true, when the last char was a backslash
  // and that one wasn't prepended by an odd number
  // of backslashes
  bool isEscaped = false;

  // not equal -1 when we found a start position
  int startPos = -1;

  // each found variable gets it's range(s) added to the list.
  // the key is the varname, e.g. the same as in initialValues
  // to be able to iterate over them in a FIFO matter, also store
  // the keys in a queue.
  QQueue<QString> keyQueue;
  QMultiMap<QString, Range> ranges;

  // valid, if we find an occurrence of ${cursor}
  Cursor finalCursorPosition = Cursor::invalid();

  // parse string for ${VAR} or %{VAR}
  // VAR must not contain $ or %
  // VAR must not contain newlines
  // VAR must be set as key in initialValues
  // expression must not be escaped
  for ( int i = 0; i < templateString.size(); ++i ) {
    if ( templateString[i] == '\\' ) {
      ++column;
      isEscaped = !isEscaped;
    } else if ( templateString[i] == '\n' ) {
      ++line;
      column = 0;
      isEscaped = false;
      if ( startPos != -1 ) {
        // don't allow variables to span multiple lines
        startPos = -1;
      }
    } else if ( templateString[i] == '%' || templateString[i] == '$' ) {
      if ( isEscaped && startPos == -1 ) {
        // remove escape char
        templateString.remove(i - 1, 1);
        --i;
        isEscaped = false;
      } else if ( startPos != -1 ) {
        // don't allow nested variables
        startPos = -1;
        ++column;
      } else if ( i + 2 < templateString.size() && templateString[i + 1] == '{' ) {
        // need at least %{}
        startPos = i;
        // skip {
        ++i;
      }
    } else if ( templateString[i] == '}' && startPos != -1 ) {
      // get key, i.e. contents between ${..}
      const QString key = templateString.mid( startPos + 2, i - (startPos + 2) );
      if ( !initialValues.contains(key) ) {
        kWarning() << "unknown variable key:" << key;
      } else if ( key == "cursor" ) {
        finalCursorPosition = Cursor(line, column - key.length() - 1);
        // don't insert anything, just remove the placeholder
        templateString.remove(startPos, i - startPos + 1);
        // correct iterator pos, 3 == $ + { + } or % + { + }
        i -= 3 + key.length();
        startPos = -1;
      } else {
        // replace variable with initial value
        templateString.replace( startPos, i - startPos + 1, initialValues[key] );
        // correct iterator pos, 3 == % + { + } or $ + { + }
        int offset = 3 + key.length() - initialValues[key].length();
        i -= offset;
        // always add ${...} to the editable ranges
        // only add %{...} to the editable ranges when it's value equals the key
        if ( templateString[startPos] == '$' || key == initialValues[key] ) {
          if ( !keyQueue.contains(key) ) {
            keyQueue.append(key);
          }
          ranges.insert( key, Range(line, column - initialValues[key].length(), line, column) );
        }
      }
      startPos = -1;
    } else {
      isEscaped = false;
      ++column;
    }
  }

  // need to insert here, so that the following smart range can get their ranges set properly.
  insertText(position, templateString);

  if ( m_doc->activeKateView() ) {
    ///HACK: make sure the view cache is up2date so we can safely set the cursor position
    ///      else one might hit the Q_ASSERT(false) in KateViewInternal::endPos()
    ///      the cause is that the view get's ::update()'d, but that does not trigger a
    ///      paint event right away.
    m_doc->activeKateView()->updateView(true);
  }

  if ( finalCursorPosition.isValid() ) {
    m_finalCursorPosition = m_doc->newSmartCursor(finalCursorPosition);
  }

  if ( ranges.isEmpty() ) {
    return;
  }

  m_wholeTemplateRange = m_doc->newSmartRange( Range(position, Cursor(line, column)), 0,
                                               SmartRange::ExpandLeft | SmartRange::ExpandRight );
  m_wholeTemplateRange->setAttribute(getAttribute(config->templateBackgroundColor()));
  m_doc->addHighlightToDocument(m_wholeTemplateRange, true);

  // create smart ranges for each found variable
  // if the variable exists more than once, create "mirrored" ranges
  foreach ( const QString& key, keyQueue ) {
    const QList<Range> &values = ranges.values(key);
    // used as parent for mirrored variables,
    // and as only item for not-mirrored variables
    SmartRange* parent = m_doc->newSmartRange(values.first(), 0, SmartRange::ExpandLeft | SmartRange::ExpandRight);
    if ( values.size() > 1 ) {
      // add all "real" ranges as children
      for (int i = 0; i < values.size(); ++i )  {
        SmartRange* range = m_doc->newSmartRange(
          values[i], parent, SmartRange::ExpandLeft | SmartRange::ExpandRight
        );
        // the last item will be our real first range (see multimap docs)
        if ( i == values.size() - 1 ) {
          range->setAttribute(editableAttribute);
        } else {
          range->setAttribute(mirroredAttribute);
        }
      }
    } else {
      // just a single range
      parent->setAttribute(editableAttribute);
    }

    m_doc->addHighlightToDocument(parent, true);
    m_templateRanges.append(parent);
  }

  setCurrentRange(m_templateRanges.first());
}

void KateTemplateHandler::slotCursorPositionChanged(View* view, const Cursor &cursor)
{
  Q_UNUSED(view);

  if ( !m_wholeTemplateRange->contains(cursor) ) {
    // terminate
    cleanupAndExit();
    return;
  }

  // only jump when cursor is exactly one pos after or before the last position

  Cursor diff(cursor - m_lastCaretPosition);
  if ( 1 != abs(diff.column()) + abs(diff.line()) ) {
    m_lastCaretPosition = cursor;
    return;
  }

  // don't jump when we are still inside one of our ranges
  // or if the old one was not inside on of our ranges
  bool wasInRange = false;
  foreach ( SmartRange* range, m_templateRanges ) {
    if ( !range->childRanges().isEmpty() ) {
      range = range->childRanges().first();
    }

    // don't use contains() as it checks for < end(), we want <= end()
    if ( cursor >= range->start() && cursor <= range->end() ) {
      // cursor is in one of our tracked ranges, do nothing
      m_lastCaretPosition = cursor;
      return;
    }
    if ( !wasInRange ) {
      wasInRange = m_lastCaretPosition >= range->start() && m_lastCaretPosition <= range->end();
    }
  }

  if ( !wasInRange ) {
    m_lastCaretPosition = cursor;
    return;
  }

  if ( cursor > m_lastCaretPosition ) {
    // moved cursor to the right
    jumpToNextRange();
  } else {
    // moved cursor to the left
    jumpToPreviousRange();
  }
}

void KateTemplateHandler::slotTextChanged(Document* document, const Range& range)
{
  if ( m_doc->isEditRunning() && !m_editWithUndo ) {
    kDebug(13020) << "slotTextChanged returning prematurely";
    return;
  }
  Q_ASSERT(document == m_doc);

  if ( !m_wholeTemplateRange->contains(range) || m_isMirroring ) {
    // outside our template or one of our own changes
    return;
  }

  // check if @p range is mirrored
  foreach ( SmartRange* parent, m_templateRanges ) {
    if ( !parent->childRanges().isEmpty() && parent->contains(range)  ) {
      foreach ( SmartRange* child, parent->childRanges() ) {
        // contains uses < end(), cannot use.
        if ( child->start() <= range.start() && child->end() >= range.start() ) {
          syncMirroredRanges(child);
          break;
        }
      }
      // our mirrored ranges cannot overlap, so always break
      break;
    }
  }
}

void KateTemplateHandler::syncMirroredRanges(SmartRange* range)
{
  Q_ASSERT(m_templateRanges.contains(range->parentRange()));

  m_isMirroring = true;
  m_doc->editStart();

  const QStringList &newText = m_doc->textLines(*range);

  foreach ( SmartRange* sibling, range->parentRange()->childRanges() ) {
    if ( sibling != range ) {
      sibling->replaceText(newText);
    }
  }

  /// TODO: now undo only undos the last char edit, and does not
  ///       merge those edits as usual
  bool undoDontMerge = m_doc->undoDontMerge();
  m_doc->setUndoDontMerge(false);
  m_doc->setUndoAllowComplexMerge(true);
  m_doc->undoSafePoint();
  m_doc->editEnd();
  m_doc->setUndoDontMerge(undoDontMerge);
  m_isMirroring = false;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
