/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2004 Joseph Wenninger <jowenn@kde.org>
 *  Copyright (C) 2009 Milian Wolff <mail@milianw.de>
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

#include "katetemplatehandler.h"
#include "katedocument.h"
#include "katesmartcursor.h"
#include "kateview.h"
#include "kateconfig.h"
#include "katerenderer.h"
#include "kateundomanager.h"
#include "katematch.h"

#include <ktexteditor/cursor.h>
#include <ktexteditor/smartcursor.h>
#include <ktexteditor/smartrange.h>
#include <ktexteditor/range.h>
#include <ktexteditor/attribute.h>

#include <QtCore/QQueue>

#include <kdebug.h>


using namespace KTextEditor;

#define ifDebug(x) x;

/// just like Range::contains() but returns true when the cursor is at the end of the range
bool customContains(SmartRange* range, const Cursor& cursor)
{
  return range->start() <= cursor && range->end() >= cursor;
}

/* ####################################### */

KateTemplateHandler::KateTemplateHandler( KateDocument *doc, const Cursor& position,
                                          const QString &templateString, const QMap<QString, QString> &initialValues, KateUndoManager* undoManager )
    : QObject(doc)
    , m_doc(doc), m_wholeTemplateRange(0), m_finalCursorPosition(0)
    , m_lastCaretPosition(position), m_isMirroring(false), m_editWithUndo(false), m_jumping(false)
{
  ifDebug(kDebug() << templateString << initialValues;)

  connect(m_doc, SIGNAL(aboutToReload(KTextEditor::Document*)),
          this, SLOT(cleanupAndExit()));

  connect(m_doc, SIGNAL(textInserted(KTextEditor::Document*, KTextEditor::Range)),
          this, SLOT(slotTemplateInserted(KTextEditor::Document*, KTextEditor::Range)));

  ///TODO: maybe use Kate::CutCopyPasteEdit or similar?
  m_doc->editStart();
  if ( m_doc->insertText(position, templateString) ) {
    Q_ASSERT(m_wholeTemplateRange);

    if ( m_doc->activeKateView() ) {
      // indent the inserted template properly, this makes it possible
      // to share snippets e.g. via GHNS without caring about
      // what indent-style to use.
      m_doc->align(m_doc->activeKateView(), *m_wholeTemplateRange);
    }
  }

  ///TODO: maybe support delayed actions, i.e.:
  /// - create doc
  /// - insert template
  /// - create view => ranges are added
  /// for now simply "just insert" the code when we have no active view
  if ( !initialValues.isEmpty() && m_doc->activeView() ) {
    // only do complex stuff when required

    handleTemplateString(initialValues);
    m_doc->undoSafePoint();
    m_doc->editEnd();

    if ( !m_templateRanges.isEmpty() ) {
      foreach ( View* view, m_doc->views() ) {
        setupEventHandler(view);
      }

      connect(m_doc, SIGNAL(viewCreated(KTextEditor::Document*, KTextEditor::View*)),
              this, SLOT(slotViewCreated(KTextEditor::Document*, KTextEditor::View*)));
      connect(m_doc, SIGNAL(textChanged(KTextEditor::Document*, KTextEditor::Range, KTextEditor::Range)),
              this, SLOT(slotTextChanged(KTextEditor::Document*, KTextEditor::Range)));
      connect(m_doc, SIGNAL(textInserted(KTextEditor::Document*, KTextEditor::Range)),
              this, SLOT(slotTextChanged(KTextEditor::Document*, KTextEditor::Range)));
      connect(m_doc, SIGNAL(textRemoved(KTextEditor::Document*, KTextEditor::Range)),
              this, SLOT(slotTextChanged(KTextEditor::Document*, KTextEditor::Range)));

      setEditWithUndo(undoManager->isActive());

      connect(undoManager, SIGNAL(isActiveChanged(bool)),
              this, SLOT(setEditWithUndo(bool)));
    } else {
      // when no interesting ranges got added, we can terminate directly
      jumpToFinalCursorPosition();
      cleanupAndExit();
    }
  } else {
    m_doc->undoSafePoint();
    m_doc->editEnd();
    // simple templates just need to be (which gets done in handleTemplateString())
    cleanupAndExit();
  }
}

