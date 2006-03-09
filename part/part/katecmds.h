/* This file is part of the KDE libraries
   Copyright (C) 2003 Anders Lund <anders@alweb.dk>
   Copyright (C) 2001-2004 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Charles Samuels <charles@kde.org>

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

#ifndef __KATE_CMDS_H__
#define __KATE_CMDS_H__

#include <ktexteditor/commandinterface.h>

#include <QStringList>

class KateDocument;
class KCompletion;

namespace KateCommands
{

/**
 * This KTextEditor::Command provides access to a lot of the core functionality
 * of kate part, settings, utilities, navigation etc.
 * it needs to get a kateview pointer, it will cast the kate::view pointer
 * hard to kateview
 */
class CoreCommands : public KTextEditor::Command, public KTextEditor::CommandExtension
{
  public:
    /**
     * execute command
     * @param view view to use for execution
     * @param cmd cmd string
     * @param errorMsg error to return if no success
     * @return success
     */
    bool exec( class KTextEditor::View *view, const QString &cmd, QString &errorMsg );

    bool help( class KTextEditor::View *, const QString &, QString & ) {return false;};

    /**
     * supported commands as prefixes
     * @return prefix list
     */
    const QStringList &cmds();

    /**
    * override completionObject from interfaces/document.h .
    */
    KCompletion *completionObject( KTextEditor::View *, const QString & );
    
    virtual void flagCompletions( QStringList& ) {}
    virtual bool wantsToProcessText( const QString & ) { return false; }
    virtual void processText( KTextEditor::View *, const QString & ) {}
};

/**
 * -- Charles Samuels <charles@kde.org>
 * Support vim/sed find and replace
 * s/search/replace/ find search, replace with replace on this line
 * %s/search/replace/ do the same to the whole file
 * s/search/replace/i do the S. and R., but case insensitively
 * $s/search/replace/ do the search are replacement to the selection only
 *
 * $s/// is currently unsupported
 **/
class SedReplace : public KTextEditor::Command
{
  public:
    /**
     * execute command
     * @param view view to use for execution
     * @param cmd cmd string
     * @param errorMsg error to return if no success
     * @return success
     */
    bool exec (class KTextEditor::View *view, const QString &cmd, QString &errorMsg);

    bool help (class KTextEditor::View *, const QString &, QString &) { return false; };

    /**
     * supported commands as prefixes
     * @return prefix list
     */
    const QStringList &cmds () { static QStringList l("s"); if (l.isEmpty()) l << "%s" << "$s"; return l; };

  private:
    /**
     * Searches one line and does the replacement in the document.
     * If @p replace contains any newline characters, the reamaining part of the
     * line is searched, and the @p line set to the last line number searched.
     * @return the number of replacements performed.
     * @param doc a pointer to the document to work on
     * @param line the number of the line to search. This may be changed by the
     * function, if newlines are inserted.
     * @param find A regular expression pattern to use for searching
     * @param replace a template for replacement. Backspaced integers are
     * replaced with captured texts from the regular expression.
     * @param delim the delimiter character from the command. In the replacement
     * text backsplashes preceeding this character are removed.
     * @param nocase parameter for matching the reqular expression.
     * @param repeat If false, the search is stopped after the first match.
     * @param startcol The position in the line to start the search.
     * @param endcol The last collumn in the line allowed in a match.
     * If it is -1, the whole line is used.
     */
    static int sedMagic(KateDocument *doc, int &line,
                        const QString &find, const QString &replace, const QString &delim,
                        bool noCase, bool repeat,
                        uint startcol=0, int endcol=-1);
};

/**
 * insert a unicode or ascii character
 * base 9+1: 1234
 * hex: 0x1234 or x1234
 * octal: 01231
 *
 * prefixed with "char:"
 **/
class Character : public KTextEditor::Command
{
  public:
    /**
     * execute command
     * @param view view to use for execution
     * @param cmd cmd string
     * @param errorMsg error to return if no success
     * @return success
     */
    bool exec (class KTextEditor::View *view, const QString &cmd, QString &errorMsg);

    bool help (class KTextEditor::View *, const QString &, QString &) { return false; };

    /**
     * supported commands as prefixes
     * @return prefix list
     */
    const QStringList &cmds () { static QStringList test("char"); return test; };
};

/**
 * insert the current date/time in the given format
 */
class Date : public KTextEditor::Command
{
  public:
    /**
     * execute command
     * @param view view to use for execution
     * @param cmd cmd string
     * @param errorMsg error to return if no success
     * @return success
     */
    bool exec (class KTextEditor::View *view, const QString &cmd, QString &errorMsg);

    bool help (class KTextEditor::View *, const QString &, QString &) { return false; };

    /**
     * supported commands as prefixes
     * @return prefix list
     */
    const QStringList &cmds () { static QStringList test("date"); return test; }
};


} // namespace KateCommands
#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
