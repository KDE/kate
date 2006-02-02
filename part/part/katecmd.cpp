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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "katecmd.h"
#include "kateglobal.h"

#include <kdebug.h>

//BEGIN KateCmd
#define CMD_HIST_LENGTH 256

KateCmd::KateCmd ()
{
}

KateCmd::~KateCmd ()
{
}

bool KateCmd::registerCommand (KTextEditor::Command *cmd)
{
  QStringList l = cmd->cmds ();

  for (int z=0; z<l.count(); z++)
    if (m_dict.contains(l[z]))
      return false;

  for (int z=0; z<l.count(); z++) {
    m_dict.insert (l[z], cmd);
    kDebug(13050)<<"Inserted command:"<<l[z]<<endl;
  }

  m_cmds += l;

  return true;
}

bool KateCmd::unregisterCommand (KTextEditor::Command *cmd)
{
  QStringList l;

  QHash<QString, KTextEditor::Command*>::const_iterator i = m_dict.constBegin();
  while (i != m_dict.constEnd()) {
      if (i.value()==cmd) l << i.key();
      ++i;
  }

  for ( QStringList::Iterator it1 = l.begin(); it1 != l.end(); ++it1 ) {
    m_dict.remove(*it1);
    kDebug(13050)<<"Removed command:"<<*it1<<endl;
  }

  return true;
}

KTextEditor::Command *KateCmd::queryCommand (const QString &cmd)
{
  // a command can be named ".*[\w\-]+" with the constrain that it must
  // contain at least one letter.
  int f = 0;
  bool b = false;
  for ( ; f < cmd.length(); f++ )
  {
    if ( cmd[f].isLetter() )
      b = true;
    if ( b && ( ! cmd[f].isLetterOrNumber() && cmd[f] != '-' && cmd[f] != '_' ) )
      break;
  }
  return m_dict.value(cmd.left(f));
}

QStringList KateCmd::cmds ()
{
  return m_cmds;
}

KateCmd *KateCmd::self ()
{
  return KateGlobal::self()->cmdManager ();
}

void KateCmd::appendHistory( const QString &cmd )
{
  if (!m_history.isEmpty()) //this line should be backported to 3.x
    if ( m_history.last() == cmd )
      return;

  if ( m_history.count() == CMD_HIST_LENGTH )
    m_history.removeFirst();

  m_history.append( cmd );
}

const QString KateCmd::fromHistory( int index ) const
{
  if ( index < 0 || index > m_history.count() - 1 )
    return QString();
  return m_history[ index ];
}
//END KateCmd

//BEGIN KateCmdShellCompletion
/*
   A lot of the code in the below class is copied from
   kdelibs/kio/kio/kshellcompletion.cpp
   Copyright (C) 2000 David Smith <dsmith@algonet.se>
   Copyright (C) 2004 Anders Lund <anders@alweb.dk>
*/
KateCmdShellCompletion::KateCmdShellCompletion()
  : KCompletion()
{
  m_word_break_char = ' ';
  m_quote_char1 = '\"';
  m_quote_char2 = '\'';
  m_escape_char = '\\';
}

QString KateCmdShellCompletion::makeCompletion( const QString &text )
{
        // Split text at the last unquoted space
  //
  splitText(text, m_text_start, m_text_compl);

  // Make completion on the last part of text
  //
  return KCompletion::makeCompletion( m_text_compl );
}

void KateCmdShellCompletion::postProcessMatch( QString *match ) const
{
  if ( match->isNull() )
    return;

  match->prepend( m_text_start );
}

void KateCmdShellCompletion::postProcessMatches( QStringList *matches ) const
{
  for ( QStringList::Iterator it = matches->begin();
        it != matches->end(); it++ )
    if ( !(*it).isNull() )
      (*it).prepend( m_text_start );
}

void KateCmdShellCompletion::postProcessMatches( KCompletionMatches *matches ) const
{
  for ( KCompletionMatches::Iterator it = matches->begin();
        it != matches->end(); it++ )
    if ( !(*it).value().isNull() )
      (*it).value().prepend( m_text_start );
}

void KateCmdShellCompletion::splitText(const QString &text, QString &text_start,
                                 QString &text_compl) const
{
  bool in_quote = false;
  bool escaped = false;
  QChar p_last_quote_char;
  int last_unquoted_space = -1;
  int end_space_len = 0;

  for (int pos = 0; pos < text.length(); pos++) {

    end_space_len = 0;

    if ( escaped ) {
      escaped = false;
    }
    else if ( in_quote && text[pos] == p_last_quote_char ) {
      in_quote = false;
    }
    else if ( !in_quote && text[pos] == m_quote_char1 ) {
      p_last_quote_char = m_quote_char1;
      in_quote = true;
    }
    else if ( !in_quote && text[pos] == m_quote_char2 ) {
      p_last_quote_char = m_quote_char2;
      in_quote = true;
    }
    else if ( text[pos] == m_escape_char ) {
      escaped = true;
    }
    else if ( !in_quote && text[pos] == m_word_break_char ) {

      end_space_len = 1;

      while ( pos+1 < text.length() && text[pos+1] == m_word_break_char ) {
        end_space_len++;
        pos++;
      }

      if ( pos+1 == text.length() )
        break;

      last_unquoted_space = pos;
    }
  }

  text_start = text.left( last_unquoted_space + 1 );

  // the last part without trailing blanks
  text_compl = text.mid( last_unquoted_space + 1 );
}

//END KateCmdShellCompletion