KateTemplateHandler::~KateTemplateHandler()
{
}

void KateTemplateHandler::slotTemplateInserted(Document *document, const Range& range)
{
  Q_ASSERT(document == m_doc);
  Q_UNUSED(document);
  ifDebug(kDebug() << "template range inserted" << range;)

  m_wholeTemplateRange = m_doc->newSmartRange( range, 0, SmartRange::ExpandLeft | SmartRange::ExpandRight );

  disconnect(m_doc, SIGNAL(textInserted(KTextEditor::Document*,KTextEditor::Range)),
             this, SLOT(slotTemplateInserted(KTextEditor::Document*,KTextEditor::Range)));
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
  ifDebug(kDebug() << "cleaning up and exiting";)
  disconnect(m_doc, SIGNAL(viewCreated(KTextEditor::Document*,KTextEditor::View*)),
             this, SLOT(slotViewCreated(KTextEditor::Document*,KTextEditor::View*)));
  disconnect(m_doc, SIGNAL(textChanged(KTextEditor::Document*,KTextEditor::Range,KTextEditor::Range)),
             this, SLOT(slotTextChanged(KTextEditor::Document*,KTextEditor::Range)));
  disconnect(m_doc, SIGNAL(textInserted(KTextEditor::Document*,KTextEditor::Range)),
             this, SLOT(slotTextChanged(KTextEditor::Document*,KTextEditor::Range)));
  disconnect(m_doc, SIGNAL(textRemoved(KTextEditor::Document*,KTextEditor::Range)),
             this, SLOT(slotTextChanged(KTextEditor::Document*,KTextEditor::Range)));

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
  delete this;
}

void KateTemplateHandler::jumpToFinalCursorPosition()
{
  if ( m_doc->activeView() && (!m_wholeTemplateRange
        || customContains(m_wholeTemplateRange, m_doc->activeView()->cursorPosition())) )
  {
    m_doc->activeView()->setSelection(Range::invalid());
    m_doc->activeView()->setCursorPosition(*m_finalCursorPosition);
  }
}

void KateTemplateHandler::setEditWithUndo(const bool &enabled)
{
  m_editWithUndo = enabled;
}

void KateTemplateHandler::slotViewCreated(Document* document, View* view)
{
  Q_ASSERT(document == m_doc); Q_UNUSED(document)
  setupEventHandler(view);
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
    if ( keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab ) {
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
        jumpToFinalCursorPosition();
        cleanupAndExit();
        return true;
      }
    } else if ( keyEvent->key() == Qt::Key_Tab && !m_doc->activeKateView()->isCompletionActive() ) {
      if ( keyEvent->modifiers() & Qt::Key_Shift ) {
        jumpToPreviousRange();
      } else {
        jumpToNextRange();
      }
      return true;
    } else if ( keyEvent->key() == Qt::Key_Backtab && !m_doc->activeKateView()->isCompletionActive() ) {
      jumpToPreviousRange();
      return true;
    }
  }
  return QObject::eventFilter(object, event);
}

void KateTemplateHandler::jumpToPreviousRange()
{
  const Cursor & cursor = m_doc->activeView()->cursorPosition();
  if ( cursor == *m_finalCursorPosition ) {
    // wrap and jump to last range
    setCurrentRange(m_templateRanges.last());
    return;
  }
  SmartRange* previousRange = 0;
  foreach ( SmartRange* range, m_templateRanges ) {
    if ( range->start() >= cursor ) {
      continue;
    }
    if ( !previousRange || range->start() > previousRange->start() ) {
      previousRange = range;
    }
  }
  if ( previousRange ) {
    setCurrentRange(previousRange);
  } else {
    // wrap and jump to final cursor
    jumpToFinalCursorPosition();
  }
}

void KateTemplateHandler::jumpToNextRange()
{
  const Cursor & cursor = m_doc->activeView()->cursorPosition();
  if ( cursor == *m_finalCursorPosition ) {
    // wrap and jump to first range
    setCurrentRange(m_templateRanges.first());
    return;
  }
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
    // wrap and jump to final cursor
    jumpToFinalCursorPosition();
  }
}

