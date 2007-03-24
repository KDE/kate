/* This file is part of the KDE libraries
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>

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

#ifndef KDELIBS_KTEXTEDITOR_TEXTHINTINTERFACE_H
#define KDELIBS_KTEXTEDITOR_TEXTHINTINTERFACE_H

#include <QtCore/QString>
#include <QtCore/QStringList>

#include <ktexteditor/ktexteditor_export.h>

#include <ktexteditor/cursor.h>

namespace KTextEditor
{

/**
 * This is an interface for the KTextEditor::View class.
 * \ingroup kte_group_view_extensions
 */
class KTEXTEDITOR_EXPORT TextHintInterface
{
  public:
    TextHintInterface();
    virtual ~TextHintInterface();

    /**
     * enable Texthints. If they are enabled a signal needTextHint is emitted, if the mouse
     * changed the position and a new character is beneath the mouse cursor. The signal is delayed
     * for a certain time, specifiedin the timeout parameter.
     */
    virtual void enableTextHints(int timeout)=0;

    /**
     * Disable texthints. Per default they are disabled.
     */
    virtual void disableTextHints()=0;

    //signals

    /**
     * emit this signal, if a tooltip text is needed for displaying.
     * I you don't want a tooltip to be displayd set text to an emtpy string in a connected slot,
     * otherwise set text to the string you want the editor to display
     */
	virtual void needTextHint(const KTextEditor::Cursor& position, QString &text)=0;

  private:
    class TextHintInterfacePrivate* const d;
};

}

Q_DECLARE_INTERFACE(KTextEditor::TextHintInterface, "org.kde.KTextEditor.TextHintInterface")

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
