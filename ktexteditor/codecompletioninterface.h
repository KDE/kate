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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __ktexteditor_codecompletioninterface_h__
#define __ktexteditor_codecompletioninterface_h__

#include <qstring.h>
#include <qstringlist.h>

namespace KTextEditor
{

/**
 * An item for the completion popup. <code>text</code> is the completed string,
 * <code>prefix</code> appears in front of it, <code>suffix</code> appears after it. 
 * <code>type</code> does not appear in the completion list.
 * <code>prefix</code>, <code>suffix</code>, and <code>type</code> are not part of the 
 * inserted text if a completion takes place. <code>comment</code> appears in a tooltip right of
 * the completion list for the currently selected item. <code>userdata</code> can be 
 * free formed data, which the user of this interface can use in 
 * @ref CodeCompletionInterface::filterInsertString().
 *
 *
 */
class CompletionEntry
{
  public:
    QString type;
    QString text;
    QString prefix;
    QString postfix;
    QString comment;

    QString userdata;

    bool operator==( const CompletionEntry &c ) const {
      return ( c.type == type &&
	       c.text == text &&
	       c.postfix == postfix &&
	       c.prefix == prefix &&
	       c.comment == comment &&
               c.userdata == userdata);
    }
};

/**
 * This is an interface for the @ref KTextEditor::View class. It can be used
 * to show completion lists, i.e. lists that pop up while a user is typing.
 * The user can then choose from the list or he can just keep typing. The
 * completion list will disappear if an item is chosen, if no completion
 * is available or if the user presses Esc etc. The contents of the list
 * is automatically adapted to the string the user types.
 *
 * There is another signal, which may be implmemented, but isn't documented here, because
 * it would have been a BIC change and I'm not sure if it is really of need
 * 
 * void completionExtendedComment(CompletionEntry)
 *
 *

 */
class CodeCompletionInterface
{
  friend class PrivateCodeCompletionInterface;

  public:
	CodeCompletionInterface();
	virtual ~CodeCompletionInterface();
  
	unsigned int codeCompletionInterfaceNumber () const;

	//
	// slots !!!
	//
    /**
     * This shows an argument hint.
     */
    virtual void showArgHint (QStringList functionList, const QString& strWrapping, const QString& strDelimiter) = 0;

    /**
     * This shows a completion list. @p offset is the real start of the text that
     * should be completed. <code>0</code> means that the text starts at the current cursor
     * position. if @p casesensitive is @p true, the popup will only contain completions
     * that match the input text regarding case.
     */
    virtual void showCompletionBox (QValueList<CompletionEntry> complList,int offset=0, bool casesensitive=true)=0;

	//
	// signals !!!
	//
	public:


	/**
	 * This signal is emitted when the completion list disappears and no completion has
	 * been done. This is the case e.g. when the user presses Escape.
	 *
	 * IMPORTANT: Please check if a connect to this signal worked, and implement some fallback
	 * when the implementation doesn't support it
	 * 
	 * IMPORTANT FOR IMPLEMENTERS: When you don't support this signal, please just override the inherited 
	 * function, if you support it, declare it as a signal
	 */
    virtual void completionAborted()=0;

	/**
	 * This signal is emitted when the completion list disappears and a completion has
	 * been inserted into text. This is the case e.g. when the user presses Return
	 * on a selected item in the completion list.
	 *
	 * IMPORTANT: Please check if a connect to this signal worked, and implement some fallback
	 * when the implementation doesn't support it
	 * 
	 * IMPORTANT FOR IMPLEMENTERS: When you don't support this signal, please just override the inherited 
	 * function, if you support it, declare it as a signal
	 */
    virtual void completionDone()=0;

	/**
	 * This signal is the same as @ref completionDone(), but additionally it carries
	 * the information which completion item was used.
	 *
	 * IMPORTANT: Please check if a connect to this signal worked, and implement some fallback
	 * when the implementation doesn't support it
         *
         * IMPORTANT: The pointer to the CompleteionEntry, is only valid in the slots connected to this signal
         * when the connected slots are left, the data element may be destroyed, depending on the implementation
	 *
	 * IMPORTANT FOR IMPLEMENTERS: When you don't support this signal, please just override the inherited 
	 * function, if you support it, declare it as a signal.
         *
	 */
    virtual void completionDone(CompletionEntry)=0;

	/**
	 * This signal is emitted when the argument hint disappears.
	 * This is the case e.g. when the user moves the cursor somewhere else.
	 *
	 * IMPORTANT: Please check if a connect to this signal worked, and implement some fallback
	 * when the implementation doesn't support it
	 * 
	 * IMPORTANT FOR IMPLEMENTERS: When you don't support this signal, please just override the inherited 
	 * function, if you support it, declare it as a signal
	 */
    virtual void argHintHidden()=0;

	/**
	 * This signal is emitted just before a completion takes place.
	 * You can use it to modify the @ref CompletionEntry. The modified
	 * entry will not be visible in the completion list (because that has
	 * just disappeared) but it will be used when the completion is
	 * inserted into the text.
	 *
	 * IMPORTANT: Please check if a connect to this signal worked, and implement some fallback
	 * when the implementation doesn't support it
	 * 
	 * IMPORTANT FOR IMPLEMENTERS: When you don't support this signal, please just override the inherited 
	 * function, if you support it, declare it as a signal
	 */
    virtual void filterInsertString(CompletionEntry*,QString*)=0;


  private:
    class PrivateCodeCompletionInterface *d;
    static unsigned int globalCodeCompletionInterfaceNumber;
    unsigned int myCodeCompletionInterfaceNumber;
};

CodeCompletionInterface *codeCompletionInterface (class View *view);

};

#endif