void KateTemplateHandler::setCurrentRange(SmartRange* range)
{
  if ( !range->childRanges().isEmpty() ) {
    // jump to first mirrored range
    range = range->childRanges().first();
  }

  if ( m_doc->activeView() ) {
    m_jumping = true;
    if ( m_uneditedRanges.contains(range) ) {
      m_doc->activeView()->setSelection(*range);
    }
    m_doc->activeView()->setCursorPosition(range->start());
    m_jumping = false;
  }

  m_lastCaretPosition = range->start();
}

/**
 * Returns an attribute with \p color as background with @p alpha alpha value.
 */
Attribute::Ptr getAttribute(QColor color, int alpha = 230)
{
  Attribute::Ptr attribute(new Attribute());
  color.setAlpha(alpha);
  attribute->setBackground(QBrush(color));
  return attribute;
}

void KateTemplateHandler::handleTemplateString(const QMap< QString, QString >& initialValues)
{
  QString templateString = m_doc->text(*m_wholeTemplateRange);

  int line = m_wholeTemplateRange->start().line();
  int column = m_wholeTemplateRange->start().column();

  // not equal -1 when we found a start position
  int startPos = -1;

  // each found variable gets it's range(s) added to the list.
  // the key is the varname, e.g. the same as in initialValues
  // to be able to iterate over them in a FIFO matter, also store
  // the keys in a queue.
  QQueue<QString> keyQueue;
  QMultiMap<QString, Range> ranges;
  QMap<Range,MirrorBehaviour> mirrorBehaviourBuildHelper;

  // valid, if we find an occurrence of ${cursor}
  Cursor finalCursorPosition = Cursor::invalid();

  // parse string for ${VAR} or %{VAR}
  // VAR must not contain $ or %
  // VAR must not contain newlines
  // VAR must be set as key in initialValues
  // expression must not be escaped
  for ( int i = 0; i < templateString.size(); ++i ) {
    ifDebug(kDebug()<<"checking character:"<<templateString[i];);
    if ( templateString[i] == '\n' ) {
      ++line;
      column = 0;
      if ( startPos != -1 ) {
        // don't allow variables to span multiple lines
        startPos = -1;
      }
    } else if ( (templateString[i] == '%' || templateString[i] == '$')
                && i + 1 < templateString.size() && templateString[i+1] == '{' ) {
      // check whether this var is escaped
      int escapeChars = 0;
      while ( i - escapeChars > 0 && templateString[i - escapeChars - 1] == '\\' ) {
        ++escapeChars;
      }
      if ( escapeChars > 0 ) {
        ifDebug(kDebug() << "found" << escapeChars << "escape chars at " << templateString.mid(i - escapeChars - 10, escapeChars + 10);)
        // remove half of the escape chars (i.e. \\ => \) and make sure the
        // odd rest is removed as well (i.e. the one that escapes this var)
        int toRemove = (escapeChars + 1) / 2;
        ifDebug(kDebug() << "will remove" << toRemove << "of those escape chars";)
        templateString.remove(i - escapeChars, toRemove);
        i -= toRemove;
        column -= toRemove;
      }
      if ( escapeChars % 2 == 0 ) {
        // don't check for startPos == -1 here, overwrite blindly since nested variables are not supported
        startPos = i;
      }
      // skip '{'
      ++i;
      column += 2;
    } else if ( (startPos!=-1) && (templateString[i] == '/') ) { // skip regexp
      i++;
      column++;
      int backslash_count=0;
      int slashcount=1;
      for (;i<templateString.size();i++,column++) {
        if ( templateString[i] == '\n' ) {
          ++line;
          column = 0;
          if ( startPos != -1 ) {
            // don't allow variables to span multiple lines
            startPos = -1;
          }
          break;
        }
        if (templateString[i]=='/') {
          if ((backslash_count % 2) ==0)
            slashcount++;
          backslash_count=0;
        } else if (templateString[i]=='\\') {
          backslash_count++;
        } else { // any character teminates a backslash sequence
          backslash_count=0; 
        }
        if (slashcount==3) {
          column++;
          break;
        }
      }      
    } else if ( templateString[i] == '}' && startPos != -1 ) {
      // get key, i.e. contents between ${..}
      QString key = templateString.mid( startPos + 2, i - (startPos + 2) );
      int keyLength=key.length();
      QString searchReplace;
      ifDebug(kDebug() << "key found:" << key;)
      if (key.contains("/")) {
        searchReplace=key.mid(key.indexOf("/")+1);
        key=key.left(key.indexOf("/"));
        ifDebug(kDebug() << "real key found:" << key;)
        ifDebug(kDebug() << "search_replace" << searchReplace;)
      }
      if ( !initialValues.contains(key) ) {
        kWarning() << "unknown variable key:" << key;
      } else if ( key == "cursor" ) {
        finalCursorPosition = Cursor(line, column - keyLength - 2);
        // don't insert anything, just remove the placeholder
        templateString.remove(startPos, i - startPos + 1);
        // correct iterator pos, 3 == $ + { + }
        i -= 3 + keyLength;
        column -= 2 + keyLength;
        startPos = -1;
      } else {
        MirrorBehaviour behaviour;
        if (!searchReplace.isEmpty()) {
          QString search;
          bool searchValid=false;
          QString replace;
          bool replaceValid=false;
          QString flags;
          //search part;
          while (!searchReplace.isEmpty()) {
            ifDebug(kDebug()<<"searchReplace="<<searchReplace;)
            int regescapes=0;
            int pos=searchReplace.indexOf("/");
            for (int epos=pos-1;(epos>=0) && (searchReplace.at(epos)=='\\');epos--)
              regescapes++;
            ifDebug(kDebug()<<"regescapes="<<regescapes;)
            if ((regescapes%2)==1) {
              search+=searchReplace.left(pos+1);
              searchReplace=searchReplace.mid(pos+1);
              ifDebug(kDebug()<<"intermediate search string is="<<search;)
            } else {
              search+=searchReplace.left(pos);
              searchReplace=searchReplace.mid(pos+1);
              searchValid=true;
              ifDebug(kDebug()<<"final search string ist="<<search;)
              ifDebug(kDebug()<<"remaining characters in searchReplace"<<searchReplace;)
              break;
            }
          }
          //replace part
          if (searchValid) {
            uint last_slash=searchReplace.lastIndexOf("/");
            if (last_slash!=-1) {
              replace=searchReplace.left(last_slash);
              replaceValid=true;
              flags=searchReplace.mid(last_slash+1);
            }
          }
          if (searchValid && replaceValid)
            behaviour=MirrorBehaviour(search,replace,flags);
        }
        const QString initialVal=behaviour.getMirrorString(initialValues[key]);

        // whether the variable starts with % or $
        QChar c = templateString[startPos];
        // replace variable with initial value
        templateString.replace( startPos, i - startPos + 1, initialVal );
        // correct iterator pos, 3 == % + { + }
        i -= 3 + keyLength - initialVal.length();
        // correct column to point at end of range, taking replacement width diff into account
        // 2 == % + {

        column -= 2 + keyLength - initialVal.length();
        // always add ${...} to the editable ranges
        // only add %{...} to the editable ranges when it's value equals the key
        ifDebug(kDebug() << "char is:" << c << "initial value is:" << initialValues[key] <<" after fixup is:"<<initialVal)
        if ( c == '$' || key == initialValues[key] ) {
          if ( !keyQueue.contains(key) ) {
            keyQueue.append(key);
          }
          Range tmp;
          ranges.insert( key,
                        tmp=Range( line, column - initialVal.length(),
                                line, column
                              )
                        );
          mirrorBehaviourBuildHelper.insert(tmp,behaviour);

          ifDebug(kDebug() << "range is:" << Range( line, column - initialVal.length(),
                                line, column
                              );)
        }
      }
      startPos = -1;
      ifDebug(kDebug()<<"i="<<i<<" template size="<<templateString.size(););
    } else {
      ++column;
    }
  }

  m_doc->replaceText(*m_wholeTemplateRange, templateString);

  Q_ASSERT(!m_wholeTemplateRange->isEmpty());

  if ( m_doc->activeKateView() ) {
    ///HACK: make sure the view cache is up2date so we can safely set the cursor position
    ///      else one might hit the Q_ASSERT(false) in KateViewInternal::endPos()
    ///      the cause is that the view get's ::update()'d, but that does not trigger a
    ///      paint event right away.
    m_doc->activeKateView()->updateView(true);
  }

  if ( finalCursorPosition.isValid() ) {
    m_finalCursorPosition = m_doc->newSmartCursor(finalCursorPosition);
  } else {
    m_finalCursorPosition = m_doc->newSmartCursor(Cursor(line, column));
  }

  if ( ranges.isEmpty() ) {
    return;
  }

  KateRendererConfig *config = m_doc->activeKateView()->renderer()->config();

  Attribute::Ptr editableAttribute = getAttribute(config->templateEditablePlaceholderColor());
  editableAttribute->setDynamicAttribute(
      Attribute::ActivateCaretIn, getAttribute(config->templateFocusedEditablePlaceholderColor(), 255)
  );

  Attribute::Ptr mirroredAttribute = getAttribute(config->templateNotEditablePlaceholderColor());

  m_wholeTemplateRange->setAttribute(getAttribute(config->templateBackgroundColor(), 200));
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
          m_uneditedRanges.append(range);
        } else {
          range->setAttribute(mirroredAttribute);
          m_mirrorBehaviour.insert(range,mirrorBehaviourBuildHelper[values[i]]);
        }
      }
    } else {
      // just a single range
      parent->setAttribute(editableAttribute);
      m_uneditedRanges.append(parent);
    }

    m_doc->addHighlightToDocument(parent, true);
    m_templateRanges.append(parent);
  }

  setCurrentRange(m_templateRanges.first());
  mirrorBehaviourBuildHelper.clear();
}

