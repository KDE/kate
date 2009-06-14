/* This file is part of the KDE libraries
   Copyright (C) 2008 Niko Sams <niko.sams@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KDELIBS_KTEXTEDITOR_CODECOMPLETIONMODELCONTROLLERINTERFACE_H
#define KDELIBS_KTEXTEDITOR_CODECOMPLETIONMODELCONTROLLERINTERFACE_H

#include <ktexteditor/ktexteditor_export.h>
#include <ktexteditor/smartrange.h>
#include <ktexteditor/cursor.h>
#include "codecompletionmodel.h"

class QModelIndex;

namespace KTextEditor {
class View;
/**
 * \short Controller interface for a CodeCompletionModel
 *
 * \ingroup kte_group_ccmodel_extensions
 *
 * The CodeCompletionModelControllerInterface gives an CodeCompletionModel better
 * control over the completion.
 *
 * By implementing methods defined in this interface you can:
 * - control when automatic completion should start (shouldStartCompletion())
 * - define a custom completion range (that will be replaced when the completion 
 *   is executed) (completionRange())
 * - dynamically modify the completion range during completion (updateCompletionRange())
 * - specify the string used for filtering the completion (filterString())
 * - control when completion should stop (shouldAbortCompletion())
 *
 * When the interface is not implemented, or no methods are overridden the
 * default behaviour is used, which will be correct in many situations.
 * 
 *
 * \section markext_access Implemeting the Interface
 * To use this class implement it in your CodeCompletionModel.
 *
 * \code
 * class MyCodeCompletion : public KTextEditor::CodeCompletionTestModel,
                    public KTextEditor::CodeCompletionModelControllerInterface
 * {
 *   Q_OBJECT
 *   Q_INTERFACES(KTextEditor::CodeCompletionModelControllerInterface)
 * public:
 *   KTextEditor::Range completionRange(KTextEditor::View* view, const KTextEditor::Cursor &position);
 * };
 * \endcode
 *
 * \see CodeCompletionModel
 * \author Niko Sams \<niko.sams@gmail.com\>
 * \since 4.2
 */
class KTEXTEDITOR_EXPORT CodeCompletionModelControllerInterface
{
public:
    CodeCompletionModelControllerInterface();
    virtual ~CodeCompletionModelControllerInterface();

    /**
     * This function decides if the automatic completion should be started when
     * the user entered some text.
     *
     * The default implementation will return true if the last character in
     * \p insertedText is a letter, a number, '.', '_' or '\>'
     *
     * \param view The view to generate completions for
     * \param insertedText The text that was inserted by the user
     * \param userInsertion Whether the the text was inserted by the user using typing.
     *                      If false, it may have been inserted for example by code-completion.
     * \param position Current cursor position
     * \return \e true, if the completion should be started, otherwise \e false
     */
    virtual bool shouldStartCompletion(View* view, const QString &insertedText, bool userInsertion, const Cursor &position);

    /**
     * This function returns the completion range that will be used for the
     * current completion.
     *
     * This range will be used for filtering the completion list and will get
     * replaced when executing the completion
     *
     * The default implementation will work for most languages that don't have
     * special chars in identifiers.
     *
     * \param view The view to generate completions for
     * \param position Current cursor position
     * \return the completion range
     */
    virtual Range completionRange(View* view, const Cursor &position);

    /**
     * This function lets the CompletionModel dynamically modify the range.
     * Called after every change to the range (eg. when user entered text)
     *
     * The default implementation does nothing.
     *
     * The smart-mutex is not locked when this is called.
     * @warning Make sure you lock it before you change the range
     *
     * \param view The view to generate completions for
     * \param range Reference to the current range
     */
    virtual void updateCompletionRange(View* view, SmartRange& range);

    /**
     * This function returns the filter-text used for the current completion.
     * Can return an empty string to disable filtering.
     *
     * The default implementation will return the text from \p range start to
     * the cursor \p position.
     *
     * The smart-mutex is not locked when this is called.
     *
     * \param view The view to generate completions for
     * \param range The completion range
     * \param position Current cursor position
     * \return the string used for filtering the completion list
     */
    virtual QString filterString(View* view, const SmartRange& range, const Cursor &position);

    /**
     * This function decides if the completion should be aborted.
     * Called after every change to the range (eg. when user entered text)
     *
     * The default implementation will return true when any special character was entered, or when the range is empty.
     *
     * The smart-mutex is not locked when this is called.
     *
     * \param view The view to generate completions for
     * \param range The completion range
     * \param currentCompletion The text typed so far
     * \return \e true, if the completion should be aborted, otherwise \e false
     */
    virtual bool shouldAbortCompletion(View* view, const SmartRange& range, const QString &currentCompletion);
    
    /**
     * When an item within this model is currently selected in the completion-list, and the user inserted the given character,
     * should the completion-item be executed? This can be used to execute items from other inputs than the return-key.
     * For example a function item could be executed by typing '(', or variable items by typing '.'.
     * \param selected The currently selected index
     * \param inserted The character that was inserted by tue user
     */
    virtual bool shouldExecute(const QModelIndex& selected, QChar inserted);
    
    /**
     * Notification that completion for this model has been aborted.
     * \param view The view in which the completion for this model was aborted
     */
    virtual void aborted(View* view);
};

///Extension of CodeCompletionModelControllerInterface
class KTEXTEDITOR_EXPORT CodeCompletionModelControllerInterface2 : public CodeCompletionModelControllerInterface {
  public:
    enum MatchReaction {
      None,
      HideListIfAutomaticInvocation /** If this is returned, the completion-list is hidden if it was invoked automatically */
    };
    /**
     * Called whenever an item in the completion-list perfectly matches the current filter text.
     * \param The index that is matched
     * \return Whether the completion-list should be hidden on this event. The default-implementation always returns HideListIfAutomaticInvocation
     */
    virtual MatchReaction matchingItem(const QModelIndex& matched);
};

}

Q_DECLARE_INTERFACE(KTextEditor::CodeCompletionModelControllerInterface, "org.kde.KTextEditor.CodeCompletionModelControllerInterface")
Q_DECLARE_INTERFACE(KTextEditor::CodeCompletionModelControllerInterface2, "org.kde.KTextEditor.CodeCompletionModelControllerInterface2")

#endif // KDELIBS_KTEXTEDITOR_CODECOMPLETIONMODELCONTROLLERINTERFACE_H
