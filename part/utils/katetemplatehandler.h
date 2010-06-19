/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2004,2010 Joseph Wenninger <jowenn@kde.org>
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

#ifndef _KATE_TEMPLATE_HANDLER_H_
#define _KATE_TEMPLATE_HANDLER_H_

#include <QtCore/QPointer>
#include <QtCore/QObject>
#include <QtCore/QMap>
#include <QtCore/QHash>
#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QRegExp>

class KateDocument;

class KateView;

class KateUndoManager;

class KateTemplateScript;

namespace KTextEditor
{

class MovingCursor;

class MovingRange;
}

/**
 * \brief Inserts a template and offers advanced snippet features, like navigation and mirroring.
 *
 * For each template inserted a new KateTemplateHandler will be created.
 *
 * The handler has the following features:
 *
 * \li It inserts the template string into the document at the requested position.
 * \li When the template contains at least one variable, the cursor will be placed
 *     at the start of the first variable and its range gets selected.
 * \li When more than one variable exists,TAB and SHIFT TAB can be used to navigate
 *     to the next/previous variable.
 * \li When a variable occurs more than once in the template, edits to any of the
 *     occurrences will be mirroed to the other ones.
 * \li When ESC is pressed, the template handler closes.
 * \li When ALT + RETURN is pressed and a \c %{cursor} variable
 *     exists in the template,the cursor will be placed there. Else the cursor will
 *     be placed at the end of the template.
 *
 * \see KateDocument::insertTemplateTextImplementation(), KTextEditor::TemplateInterface
 *
 * \author Milian Wolff <mail@milianw.de>
 */

class KateTemplateHandler: public QObject
{
  Q_OBJECT

public:
  /**
   * Setup the template handler, insert the template string.
   *
   * NOTE: The handler deletes itself when required, you do not need to
   *       keep track of it.
   */
  KateTemplateHandler(KateView *view,
                      const KTextEditor::Cursor& position,
                      const QString &templateString,
                      const QMap<QString, QString> &initialValues,
                      KateUndoManager* undoManager,
                      KateTemplateScript* templateScript);

  /**
   * Cancels the template handler and cleans everything up.
   */
  virtual ~KateTemplateHandler();

protected:
  /**
   * \brief Provide keyboard interaction for the template handler.
   *
   * The event filter handles the following shortcuts:
   *
   * TAB: jump to next editable (i.e. not mirrored) range.
   *      NOTE: this prevents indenting via TAB.
   * SHIFT + TAB: jump to previous editable (i.e. not mirrored) range.
   *      NOTE: this prevents un-indenting via SHIFT + TAB.
   * ESC: terminate template handler (only when no completion is active).
   * ALT + RETURN: accept template and jump to the end-cursor.
   *               if %{cursor} was given in the template, that will be the
   *               end-cursor.
   *               else just jump to the end of the inserted text.
   */
  virtual bool eventFilter(QObject* object, QEvent* event);

private:
  /**
   * Inserts the @p text at @p position and sets an undo point.
   */
  void insertText(const KTextEditor::Cursor& position, const QString& text);

  /**
   * Parses the inserted & indented template string (in \p m_wholeTemplateRange)
   * for variables and replaces their palceholders with their \p initialValues.
   * Also create child ranges to \p m_templateRange for editable ranges. When a
   * variable occurs more than once, create mirrored ranges.
   */
  void handleTemplateString(const QMap<QString, QString> &initialValues);

  /**
   * Install an event filter on the filter proxy of \p view for
   * navigation between the ranges and terminating the KateTemplateHandler.
   *
   * \see eventFilter()
   */
  void setupEventHandler(KTextEditor::View* view);

  /**
   * Jumps to the previous editable range. If there is none, wrap and jump to the last range.
   *
   * \see setCurrentRange(), jumpToNextRange()
   */
  void jumpToPreviousRange();

  /**
   * Jumps to the next editable range. If there is none, wrap and jump to the last range.
   *
   * \see setCurrentRange(), jumpToPreviousRange()
   */
  void jumpToNextRange();

  /**
   * Set selection to \p range and move the cursor to it's beginning.
   */
  void setCurrentRange(KTextEditor::MovingRange* range);

  /**
   * Syncs the contents of all mirrored ranges for a given variable.
   *
   * \param range The range that acts as base. It's contents will be propagated.
   *              Mirrored ranges can be found as child of a child of \p m_templateRange
   */
  void syncMirroredRanges(KTextEditor::MovingRange* range);

public:

  class MirrorBehaviour
  {