void KateTemplateHandler::slotTextChanged(Document* document, const Range& range)
{
  Q_ASSERT(document == m_doc);

  ifDebug(kDebug() << "text changed" << document << range;)

  if ( m_isMirroring ) {
    ifDebug(kDebug() << "mirroring - ignoring edit";)
    return;
  }
  if ( (!m_editWithUndo && m_doc->isEditRunning()) || range.isEmpty() ) {
    ifDebug(kDebug(13020) << "slotTextChanged returning prematurely";)
    return;
  }
  if ( m_wholeTemplateRange->isEmpty() ) {
    ifDebug(kDebug() << "template range got deleted, exiting";)
    cleanupAndExit();
    return;
  }
  if ( range.end() == *m_finalCursorPosition ) {
    ifDebug(kDebug() << "editing at final cursor position, exiting.";)
    cleanupAndExit();
    return;
  }
  if ( !m_wholeTemplateRange->contains(range.start()) ) {
    // outside our template or one of our own changes
    ifDebug(kDebug() << range << "not contained in" << *m_wholeTemplateRange;)
    return;
  }

  ifDebug(kDebug() << "see if we have to mirror the edit";)

  // Check if @p range is mirrored.
  // If we have two adjacent mirrored ranges, think ${first}${second},
  // the second will be mirrored, as that's what the user will see "activated",
  // since Kate uses contains() that does a comparison "< end()".
  // We use "<= end()" though so we can handle contents that were added at the end of a range.
  // TODO: make it possible to select either, the left or right rannge (LOWPRIO)

  // The found child range to act as base for mirroring.
  SmartRange* baseRange = 0;
  // The left-adjacent range that gets some special treatment (if it exists).
  SmartRange* leftAdjacentRange = 0;

  foreach ( SmartRange* parent, m_templateRanges ) {
    if ( customContains(parent, range.start()) )
    {
      if ( parent->childRanges().isEmpty() ) {
        // simple, not-mirrored range got changed
        m_uneditedRanges.removeOne(parent);
      } else {
        // handle mirrored ranges
        if ( baseRange ) {
          // look for adjacent range (we find the right-handed one here)
          ifDebug(kDebug() << "looking for adjacent mirror to" << *baseRange;)
          foreach ( SmartRange* child, parent->childRanges() ) {
            if ( child->start() == range.start() && child->end() >= range.end() ) {
              ifDebug(kDebug() << "found adjacent mirror - using as base" << *child;)
              leftAdjacentRange = baseRange;
              baseRange = child;
              break;
            }
          }
          // adjacent mirror handled, we can finish
          break;
        } else {
          // find mirrored range that got edited
          foreach ( SmartRange* child, parent->childRanges() ) {
            if ( customContains(child, range.start()) ) {
              baseRange = child;
              break;
            }
          }
          if ( baseRange && baseRange->end() != range.end() ) {
            // finish, don't look for adjacent mirror as we are not at the end of this range
            break;
          } else if ( baseRange && (baseRange->isEmpty() || range == *baseRange) ) {
            // always add to empty ranges first. else ${foo}${bar} will make foo unusable when
            // it gets selected and an edit takes place.
            break;
          } // else don't finish, look for baseRange or adjacent mirror first
        }
      }
    } else if ( baseRange ) {
      // no adjacent mirrored range found, we can finish
      break;
    }
  }
  if ( baseRange ) {
    m_uneditedRanges.removeOne(baseRange);
    if ( leftAdjacentRange ) {
      // revert that something got added to the adjacent range
      ifDebug(kDebug() << "removing edited range" << range << "from left adjacent range" << *leftAdjacentRange;)
      leftAdjacentRange->end() = range.start();
      ifDebug(kDebug() << "new range:" << *leftAdjacentRange;)
    }
    syncMirroredRanges(baseRange);
    if ( range.start() == baseRange->start() && m_doc->activeKateView() ) {
      // TODO: update the attribute, since kate doesn't do that automatically
      // TODO: fix in kate itself
      // TODO: attribute->changed == undefined reference...
    }
  } else {
    ifDebug(kDebug() << "no range found to mirror this edit";)
  }
}

