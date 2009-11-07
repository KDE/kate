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
#ifndef _KATE_TEMPLATE_HANDLER_H_
#define _KATE_TEMPLATE_HANDLER_H_

#include "katesmartrange.h"
#include <QtCore/QPointer>
#include <QtCore/QObject>
#include <QtCore/QMap>
#include <QtCore/QHash>
#include <QtCore/QString>
#include <QtCore/QList>

class KateDocument;

namespace KTextEditor {
  class SmartRange;
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
    KateTemplateHandler(KateDocument *doc, const KTextEditor::Cursor& position,
                        const QString &templateString, const QMap<QString, QString> &initialValues);

    /**
     * Cancels the template handler and cleans everything up.
     */
    virtual ~KateTemplateHandler();

  public Q_SLOTS:
    /**
     * Keeps track of whether undo tracking is enabled in the document's undo manager.
     */
    void setEditWithUndo(const bool &enabled);

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
     * Parses the template string for variables and replaces their palceholders
     * with their \p initialValues. Also create child ranges to \p m_templateRange
     * for editable ranges. When a variable occurs more than once, create mirrored ranges.
     *
     * Also inserts the template text after evaluation.
     */
    void handleTemplateString(const KTextEditor::Cursor &position, QString templateString,
                              const QMap<QString, QString> &initialValues);

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
    void setCurrentRange(KTextEditor::SmartRange* range);

    /**
     * Syncs the contents of all mirrored ranges for a given variable.
     *
     * \param range The range that acts as base. It's contents will be propagated.
     *              Mirrored ranges can be found as child of a child of \p m_templateRange
     */
    void syncMirroredRanges(KTextEditor::SmartRange* range);

    /**
     * Cleans up the template handler and deletes it.
     *
     * We cannot always do that blindly in the dtor, as it would crash
     * when we try to do the cleanup when the dtor is called by the
     * parent's dtor, i.e. when a document gets closed.
     */
    void cleanupAndExit();

    /**
     * Jumps to the final cursor position. This is either \p m_finalCursorPosition, or
     * if that is not set, the end of \p m_templateRange.
     */
    void jumpToFinalCursorPosition();

  private Q_SLOTS:
    /**
     * Install event filter on new views.
     */
    void slotViewCreated(KTextEditor::Document* document, KTextEditor::View* view);

    /**
     * When we exit one of our ranges, jump to the next one.
     *
     * If we exit \p m_templateRange terminate the template handler.
     */
    void slotCursorPositionChanged(KTextEditor::View* view, const KTextEditor::Cursor &cursor);

    /**
     * When the start of @p oldRange overlaps with one of our mirrored ranges,
     * update the contents of the siblings.
     *
     * @see syncMirroredRanges()
     */
    void slotTextChanged(KTextEditor::Document* document, const KTextEditor::Range& oldRange);

  private:
    /// The document we operate on.
    KateDocument *m_doc;
    /// List of ranges with highlighting, marking the inserted variables in the template.
    /// Mirrored variables will have a parent range and the real ranges will be the children.
    /// Each direct child of the list is added as a Highlighting Range with dynamic attribs.
    ///
    /// Ordered by first occurrence in the document.
    ///
    /// NOTE: this design is due to some Kate limiations with overlapping ranges.
    ///       and using a structure like Parent -> NotMirrored + MirrorParents -> Mirrors
    ///       leads to overlaps, since mirrors often occur anywhere in the template text.
    QList<KTextEditor::SmartRange*> m_templateRanges;
    /// A range that occupies the whole range of the inserted template.
    /// When the cursor moves outside it, the template handler gets closed.
    KTextEditor::SmartRange* m_wholeTemplateRange;
    /// Position of the (last) occurrence of ${cursor} in the template string.
    KTextEditor::SmartCursor* m_finalCursorPosition;
    /// The last caret position during editing.
    KTextEditor::Cursor m_lastCaretPosition;
    /// Set to true when we are currently mirroring, to prevent recursion.
    bool m_isMirroring;
    /// Whether undo tracking is enabled in the undo manager.
    bool m_editWithUndo;
    /// Whether we are currently setting the cursor manually.
    bool m_jumping;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