  public:
    MirrorBehaviour(); //clone
    MirrorBehaviour(const QString &regexp, const QString &replacement, const QString &flags);   //regexp
    MirrorBehaviour(KateTemplateScript* templateScript, const QString& functionName, KateTemplateHandler* handler);   //scripted
    ~MirrorBehaviour();
    QString getMirrorString(const QString &source);

  private:
    enum Behaviour {
      Clone = 0,
      Regexp = 1,
      Scripted = 2
    };
    Behaviour m_behaviour;
    QString m_search;
    QString m_replace;
    QRegExp m_expr;
    bool m_global;
    KateTemplateScript* m_templateScript;
    QString m_functionName;
    KateTemplateHandler *m_handler;
  };

private:

  QHash<KTextEditor::MovingRange*, MirrorBehaviour> m_mirrorBehaviour;


  /**
   * Jumps to the final cursor position. This is either \p m_finalCursorPosition, or
   * if that is not set, the end of \p m_templateRange.
   */
  void jumpToFinalCursorPosition();

  KateDocument *doc();

private Q_SLOTS:
  /**
   * Cleans up the template handler and deletes it.
   *
   * We cannot always do that blindly in the dtor, as it would crash
   * when we try to do the cleanup when the dtor is called by the
   * parent's dtor, i.e. when a document gets closed.
   */
  void cleanupAndExit();

  /**
   * Saves the range of the inserted template. This is required since
   * tabs could get expanded on insert. While we are at it, we can
   * use it to auto-indent the code after insert.
   */
  void slotTemplateInserted(KTextEditor::Document* document, const KTextEditor::Range& range);
  /**
   * Install event filter on new views.
   */
  void slotViewCreated(KTextEditor::Document* document, KTextEditor::View* view);
  /**
   * When the start of @p oldRange overlaps with one of our mirrored ranges,
   * update the contents of the siblings.
   *
   * @see syncMirroredRanges()
   */
  void slotTextChanged(KTextEditor::Document* document, const KTextEditor::Range& oldRange);

  /**
   * Keeps track of whether undo tracking is enabled in the document's undo manager.
   */
  void setEditWithUndo(const bool &enabled);

public:
  KateView* view();

private:
  /// The document we operate on.
  KateView *m_view;
  /// The undo manager associated with our document
  KateUndoManager *const m_undoManager;
  /// List of ranges with highlighting, marking the inserted variables in the template.
  /// Mirrored variables will have a parent range and the real ranges will be the children.
  /// Each direct child of the list is added as a Highlighting Range with dynamic attribs.
  ///
  /// Ordered by first occurrence in the document.
  ///
  /// NOTE: this design is due to some Kate limiations with overlapping ranges.
  ///       and using a structure like Parent -> NotMirrored + MirrorParents -> Mirrors
  ///       leads to overlaps, since mirrors often occur anywhere in the template text.
  QList<KTextEditor::MovingRange*> m_templateRanges;

  /// mapping of ranges to there children
  QMap<KTextEditor::MovingRange*, QList<KTextEditor::MovingRange*> > m_templateRangesChildren;
  QMap<KTextEditor::MovingRange*, KTextEditor::MovingRange*> m_templateRangesChildToParent;

  /// A range that occupies the whole range of the inserted template.
  /// When the cursor moves outside it, the template handler gets closed.
  KTextEditor::MovingRange *m_wholeTemplateRange;
  /// Position of the (last) occurrence of ${cursor} in the template string.
  KTextEditor::MovingCursor *m_finalCursorPosition;
  /// The last caret position during editing.
  KTextEditor::Cursor m_lastCaretPosition;
  /// MovingRanges that are still in this list have not yet been changed. Hence they'll
  /// be selected when they get focused. All others just get the cursor placed at the
  /// beginning.
  QList<KTextEditor::MovingRange*> m_uneditedRanges;
  /// stores the master ranges for mirroring, otherwise just the range, needed for cursor placement
  QList<KTextEditor::MovingRange*> m_masterRanges;
  /// Set to true when we are currently mirroring, to prevent recursion.
  bool m_isMirroring;
  /// Whether undo tracking is enabled in the undo manager.
  bool m_editWithUndo;
  /// Whether we are currently setting the cursor manually.
  bool m_jumping;
  /// template script pointer for the template script, which might be used by the current template
  KateTemplateScript* m_templateScript;

  QList<KTextEditor::MovingRange*> m_spacersMovingRanges;

  bool m_initialRemodify;
};


#endif

// kate: indent-mode cstyle; space-indent on; indent-width 2; replace-tabs on;  replace-tabs on;  replace-tabs on;  replace-tabs on;