void KateTemplateHandler::syncMirroredRanges(SmartRange* range)
{
  Q_ASSERT(m_templateRanges.contains(range->parentRange()));

  m_isMirroring = true;
  m_doc->editStart();

  const QString &newText = m_doc->text(*range);
  ifDebug(kDebug() << "mirroring" << newText << "from" << *range;)

  foreach ( SmartRange* sibling, range->parentRange()->childRanges() ) {
    if ( sibling != range ) {
      m_doc->replaceText(*sibling, m_mirrorBehaviour[sibling].getMirrorString(newText));
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



//BEGIN MIRROR BEHAVIOUR
  KateTemplateHandler::MirrorBehaviour::MirrorBehaviour():
    m_behaviour(Clone){}
  
  KateTemplateHandler::MirrorBehaviour::MirrorBehaviour(const QString &regexp, const QString &replacement,const QString& flags):
    m_behaviour(Regexp),m_search(regexp),m_replace(replacement) {
      m_global=flags.contains("g");
      m_expr=QRegExp(regexp,flags.contains("i")?Qt::CaseInsensitive:Qt::CaseSensitive,QRegExp::RegExp2);
  }

  KateTemplateHandler::MirrorBehaviour::~MirrorBehaviour(){}

  
  QString KateTemplateHandler::MirrorBehaviour::getMirrorString(const QString &source) {
    QString ahead;
    QString output;
    QString finalOutput;
    int pos;
    int matchCounter=0;
    switch (m_behaviour) {
      case Clone:
        return source;
        break;
      case Regexp: {
        ifDebug(kDebug() << "regexp "<< m_search << " replacement " << m_replace;)
        if (m_global) {
          ahead=source;
          while (ahead.length()>0) {
            if ((pos=m_expr.indexIn(ahead))==-1) return finalOutput+ahead;
            KateMatch match (0, KTextEditor::Search::Default);
            QStringList results = m_expr.capturedTexts();
            output = match.buildReplacement (m_replace, false, ++matchCounter, &results);
            finalOutput=finalOutput+ahead.left(pos)+output;
            ahead=ahead.mid(pos+m_expr.matchedLength());
          }
          return finalOutput;
        } else {
         if ((pos=m_expr.indexIn(source))==-1) return source;
         KateMatch match (0, KTextEditor::Search::Default);
         QStringList results = m_expr.capturedTexts();
         output = match.buildReplacement (m_replace, false, 1, &results);
         return source.left(pos)+output+source.mid(pos+m_expr.matchedLength());
         break;
        }
        break;
      }
      case Scripted:
      default:
        return QString();
    }
    return QString();
  }

//END MIRROR BEHAVOUR


#include "katetemplatehandler.moc"

// kate: space-indent on; indent-width 2; replace-tabs on;
