/* This file is part of the KDE libraries
   Copyright (C) 2003 - 2005 Anders Lund <anders@alweb.dk>
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

#include "katecmds.h"

#include "katedocument.h"
#include "kateview.h"
#include "kateconfig.h"
#include "kateautoindent.h"
#include "katetextline.h"
#include "katesyntaxmanager.h"
#include "kateglobal.h"
#include "katerenderer.h"
#include "katecmd.h"

#include <kdebug.h>
#include <klocale.h>
#include <kurl.h>
#include <kshellcompletion.h>

#include <QtCore/QRegExp>

//BEGIN CoreCommands
// syncs a config flag in the document with a boolean value
static void setDocFlag( KateDocumentConfig::ConfigFlags flag, bool enable,
                  KateDocument *doc )
{
  doc->config()->setConfigFlags( flag, enable );
}

// this returns wheather the string s could be converted to
// a bool value, one of on|off|1|0|true|false. the argument val is
// set to the extracted value in case of success
static bool getBoolArg( const QString &t, bool *val  )
{
  bool res( false );
  QString s = t.toLower();
  res = (s == "on" || s == "1" || s == "true");
  if ( res )
  {
    *val = true;
    return true;
  }
  res = (s == "off" || s == "0" || s == "false");
  if ( res )
  {
    *val = false;
    return true;
  }
  return false;
}

const QStringList &KateCommands::CoreCommands::cmds()
{
  static QStringList l;

  if (l.isEmpty())
  l << "indent" << "unindent" << "cleanindent"
    << "comment" << "uncomment" << "goto" << "kill-line"
    << "set-tab-width" << "set-replace-tabs" << "set-show-tabs"
    << "set-remove-trailing-space"
    << "set-indent-width"
    << "set-indent-mode" << "set-auto-indent"
    << "set-line-numbers" << "set-folding-markers" << "set-icon-border"
    << "set-wrap-cursor"
    << "set-word-wrap" << "set-word-wrap-column"
    << "set-replace-tabs-save" << "set-remove-trailing-space-save"
    << "set-highlight" << "set-mode" << "set-show-indent"
    << "w" << "print" << "hardcopy";

  return l;
}

bool KateCommands::CoreCommands::exec(KTextEditor::View *view,
                            const QString &_cmd,
                            QString &errorMsg)
{
  return exec( view, _cmd, errorMsg, KTextEditor::Range::invalid() );
}

bool KateCommands::CoreCommands::exec(KTextEditor::View *view,
                            const QString &_cmd,
                            QString &errorMsg,
                            const KTextEditor::Range& range)
{
#define KCC_ERR(s) { errorMsg=s; return false; }
  // cast it hardcore, we know that it is really a kateview :)
  KateView *v = (KateView*) view;

  if ( ! v )
    KCC_ERR( i18n("Could not access view") );

  //create a list of args
  QStringList args(_cmd.split( QRegExp("\\s+"), QString::SkipEmptyParts)) ;
  QString cmd ( args.takeFirst() );

  // ALL commands that takes no arguments.
  if ( cmd == "indent" )
  {
    if ( range.isValid() ) {
      v->doc()->editStart();
      for ( int line = range.start().line(); line <= range.end().line(); line++ ) {
        v->doc()->indent( v, line, 1 );
      }
      v->doc()->editEnd();
    } else {
      v->indent();
    }
    return true;
  }
#if 0
  else if ( cmd == "run-myself" )
  {
#ifndef Q_WS_WIN //todo
    return KateGlobal::self()->jscript()->execute(v, v->doc()->text(), errorMsg);
#else
    return 0;
#endif
  }
#endif
  else if ( cmd == "unindent" )
  {
    if ( range.isValid() ) {
      v->doc()->editStart();
      for ( int line = range.start().line(); line <= range.end().line(); line++ ) {
        v->doc()->indent( v, line, -1 );
      }
      v->doc()->editEnd();
    } else {
      v->unIndent();
    }
    return true;
  }
  else if ( cmd == "cleanindent" )
  {
    if ( range.isValid() ) {
      v->doc()->editStart();
      for ( int line = range.start().line(); line <= range.end().line(); line++ ) {
        v->doc()->indent( v, line, 0 );
      }
      v->doc()->editEnd();
    } else {
      v->cleanIndent();
    }
    return true;
  }
  else if ( cmd == "comment" )
  {
    if ( range.isValid() ) {
      v->doc()->editStart();
      for ( int line = range.start().line(); line <= range.end().line(); line++ ) {
        v->doc()->comment( v, line, 0, 1 );
      }
      v->doc()->editEnd();
    } else {
      v->comment();
    }
    return true;
  }
  else if ( cmd == "uncomment" )
  {
    if ( range.isValid() ) {
      v->doc()->editStart();
      for ( int line = range.start().line(); line <= range.end().line(); line++ ) {
        v->doc()->comment( v, line, 0, -1 );
      }
      v->doc()->editEnd();
    } else {
      v->uncomment();
    }
    return true;
  }
  else if ( cmd == "kill-line" )
  {
    if ( range.isValid() ) {
      v->doc()->editStart();
      for ( int line = range.start().line(); line <= range.end().line(); line++ ) {
        v->doc()->removeLine( range.start().line() );
      }
      v->doc()->editEnd();
    } else {
      v->killLine();
    }
    return true;
  }
  else if ( cmd == "w" )
  {
    v->doc()->documentSave();
    return true;
  }
  else if ( cmd == "print" || cmd == "hardcopy" )
  {
    v->doc()->printDialog();
    return true;
  }
  else if ( cmd == "set-indent-mode" )
  {
    v->doc()->config()->setIndentationMode( args.first() );
    return true;
  }
  else if ( cmd == "set-highlight" )
  {
    if ( v->doc()->setHighlightingMode( args.first()) )
      return true;

    KCC_ERR( i18n("No such highlighting '%1'",  args.first() ) );
  }
  else if ( cmd == "set-mode" )
  {
    if ( v->doc()->setMode( args.first()) )
      return true;

    KCC_ERR( i18n("No such mode '%1'",  args.first() ) );
  }

  // ALL commands that takes exactly one integer argument.
  else if ( cmd == "set-tab-width" ||
            cmd == "set-indent-width" ||
            cmd == "set-word-wrap-column" ||
            cmd == "goto" )
  {
    // find a integer value > 0
    if ( ! args.count() )
      KCC_ERR( i18n("Missing argument. Usage: %1 <value>",  cmd ) );
    bool ok;
    int val ( args.first().toInt( &ok, 10 ) ); // use base 10 even if the string starts with '0'
    if ( !ok )
      KCC_ERR( i18n("Failed to convert argument '%1' to integer.",
                  args.first() ) );

    if ( cmd == "set-tab-width" )
    {
      if ( val < 1 )
        KCC_ERR( i18n("Width must be at least 1.") );
      v->doc()->config()->setTabWidth( val );
    }
    else if ( cmd == "set-indent-width" )
    {
      if ( val < 1 )
        KCC_ERR( i18n("Width must be at least 1.") );
      v->doc()->config()->setIndentationWidth( val );
    }
    else if ( cmd == "set-word-wrap-column" )
    {
      if ( val < 2 )
        KCC_ERR( i18n("Column must be at least 1.") );
      v->doc()->setWordWrapAt( val );
    }
    else if ( cmd == "goto" )
    {
      if ( args.first().at(0) == '-' || args.first().at(0) == '+' ) {
        // if the number starts with a minus or plus sign, add/subract the number
        val = v->cursorPosition().line() + val;
      } else {
        val--; // convert given line number to the internal representation of line numbers
      }

      // constrain cursor to the range [0, number of lines]
      if ( val < 0 ) {
        val = 0;
      } else if ( val > v->doc()->lines()-1 ) {
        val = v->doc()->lines()-1;
      }

      v->setCursorPosition( KTextEditor::Cursor( val, 0 ) );
      return true;
    }
    return true;
  }

  // ALL commands that takes 1 boolean argument.
  else if ( cmd == "set-icon-border" ||
            cmd == "set-folding-markers" ||
            cmd == "set-line-numbers" ||
            cmd == "set-replace-tabs" ||
            cmd == "set-remove-trailing-space" ||
            cmd == "set-show-tabs" ||
            cmd == "set-word-wrap" ||
            cmd == "set-wrap-cursor" ||
            cmd == "set-replace-tabs-save" ||
            cmd == "set-remove-trailing-space-save" ||
            cmd == "set-show-indent" )
  {
    if ( ! args.count() )
      KCC_ERR( i18n("Usage: %1 on|off|1|0|true|false",  cmd ) );
    bool enable = false;
    if ( getBoolArg( args.first(), &enable ) )
    {
      if ( cmd == "set-icon-border" )
        v->setIconBorder( enable );
      else if (cmd == "set-folding-markers")
        v->setFoldingMarkersOn( enable );
      else if ( cmd == "set-line-numbers" )
        v->setLineNumbersOn( enable );
      else if ( cmd == "set-show-indent" )
        v->renderer()->setShowIndentLines( enable );
      else if ( cmd == "set-replace-tabs" )
        setDocFlag( KateDocumentConfig::cfReplaceTabsDyn, enable, v->doc() );
      else if ( cmd == "set-remove-trailing-space" )
        setDocFlag( KateDocumentConfig::cfRemoveTrailingDyn, enable, v->doc() );
      else if ( cmd == "set-show-tabs" )
        setDocFlag( KateDocumentConfig::cfShowTabs, enable, v->doc() );
      else if ( cmd == "set-show-trailing-spaces" )
        setDocFlag( KateDocumentConfig::cfShowSpaces, enable, v->doc() );
      else if ( cmd == "set-word-wrap" )
        v->doc()->setWordWrap( enable );
      else if ( cmd == "set-remove-trailing-space-save" )
        setDocFlag( KateDocumentConfig::cfRemoveSpaces, enable, v->doc() );
      else if ( cmd == "set-wrap-cursor" )
        setDocFlag( KateDocumentConfig::cfWrapCursor, enable, v->doc() );

      return true;
    }
    else
      KCC_ERR( i18n("Bad argument '%1'. Usage: %2 on|off|1|0|true|false",
                 args.first() ,  cmd ) );
  }

  // unlikely..
  KCC_ERR( i18n("Unknown command '%1'", cmd) );
}

bool KateCommands::CoreCommands::supportsRange(const QString &range)
{
  static QStringList l;

  if (l.isEmpty())
  l << "indent" << "unindent" << "cleanindent"
    << "comment" << "uncomment" << "kill-line";

  return l.contains(range);
}

KCompletion *KateCommands::CoreCommands::completionObject( KTextEditor::View *view, const QString &cmd )
{
  Q_UNUSED(view);

  if ( cmd == "set-highlight" )
  {
    QStringList l;
    for ( int i = 0; i < KateHlManager::self()->highlights(); i++ )
      l << KateHlManager::self()->hlName (i);

    KateCmdShellCompletion *co = new KateCmdShellCompletion();
    co->setItems( l );
    co->setIgnoreCase( true );
    return co;
  }
  return 0L;
}
//END CoreCommands

//BEGIN SedReplace
static void replace(QString &s, const QString &needle, const QString &with)
{
  int pos=0;
  while (1)
  {
    pos=s.indexOf(needle, pos);
    if (pos==-1) break;
    s.replace(pos, needle.length(), with);
    pos+=with.length();
  }

}

static int backslashString(const QString &haystack, const QString &needle, int index)
{
  int len=haystack.length();
  int searchlen=needle.length();
  bool evenCount=true;
  while (index<len)
  {
    if (haystack[index]=='\\')
    {
      evenCount=!evenCount;
    }
    else
    {  // isn't a slash
      if (!evenCount)
      {
        if (haystack.mid(index, searchlen)==needle)
          return index-1;
      }
      evenCount=true;
    }
    index++;

  }

  return -1;
}

// exchange "\t" for the actual tab character, for example
static void exchangeAbbrevs(QString &str)
{
  // the format is (findreplace)*[nullzero]
  const char *magic="a\x07t\tn\n";

  while (*magic)
  {
    int index=0;
    char replace=magic[1];
    while ((index=backslashString(str, QString (QChar::fromAscii(*magic)), index))!=-1)
    {
      str.replace(index, 2, QChar(replace));
      index++;
    }
    magic++;
    magic++;
  }
}

int KateCommands::SedReplace::sedMagic( KateDocument *doc, int &line,
                                        const QString &find, const QString &repOld, const QString &delim,
                                        bool noCase, bool repeat,
                                        uint startcol, int endcol )
{
  KateTextLine::Ptr ln = doc->kateTextLine( line );
  if ( ! ln || ! ln->length() ) return 0;

  // HANDLING "\n"s in PATTERN
  // * Create a list of patterns, splitting PATTERN on (unescaped) "\n"
  // * insert $s and ^s to match line ends/beginnings
  // * When matching patterhs after the first one, replace \N with the captured
  //   text.
  // * If all patterns in the list match sequentiel lines, there is a match, so
  // * remove line/start to line + patterns.count()-1/patterns.last.length
  // * handle capatures by putting them in one list.
  // * the existing insertion is fine, including the line calculation.

  QStringList patterns(find.split( QRegExp("(^\\\\n|(?![^\\\\])\\\\n)"), QString::KeepEmptyParts));
  if ( patterns.count() > 1 )
  {
    for ( int i = 0; i < patterns.count(); i++ )
    {
      if ( i < patterns.count() - 1 )
        patterns[i].append("$");
      if ( i )
        patterns[i].prepend("^");

       kDebug(13025)<<"patterns["<<i<<"] ="<<patterns[i];
    }
  }

  QRegExp matcher(patterns[0], noCase ?Qt::CaseSensitive:Qt::CaseInsensitive);

  uint len;
  int matches = 0;

  while ( ln->searchText( startcol, matcher, &startcol, &len ) )
  {

    if ( endcol >= 0  && startcol + len > (uint)endcol )
      break;

    matches++;


    QString rep=repOld;

    // now set the backreferences in the replacement
    const QStringList backrefs=matcher.capturedTexts();
    int refnum=1;

    QStringList::ConstIterator i = backrefs.begin();
    ++i;

    for (; i!=backrefs.end(); ++i)
    {
      // I need to match "\\" or "", but not "\"
      QString number=QString::number(refnum);

      int index=0;
      while (index!=-1)
      {
        index=backslashString(rep, number, index);
        if (index>=0)
        {
          rep.replace(index, 2, *i);
          index+=(*i).length();
        }
      }

      refnum++;
    }

    replace(rep, "\\\\", "\\");
    replace(rep, "\\" + delim, delim);

    doc->removeText( KTextEditor::Range (line, startcol, line, startcol + len) );
    doc->insertText( KTextEditor::Cursor (line, startcol), rep );

    // TODO if replace contains \n,
    // change the line number and
    // check for text that needs be searched behind the last inserted newline.
    int lns = rep.count(QChar::fromLatin1('\n'));
    if ( lns > 0 )
    {
      line += lns;

      if ( doc->lineLength( line ) > 0 && ( endcol < 0 || (uint)endcol  >= startcol + len ) )
      {
      //  if ( endcol  >= startcol + len )
          endcol -= (startcol + len);
          uint sc = rep.length() - rep.lastIndexOf('\n') - 1;
        matches += sedMagic( doc, line, find, repOld, delim, noCase, repeat, sc, endcol );
      }
    }

    if (!repeat) break;
    startcol+=rep.length();

    // sanity check -- avoid infinite loops eg with %s,.*,,g ;)
    uint ll = ln->length();
    if ( ! ll || startcol > ll )
      break;
  }

  return matches;
}

bool KateCommands::SedReplace::exec (KTextEditor::View *view, const QString &cmd, QString &msg)
{
  return exec(view, cmd, msg, KTextEditor::Range::invalid());
}

bool KateCommands::SedReplace::exec (class KTextEditor::View *view, const QString &cmd,
    QString &msg, const KTextEditor::Range &r)
{
  kDebug(13025)<<"SedReplace::execCmd( "<<cmd<<" )";
  if (r.isValid())
    kDebug(13025)<<"Range: " << r;

  QRegExp delim("^s\\s*([^\\w\\s])");
  if ( delim.indexIn( cmd ) < 0 ) return false;

  bool noCase=cmd[cmd.length()-1]=='i' || cmd[cmd.length()-2]=='i';
  bool repeat=cmd[cmd.length()-1]=='g' || cmd[cmd.length()-2]=='g';

  QString d = delim.cap(1);
  kDebug(13025)<<"SedReplace: delimiter is '"<<d<<"'";

  QRegExp splitter( QString("^s\\s*")  + d + "((?:[^\\\\\\" + d + "]|\\\\.)*)\\"
      + d +"((?:[^\\\\\\" + d + "]|\\\\.)*)(\\" + d + "[ig]{0,2})?$" );
  if (splitter.indexIn(cmd)<0) return false;

  QString find=splitter.cap(1);
  kDebug(13025)<< "SedReplace: find=" << find;

  QString replace=splitter.cap(2);
  exchangeAbbrevs(replace);
  kDebug(13025)<< "SedReplace: replace=" << replace;

  if ( find.contains("\\n") )
  {
    // FIXME: make replacing newlines work
    msg = i18n("Sorry, but Kate is not able to replace newlines, yet");
    return false;
  }

  KateDocument *doc = ((KateView*)view)->doc();
  if ( ! doc ) return false;

  doc->editStart();

  int replacementsDone = 0;
  int linesTouched = 0;

  if (r.isValid()) { // given range
    for (int line = r.start().line(); line <= r.end().line(); line++) {
      int temp = replacementsDone;
      replacementsDone += sedMagic( doc, line, find, replace, d, !noCase, repeat );
      if (replacementsDone > temp) {
        linesTouched++;
      }
    }
  } else { // current line
    int line= view->cursorPosition().line();
    replacementsDone += sedMagic(doc, line, find, replace, d, !noCase, repeat);
    if (replacementsDone > 0) {
      linesTouched = 1;
    }
  }

  msg = i18ncp("%2 is the translation of the next message",
               "1 replacement done on %2", "%1 replacements done on %2", replacementsDone,
               i18ncp("substituted into the previous message",
                      "1 line", "%1 lines", linesTouched));

  doc->editEnd();

  return true;
}

//END SedReplace

//BEGIN Character
bool KateCommands::Character::exec (KTextEditor::View *view, const QString &_cmd, QString &)
{
  QString cmd = _cmd;

  // hex, octal, base 9+1
  QRegExp num("^char *(0?x[0-9A-Fa-f]{1,4}|0[0-7]{1,6}|[0-9]{1,5})$");
  if (num.indexIn(cmd)==-1) return false;

  cmd=num.cap(1);

  // identify the base

  unsigned short int number=0;
  int base=10;
  if (cmd[0]=='x' || cmd.startsWith(QLatin1String("0x")))
  {
    cmd.remove(QRegExp("^0?x"));
    base=16;
  }
  else if (cmd[0]=='0')
    base=8;
  bool ok;
  number=cmd.toUShort(&ok, base);
  if (!ok || number==0) return false;
  if (number<=255)
  {
    char buf[2];
    buf[0]=(char)number;
    buf[1]=0;

    view->document()->insertText(view->cursorPosition(), QString(buf));
  }
  else
  { // do the unicode thing
    QChar c(number);

    view->document()->insertText(view->cursorPosition(), QString(&c, 1));
  }

  return true;
}

//END Character

//BEGIN Date
bool KateCommands::Date::exec (KTextEditor::View *view, const QString &cmd, QString &)
{
  if (!cmd.startsWith(QLatin1String("date")))
    return false;

  if (QDateTime::currentDateTime().toString(cmd.mid(5, cmd.length()-5)).length() > 0)
    view->document()->insertText(view->cursorPosition(), QDateTime::currentDateTime().toString(cmd.mid(5, cmd.length()-5)));
  else
    view->document()->insertText(view->cursorPosition(), QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));

  return true;
}

//END Date

// kate: space-indent on; indent-width 2; replace-tabs on;
