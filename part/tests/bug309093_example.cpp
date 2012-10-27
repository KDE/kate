/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2003-2005 Anders Lund <anders@alweb.dk>
 *  Copyright (C) 2001-2010 Christoph Cullmann <cullmann@kde.org>
 *  Copyright (C) 2001 Charles Samuels <charles@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "katecmds.h"

#include "katedocument.h"
#include "kateview.h"
#include "kateconfig.h"
#include "kateautoindent.h"
#include "katetextline.h"
#include "katesyntaxmanager.h"
#include "kateglobal.h"
#include "kateviglobal.h"
#include "katevinormalmode.h"
#include "katerenderer.h"
#include "katecmd.h"

#include <kdebug.h>
#include <klocale.h>
#include <kurl.h>
#include <kshellcompletion.h>

#include <QDir>
#include <QtCore/QRegExp>

//BEGIN CoreCommands
KateCommands::CoreCommands* KateCommands::CoreCommands::m_instance = 0;

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
    << "print";

  return l;
}

bool KateCommands::AppCommands::help(KTextEditor::View *view, const QString &cmd, QString &msg)
{
  QString realcmd=cmd.trimmed();
if (realcmd=="cleanindent") {
    msg=;
    return true;
  } else   if (realcmd=="comment") {
    msg=;
    return true;
  } else   if (realcmd=="uncomment") {
    msg=;
    return true;
  } else   if (realcmd=="goto") {
    msg=;
    return true;
  } else   if (realcmd=="kill-line") {
    msg=;
    return true;
  } else   if (realcmd=="set-tab-width") {
    msg=;
    return true;
  } else   if (realcmd=="set-replace-tab") {
    msg=;
    return true;
  } else   if (realcmd=="set-show-tabs") {
    msg=;
    return true;
  } else   if (realcmd=="set-remove-trailing-space") {
    msg=;
    return true;
  } else   if (realcmd=="set-indent-width") {
    msg=;
    return true;
  } else   if (realcmd=="set-indent-mode") {
    msg=;
    return true;
  } else   if (realcmd=="set-auto-indent") {
    msg=;
    return true;
  } else   if (realcmd=="set-line-numbers") {
    msg=;
    return true;
  } else   if (realcmd=="set-folding-markers") {
    msg=;
    return true;
  } else   if (realcmd=="set-icon-border") {
    msg=;
    return true;
  } else   if (realcmd=="set-wrap-cursor") {
    msg=;
    return true;
  } else   if (realcmd=="set-word-wrap") {
    msg=;
    return true;
  } else   if (realcmd=="set-word-wrap-column") {
    msg=;
    return true;
  } else   if (realcmd=="set-replace-tabs-save") {
    msg=;
    return true;
  } else   if (realcmd=="set-remove-trailing-space-save") {
    msg=;
    return true;
  } else   if (realcmd=="set-highlight") {
    msg=;
    return true;
  } else   if (realcmd=="set-mode") {
    msg=;
    return true;
  } else   if (realcmd=="set-show-indent") {
    msg=;
    return true;
  } else   if (realcmd=="print") {
    msg=;
    return true;
  } else
  
  
  return false;
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
  KateView *v = static_cast<KateView*>(view);

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
        v->doc()->indent( KTextEditor::Range(line, 0, line, 0), 1 );
      }
      v->doc()->editEnd();
    } else {
      v->indent();
    }
    return true;
  }
  else if ( cmd == "unindent" )
  {
    if ( range.isValid() ) {
      v->doc()->editStart();
      for ( int line = range.start().line(); line <= range.end().line(); line++ ) {
        v->doc()->indent( KTextEditor::Range(line, 0, line, 0), -1 );
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
        v->doc()->indent( KTextEditor::Range(line, 0, line, 0), 0 );
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
  else if ( cmd == "print" )
  {
    v->doc()->printDialog();
    return true;
  }

  // ALL commands that take a string argument
  else if ( cmd == "set-indent-mode" ||
            cmd == "set-highlight" ||
            cmd == "set-mode" )
  {
    // need at least one item, otherwise args.first() crashes
    if ( ! args.count() )
      KCC_ERR( i18n("Missing argument. Usage: %1 <value>",  cmd ) );

    if ( cmd == "set-indent-mode" )
    {
      v->doc()->config()->setIndentationMode( args.first() );
      return true;
    }
    else if ( cmd == "set-highlight" )
    {
      if ( v->doc()->setHighlightingMode( args.first()) )
      {
        static_cast<KateDocument*>(v->doc())->setDontChangeHlOnSave ();
        return true;
      }

      KCC_ERR( i18n("No such highlighting '%1'",  args.first() ) );
    }
    else if ( cmd == "set-mode" )
    {
      if ( v->doc()->setMode( args.first()) )
        return true;

      KCC_ERR( i18n("No such mode '%1'",  args.first() ) );
    }
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
    KateDocumentConfig * const config = v->doc()->config();
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
        config->setReplaceTabsDyn( enable );
      else if ( cmd == "set-remove-trailing-space" )
        config->setRemoveTrailingDyn( enable );
      else if ( cmd == "set-show-tabs" )
        config->setShowTabs( enable );
      else if ( cmd == "set-show-trailing-spaces" )
        config->setShowSpaces( enable );
      else if ( cmd == "set-word-wrap" )
        v->doc()->setWordWrap( enable );
      else if ( cmd == "set-remove-trailing-space-save" )
        config->setRemoveSpaces( enable );
      else if ( cmd == "set-wrap-cursor" )
        config->setWrapCursor( enable );

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
  Q_UNUSED(view)

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

// BEGIN ViCommands
KateCommands::ViCommands* KateCommands::ViCommands::m_instance = 0;

const QStringList &KateCommands::ViCommands::cmds()
{
  static QStringList l;

  if (l.isEmpty())
  l << "nnoremap" << "nn" << "d" << "delete" << "j" << "c" << "change" << "<" << ">" << "y" << "yank" <<
       "ma" << "mark" << "k";

  return l;
}

bool KateCommands::ViCommands::exec(KTextEditor::View *view,
                            const QString &_cmd,
                            QString &msg)
{
  return exec( view, _cmd, msg, KTextEditor::Range::invalid() );
}

bool KateCommands::ViCommands::exec(KTextEditor::View *view,
                            const QString &_cmd,
                            QString &msg,
                            const KTextEditor::Range& range)
{
  Q_UNUSED(range)
  // cast it hardcore, we know that it is really a kateview :)
  KateView *v = static_cast<KateView*>(view);

  if ( !v ) {
    msg = i18n("Could not access view");
    return false;
  }

  //create a list of args
  QStringList args(_cmd.split( QRegExp("\\s+"), QString::SkipEmptyParts)) ;
  QString cmd ( args.takeFirst() );

  // ALL commands that takes no arguments.
  if ( cmd == "nnoremap" || cmd == "nn" )
  {
    if ( args.count() == 1 ) {
      msg = KateGlobal::self()->viInputModeGlobal()->getMapping( NormalMode, args.at( 0 ), true );
      if ( msg.isEmpty() ) {
        msg = i18n( "No mapping found for \"%1\"", args.at(0) );
        return false;
      } else {
        msg = i18n( "\"%1\" is mapped to \"%2\"", args.at( 0 ), msg );
      }
    } else if ( args.count() == 2 ) {
      KateGlobal::self()->viInputModeGlobal()->addMapping( NormalMode, args.at( 0 ), args.at( 1 ) );
    } else {
      msg = i18n("Missing argument(s). Usage: %1 <from> [<to>]",  cmd );
      return false;
    }

    return true;
  }

  KateViNormalMode* nm = v->getViInputModeManager()->getViNormalMode();

  if (cmd == "d" || cmd == "delete" || cmd == "j" ||
      cmd == "c" || cmd == "change" ||  cmd == "<" || cmd == ">" ||
      cmd == "y" || cmd == "yank") {

    KTextEditor::Cursor start_cursor_position = v->cursorPosition();

    int count = 1;
    if (range.isValid()){
        count = qAbs(range.end().line() - range.start().line())+1;
        if (cmd == "j") count-=1;
        v->setCursorPosition(KTextEditor::Cursor(qMin(range.start().line(),
                                                      range.end().line()),0));
    }

    QRegExp number("^(\\d+)$");
    for (int i = 0; i < args.count(); i++) {
        if (number.indexIn(args.at(i)) != -1)
            count += number.cap().toInt() - 1;

        QChar r = args.at(i).at(0);
        if (args.at(i).size() == 1 && ( (r >= 'a' && r <= 'z') || r == '_' || r == '+' || r == '*' ))
                nm->setRegister(r);
    }

    nm->setCount(count);

    if (cmd == "d" || cmd == "delete" )
        nm->commandDeleteLine();
    if (cmd == "j")
        nm->commandJoinLines();
    if (cmd == "c" || cmd == "change" )
        nm->commandChangeLine();
    if (cmd == "<")
        nm->commandUnindentLine();
    if (cmd == ">")
        nm->commandIndentLine();
    if (cmd == "y" || cmd == "yank" ){
        nm->commandYankLine();
        v->setCursorPosition(start_cursor_position);
    }

    // TODO - should we resetParser, here? We'd have to make it public, if so.
    // Or maybe synthesise a KateViCommand to execute instead ... ?
    nm->setCount(0);

    return true;
  }

  if (cmd == "mark" || cmd == "ma" || cmd == "k" ) {
      if (args.count() == 0){
          if (cmd == "mark"){
              // TODO: show up mark list;
          } else {
              msg = i18n("Wrong arguments");
              return false;
          }
      } else if (args.count() == 1) {

        QChar r = args.at(0).at(0);
        int line;
        if ( (r >= 'a' && r <= 'z') || r == '_' || r == '+' || r == '*' ) {
            if (range.isValid())
                line = qMax(range.end().line(),range.start().line());
            else
                line = v->cursorPosition().line();

            v->getViInputModeManager()->addMark(v->doc(),r,KTextEditor::Cursor(line, 0));
        }
      } else {
          msg = i18n("Wrong arguments");
          return false;
      }
      return true;
  }

  // should not happen :)
  msg = i18n("Unknown command '%1'", cmd);
  return false;
}

bool KateCommands::ViCommands::supportsRange(const QString &range)
{
  static QStringList l;

  if (l.isEmpty())
  l << "d" << "delete" << "j" << "c" << "change" << "<" <<
       ">" << "y" << "yank" << "ma" << "mark" << "k";

  return l.contains(range.split(" ").at(0));
}

KCompletion *KateCommands::ViCommands::completionObject( KTextEditor::View *view, const QString &cmd )
{
  Q_UNUSED(view)

  KateView *v = static_cast<KateView*>(view);

  if ( v && ( cmd == "nn" || cmd == "nnoremap" ) )
  {
    QStringList l = KateGlobal::self()->viInputModeGlobal()->getMappings( NormalMode );

    KateCmdShellCompletion *co = new KateCmdShellCompletion();
    co->setItems( l );
    co->setIgnoreCase( false );
    return co;
  }
  return 0L;
}
//END ViCommands

// BEGIN AppCommands
KateCommands::AppCommands* KateCommands::AppCommands::m_instance = 0;

KateCommands::AppCommands::AppCommands()
    : KTextEditor::Command()
{
    re_write.setPattern("w"); // temporarily add :w
    //re_write.setPattern("w(a)?");
    //re_quit.setPattern("(w)?q?(a)?");
    //re_exit.setPattern("x(a)?");
    //re_changeBuffer.setPattern("b(n|p)");
    //re_edit.setPattern("e(dit)?");
    //re_new.setPattern("(v)?new");
}

const QStringList& KateCommands::AppCommands::cmds()
{
    static QStringList l;

    if (l.empty()) {
        //l << "q" << "qa" << "w" << "wq" << "wa" << "wqa" << "x" << "xa"
          //<< "bn" << "bp" << "new" << "vnew" << "e" << "edit" << "enew";
        l << "w";
    }

    return l;
}

// commands that don't need to live in the hosting application should be
// implemented here. things such as quitting and splitting the view can not be
// done from the editor part and needs to be implemented in the hosting
// application.
bool KateCommands::AppCommands::exec(KTextEditor::View *view,
                                     const QString &cmd, QString &msg )
{
    QStringList args(cmd.split( QRegExp("\\s+"), QString::SkipEmptyParts)) ;
    QString command( args.takeFirst() );
    QString file = args.join(QString(' '));

    if (re_write.exactMatch(command)) { // w, wa
        /*        if (!re_write.cap(1).isEmpty()) { // [a]ll
            view->document()->saveAll();
            msg = i18n("All documents written to disk");
        } else { // w*/
        // Save file
        if (file.isEmpty()) {
            view->document()->documentSave();
        } else {
            KUrl base = view->document()->url();
            KUrl url( base.isValid() ? base : KUrl( QDir::homePath() ), file );
            view->document()->saveAs( url );
        }
        msg = i18n("Document written to disk");
    }

    return true;
}

bool KateCommands::AppCommands::help(KTextEditor::View *view, const QString &cmd, QString &msg)
{
    Q_UNUSED(view);

    if (re_write.exactMatch(cmd)) {
        msg = i18n("<p><b>w/wa &mdash; write document(s) to disk</b></p>"
              "<p>Usage: <tt><b>w[a]</b></tt></p>"
              "<p>Writes the current document(s) to disk. "
              "It can be called in two ways:<br />"
              " <tt>w</tt> &mdash; writes the current document to disk<br />"
              " <tt>wa</tt> &mdash; writes all document to disk.</p>"
              "<p>If no file name is associated with the document, "
              "a file dialog will be shown.</p>");
        return true;
    }
    return false;
}
//END AppCommands

//BEGIN SedReplace
KateCommands::SedReplace* KateCommands::SedReplace::m_instance = 0;

static void replace(QString &s, const QString &needle, const QString &with)
{
  int pos = 0;
  while (1)
  {
    pos = s.indexOf(needle, pos);
    if (pos == -1) break;
    s.replace(pos, needle.length(), with);
    pos += with.length();
  }

}

static int backslashString(const QString &haystack, const QString &needle, int index)
{
  int len = haystack.length();
  int searchlen = needle.length();
  bool evenCount = true;
  while (index < len)
  {
    if (haystack[index] == '\\')
    {
      evenCount = !evenCount;
    }
    else
    { // isn't a slash
      if (!evenCount)
      {
        if (haystack.mid(index, searchlen) == needle)
          return index - 1;
      }
      evenCount = true;
    }
    ++index;

  }

  return -1;
}

// exchange "\t" for the actual tab character, for example
static void exchangeAbbrevs(QString &str)
{
  // the format is (findreplace)*[nullzero]
  const char *magic = "a\x07t\tn\n";

  while (*magic)
  {
    int index = 0;
    char replace = magic[1];
    while ((index = backslashString(str, QString (QChar::fromAscii(*magic)), index)) != -1)
    {
      str.replace(index, 2, QChar(replace));
      ++index;
    }
    ++magic;
    ++magic;
  }
}

int KateCommands::SedReplace::sedMagic( KateDocument *doc, int &line,
                                        const QString &find, const QString &repOld, const QString &delim,
                                        bool noCase, bool repeat,
                                        int startcol, int endcol )
{
  Kate::TextLine ln = doc->kateTextLine( line );
  if ( !ln || !ln->length() ) return 0;

  // HANDLING "\n"s in PATTERN
  // * Create a list of patterns, splitting PATTERN on (unescaped) "\n"
  // * insert $s and ^s to match line ends/beginnings
  // * When matching patterns after the first one, replace \N with the captured
  //   text.
  // * If all patterns in the list match sequential lines, there is a match, so
  // * remove line/start to line + patterns.count()-1/patterns.last.length
  // * handle captures by putting them in one list.
  // * the existing insertion is fine, including the line calculation.

  QStringList patterns(find.split( QRegExp("(^\\\\n|(?![^\\\\])\\\\n)"), QString::KeepEmptyParts));
  if ( patterns.count() > 1 )
  {
    for ( int i = 0; i < patterns.count(); ++i )
    {
      if ( i < patterns.count() - 1 )
        patterns[i].append("$");
      if ( i )
        patterns[i].prepend("^");

       kDebug(13025) << "patterns[" << i << "] =" << patterns[i];
    }
  }

  QRegExp matcher(patterns[0], noCase ?Qt::CaseSensitive:Qt::CaseInsensitive);

  int matches = 0;

  while ( (startcol = matcher.indexIn(ln->string(), startcol)) >= 0 )
  {
    const int len = matcher.matchedLength();

    if ( endcol >= 0 && startcol + len > endcol )
      break;

    ++matches;


    QString rep = repOld;

    // now set the backreferences in the replacement
    const QStringList backrefs = matcher.capturedTexts();
    int refnum = 1;

    QStringList::ConstIterator i = backrefs.begin();
    ++i;

    for (; i != backrefs.end(); ++i)
    {
      // I need to match "\\" or "", but not "\"
      QString number = QString::number(refnum);

      int index = 0;
      while (index != -1)
      {
        index = backslashString(rep, number, index);
        if (index >= 0)
        {
          rep.replace(index, 2, *i);
          index += (*i).length();
        }
      }

      ++refnum;
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

      if ( doc->lineLength( line ) > 0 && ( endcol < 0 || endcol >= startcol + len ) )
      {
      // if ( endcol >= startcol + len )
          endcol -= (startcol + len);
          uint sc = rep.length() - rep.lastIndexOf('\n') - 1;
        matches += sedMagic( doc, line, find, repOld, delim, noCase, repeat, sc, endcol );
      }
    }

    if (!repeat) break;
    startcol += rep.length();

    // sanity check -- avoid infinite loops eg with %s,.*,,g ;)
    int ll = ln->length();
    if ( !ll || startcol > (ll - 1) )
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
  kDebug(13025) << "SedReplace::execCmd( " << cmd << " )";
  if (r.isValid()) {
    kDebug(13025) << "Range: " << r;
  }

  QRegExp delim("^s\\s*([^\\w\\s])");
  if ( delim.indexIn( cmd ) < 0 ) return false;

  bool noCase = cmd[cmd.length() - 1] == 'i' || cmd[cmd.length() - 2] == 'i';
  bool repeat = cmd[cmd.length() - 1] == 'g' || cmd[cmd.length() - 2] == 'g';

  QString d = delim.cap(1);
  kDebug(13025) << "SedReplace: delimiter is '" << d << "'";

  QRegExp splitter( QString("^s\\s*") + d + "((?:[^\\\\\\" + d + "]|\\\\.)*)\\"
      + d + "((?:[^\\\\\\" + d + "]|\\\\.)*)(\\" + d + "[ig]{0,2})?$" );
  if (splitter.indexIn(cmd) < 0) return false;

  QString find = splitter.cap(1);
  kDebug(13025) << "SedReplace: find =" << find;

  QString replace = splitter.cap(2);
  exchangeAbbrevs(replace);
  kDebug(13025) << "SedReplace: replace =" << replace;

  if ( find.contains("\\n") )
  {
    // FIXME: make replacing newlines work
    msg = i18n("Sorry, but Kate is not able to replace newlines, yet");
    return false;
  }

  KateDocument *doc = static_cast<KateView*>(view)->doc();
  if ( !doc ) return false;

  static_cast<KateView*>(view)->setSearchPattern(find);
  doc->editStart();

  int replacementsDone = 0;
  int linesTouched = 0;
  int linesAdded = 0;

  if (r.isValid()) { // given range
    for (int line = r.start().line(); line <= r.end().line() + linesAdded; ++line) {
      int temp = replacementsDone;
      int r = sedMagic( doc, line, find, replace, d, !noCase, repeat );
      replacementsDone += r;

      // if we replaced the text with n newlines, we have n new lines to look at
      if (replace.contains('\n')) {
        linesAdded += r * replace.count('\n');
      }

      if (replacementsDone > temp) {
        ++linesTouched;
      }
    }
  } else { // current line
    int line = view->cursorPosition().line();
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
KateCommands::Character* KateCommands::Character::m_instance = 0;

bool KateCommands::Character::help (class KTextEditor::View *, const QString &cmd, QString &msg)
{     
      if (cmd.trimmed()=="char") {
        msg = i18n("<p> char <b>identifier</b> </p>"
        "<p>This command allows you to insert literal characters by their numerical identifier, in decimal, octal or hexadecimal form.</p>"
          "<p>Examples:<ul>"
          "<li>char <b>234</b></li>"
          "<li>char <b>0x1234</b></li>"
          "</ul></p>");
        return true;
    }
    return false;
}

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
KateCommands::Date* KateCommands::Date::m_instance = 0;

bool KateCommands::Date::help (class KTextEditor::View *, const QString &cmd, QString &msg)
{     
      if (cmd.trimmed()=="date") {
        msg = i18n("<p>date or date <b>format</b></p>"
        "<p>Inserts a date/time string as defined by the specified format, or the format yyyy-MM-dd hh:mm:ss if none is specified."
        "<p>Possible format specifiers are:"
        "<table>"
            "<tr><td>dd</td><td>The day as number with a leading zero (01-31).</td></tr>"
            "<tr><td>ddd</td><td>The abbreviated localized day name (e.g. 'Mon'..'Sun').</td></tr>"
            "<tr><td>dddd</td><td>The long localized day name (e.g. 'Monday'..'Sunday').</td></tr>"
            "<tr><td>M</td><td>The month as number without a leading zero (1-12).</td></tr>"
            "<tr><td>MM</td><td>The month as number with a leading zero (01-12).</td></tr>"
            "<tr><td>MMM</td><td>The abbreviated localized month name (e.g. 'Jan'..'Dec').</td></tr>"
            "<tr><td>yy</td><td>The year as two digit number (00-99).</td></tr>"
            "<tr><td>yyyy</td><td>The year as four digit number (1752-8000).</td></tr>"
            "<tr><td>h</td><td>The hour without a leading zero (0..23 or 1..12 if AM/PM display).</td></tr>"
            "<tr><td>hh</td><td>The hour with a leading zero (00..23 or 01..12 if AM/PM display).</td></tr>"
            "<tr><td>m</td><td>The minute without a leading zero (0..59).</td></tr>"
            "<tr><td>mm</td><td>The minute with a leading zero (00..59).</td></tr>"
            "<tr><td>s</td><td>The second without a leading zero (0..59).</td></tr>"
            "<tr><td>ss</td><td>The second with a leading zero (00..59).</td></tr>"
            "<tr><td>z</td><td>The milliseconds without leading zeroes (0..999).</td></tr>"
            "<tr><td>zzz</td><td>The milliseconds with leading zeroes (000..999).</td></tr>"
            "<tr><td>AP</td><td>Use AM/PM display. AP will be replaced by either \"AM\" or \"PM\".</td></tr>"
            "<tr><td>ap</td><td>Use am/pm display. ap will be replaced by either \"am\" or \"pm\".</td></tr>"
        "</table></p>");
        return true;
    }
    return false;
}

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
