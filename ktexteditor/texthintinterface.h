/* This file is part of the KDE libraries
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2013 Dominik Haumann <dhaumann@kde.org>

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
#ifndef KDELIBS_KTEXTEDITOR_TEXTHINTINTERFACE_H
#define KDELIBS_KTEXTEDITOR_TEXTHINTINTERFACE_H

#include <QtCore/QString>

#include <ktexteditor/ktexteditor_export.h>

#include <ktexteditor/cursor.h>

namespace KTextEditor
{

/**
 * \brief Text hint interface showing tool tips under the mouse for the View.
 *
 * \ingroup kte_group_view_extensions
 *
 * \section texthint_introduction Introduction
 *
 * The text hint interface provides a way to show tool tips for text located
 * under the mouse. Possible applications include showing a value of a variable
 * when debugging an application, or showing a complete path of an include
 * directive.
 *
 * By default, the text hint interface is disable for the View. To enable it,
 * call enableTextHints() with the desired timeout. The timeout specifies the
 * delay the user needs to hover over the text before the tool tip is shown.
 * Therefore, the timeout should not be too large, a value of 200 milliseconds
 * is recommended.
 *
 * Once text hints are enabled, the signal needTextHint() is emitted after the
 * timeout whenever the mouse moved to a new text position in the View.
 * Therefore, in order to show a tool tip, you need to connect to this signal
 * and then fill the parameter \p text with the text to display.
 *
 * To disable all text hints, call disableTextHints(). This, however will disable
 * the text hints entirely for the View. If there are multiple users of the
 * TextHintInterface, this might lead to a conflict.
 *
 * \section texthint_access Accessing the TextHintInterface
 *
 * The TextHintInterface is an extension interface for a
 * View, i.e. the View inherits the interface \e provided that the
 * used KTextEditor library implements the interface. Use qobject_cast to
 * access the interface:
 * \code
 * // view is of type KTextEditor::View*
 * KTextEditor::TextHintInterface *iface =
 *     qobject_cast<KTextEditor::TextHintInterface*>( view );
 *
 * if( iface ) {
 *     // the implementation supports the interface
 *     // do stuff
 * }
 * \endcode
 *
 * \since KDE 4.11
 */
class KTEXTEDITOR_EXPORT TextHintInterface
{
  public:
    TextHintInterface();
    virtual ~TextHintInterface();

    /**
     * Enable text hints with the specified \p timeout in milliseconds.
     * The timeout specifies the delay the user needs to hover over the text
     * befure the tool tip is shown. Therefore, \p timeout should not be
     * too large, a value of 200 milliseconds is recommended.
     *     
     * After enabling the text hints, the signal needTextHint() is emitted
     * whenever the mouse position changed and a new character is underneath
     * the mouse cursor. Calling the signal is delayed for the time specified
     * by \p timeout.
     *
     * \param timeout tool tip delay in milliseconds
     *
     * \todo KDE5 add default value for timeout
     */
    virtual void enableTextHints(int timeout) = 0;

    /**
     * Disable all text hints for the view.
     * By default, text hints are disabled.
     */
    virtual void disableTextHints() = 0;

  //
  // signals
  //
  public:
    /**
     * This signal is emitted whenever the timeout for displaying a text hint
     * is triggered. The text cursor \p position specifies the mouse position
     * in the text. To show a text hint, fill \p text with the text to be
     * displayed. If you do not want a tool tip to be displayed, set \p text to
     * an empty QString() in the connected slot.
     *
     * \param position text cursor under the mouse position
     * \param text tool tip to be displayed, or empty string to hide
     *
     * \todo KDE5: add first parameter "KTextEditor::View * view"
     */
    virtual void needTextHint(const KTextEditor::Cursor& position, QString &text) = 0;

  private:
    class TextHintInterfacePrivate* const d;
};

}

Q_DECLARE_INTERFACE(KTextEditor::TextHintInterface, "org.kde.KTextEditor.TextHintInterface")

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
