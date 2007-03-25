/* This file is part of the KDE libraries
   Copyright (C) 2005-2006 Hamish Rodda <rodda@kde.org>

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

#ifndef KDELIBS_KTEXTEDITOR_CODECOMPLETIONINTERFACE_H
#define KDELIBS_KTEXTEDITOR_CODECOMPLETIONINTERFACE_H

#include <ktexteditor/ktexteditor_export.h>
#include <QtCore/QObject>
#include <ktexteditor/range.h>

namespace KTextEditor {

class CodeCompletionModel;

class KTEXTEDITOR_EXPORT CodeCompletionInterface
{
  public:
    virtual ~CodeCompletionInterface();

    /**
     * Query whether the code completion box is currently displayed.
     */
    virtual bool isCompletionActive() const = 0;

    /**
     * Invoke code completion over a given range, with a specific \a model.
     */
    virtual void startCompletion(const Range& word, CodeCompletionModel* model) = 0;

    /**
     * Abort the currently displayed code completion without executing any currently
     * selected completion.
     */
    virtual void abortCompletion() = 0;

    /**
     * Force execution of the currently selected completion, and hide the code completion
     * box.
     */
    virtual void forceCompletion() = 0;

    /**
     * Register a new code completion \p model.
     * \param model new completion model
     * \see unregisterCompletionModel()
     */
    virtual void registerCompletionModel(CodeCompletionModel* model) = 0;

    /**
     * Unregister a code completion \p model.
     * \param model the model that should be unregistered
     * \see registerCompletionModel()
     */
    virtual void unregisterCompletionModel(CodeCompletionModel* model) = 0;

    /**
     * Determine the status of automatic code completion invocation.
     */
    virtual bool isAutomaticInvocationEnabled() const = 0;

    /**
     * Enable or disable automatic code completion invocation.
     */
    virtual void setAutomaticInvocationEnabled(bool enabled = true) = 0;

  //signals:
    //void completionExecuted(KTextEditor::View* view, const KTextEditor::Cursor& position, KTextEditor::CodeCompletionModel* model, int row);
    //void completionAborted(KTextEditor::View* view);
};

}

Q_DECLARE_INTERFACE(KTextEditor::CodeCompletionInterface, "org.kde.KTextEditor.CodeCompletionInterface")

#endif // KDELIBS_KTEXTEDITOR_CODECOMPLETIONINTERFACE_H
