/* This file is part of the KDE libraries
   Copyright (C) 2001, 2003 Christoph Cullmann <cullmann@kde.org>

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

#ifndef _KATE_CMD_H
#define _KATE_CMD_H

#include <ktexteditor/commandinterface.h>

#include <kcompletion.h>

#include <QHash>
#include <QStringList>

class KateCmd
{
  public:
    KateCmd ();
    ~KateCmd ();

    static KateCmd *self ();

    bool registerCommand (KTextEditor::Command *cmd);
    bool unregisterCommand (KTextEditor::Command *cmd);
    KTextEditor::Command *queryCommand (const QString &cmd);

    QStringList cmds ();
    void appendHistory( const QString &cmd );
    const QString fromHistory( int i ) const;
    uint historyLength() const { return m_history.count(); }

  private:
    QHash<QString, KTextEditor::Command *> m_dict;
    QStringList m_cmds;
    QStringList m_history;
};

/**
 * A KCompletion object that completes last ?unquoted? word in the string
 * passed. Dont mistake "shell" for anything related to quoting, this
 * simply mimics shell tab completion by completing the last word in the
 * provided text.
 */
class KateCmdShellCompletion : public KCompletion
{
  public:
    KateCmdShellCompletion();

    /**
     * Finds completions to the given text.
     * The first match is returned and emitted in the signal match().
     * @param text the text to complete
     * @return the first match, or QString::null if not found
     */
    QString makeCompletion(const QString &text);

  protected:
        // Called by KCompletion
    void postProcessMatch( QString *match ) const;
    void postProcessMatches( QStringList *matches ) const;
    void postProcessMatches( KCompletionMatches *matches ) const;

  private:
  /**
   * Split text at the last unquoted space
   *
   * @param text_start will be set to the text at the left, including the space
   * @param text_compl Will be set to the text at the right. This is the text to complete.
   */
   void splitText( const QString &text, QString &text_start, QString &text_compl ) const;

   QChar m_word_break_char;
   QChar m_quote_char1;
   QChar m_quote_char2;
   QChar m_escape_char;

   QString m_text_start;
   QString m_text_compl;

};

#endif
