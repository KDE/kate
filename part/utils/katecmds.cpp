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
#include "kateviemulatedcommandbar.h"
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
  l << "indent" << "unindent" << "cleanindent" << "fold" << "tfold" << "unfold"
    << "comment" << "uncomment" << "goto" << "kill-line"
    << "set-tab-width" << "set-replace-tabs" << "set-show-tabs"
    << "set-indent-width"
    << "set-indent-mode" << "set-auto-indent"
    << "set-line-numbers" << "set-folding-markers" << "set-icon-border"
    << "set-indent-pasted-text" << "set-word-wrap" << "set-word-wrap-column"
    << "set-replace-tabs-save" << "set-remove-trailing-spaces"
    << "set-highlight" << "set-mode" << "set-show-indent"
    << "print";

  return l;
}

bool KateCommands::CoreCommands::help(KTextEditor::View *, const QString &cmd, QString &msg)
{
  QString realcmd=cmd.trimmed();
  if (realcmd=="indent") {
    msg=i18n("<p>indent</p>"
      "<p>Indents the selected lines or the current line</p>");
    return true;
  } else   if (realcmd=="unindent") {
    msg=i18n("<p>unindent</p>"
      "<p>Unindents the selected lines or current line.</p>");
    return true;
  } else   if (realcmd=="cleanindent") {
    msg=i18n("<p>cleanindent</p>"
      "<p>Cleans up the indentation of the selected lines or current line according to the indentation settings in the document. </p>");
    return true;
  } else   if (realcmd=="comment") {
    msg=i18n("<p>comment</p>"
      "<p>Inserts comment markers to make the selection or selected lines or current line a comment according to the text format as defined by the syntax highlight definition for the document.</p>");
    return true;
  } else   if (realcmd=="uncomment") {
    msg=i18n("<p>uncomment</p>"
      "<p>Removes comment markers from the selection or selected lines or current line according to the text format as defined by the syntax highlight definition for the document.</p>");
    return true;
  } else   if (realcmd=="goto") {
    msg=i18n("<p>goto <b>line number</b></p>"
      "<p>This command navigates to the specified line number.</p>");
    return true;
  } else   if (realcmd=="set-indent-pasted-text") {
    msg=i18n("<p>set-indent-pasted-text <b>enable</b></p>"
      "<p>If enabled, indentation of text pasted from the clipboard is adjusted using the current indenter.</p>"
      "<p>Possible true values: 1 on true<br/>"
      "possible false values: 0 off false</p>");
    return true;
  } else   if (realcmd=="kill-line") {
    msg=i18n("Deletes the current line.");
    return true;
  } else   if (realcmd=="set-tab-width") {
    msg=i18n("<p>set-tab-width <b>width</b></p>"
      "<p>Sets the tab width to the number <b>width</b></p>");
    return true;
  } else   if (realcmd=="set-replace-tab") {
    msg=i18n("<p>set-replace-tab <b>enable</b></p>"
      "<p>If enabled, tabs are replaced with spaces as you type.</p>"
      "<p>Possible true values: 1 on true<br/>"
      "possible false values: 0 off false</p>");
    return true;
  } else   if (realcmd=="set-show-tabs") {
    msg=i18n("<p>set-show-tabs <b>enable</b></p>"
      "<p>If enabled, TAB characters and trailing whitespace will be visualized by a small dot.</p>"
      "<p>Possible true values: 1 on true<br/>"
      "possible false values: 0 off false</p>");
    return true;
  } else   if (realcmd=="set-remove-trailing-spaces") {
    msg=i18n("<p>set-remove-trailing-spaces <b>mode</b></p>"
      "<p>Removes the trailing spaces in the document depending on the <b>mode</b>.</p>"
      "<p>Possible values:"
      "<ul>"
      "<li><b>none</b>: never remove trailing spaces.</li>"
      "<li><b>modified</b>: remove trailing spaces only of modified lines.</li>"
      "<li><b>all</b>: remove trailing spaces in the entire document.</li>"
      "</ul></p>");
    return true;
  } else   if (realcmd=="set-indent-width") {
    msg=i18n("<p>set-indent-width <b>width</b></p>"
      "<p>Sets the indentation width to the number <b>width</b>. Used only if you are indenting with spaces.</p>");
    return true;
  } else   if (realcmd=="set-indent-mode") {
    msg=i18n("<p>set-indent-mode <b>mode</b></p>"
      "<p>The mode parameter is a value as seen in the Tools - Indentation menu</p>");
    return true;
  } else   if (realcmd=="set-auto-indent") {
    msg=i18n("<p>set-auto-indent <b>enable</b></p>"
      "<p>Enable or disable autoindentation.</p>"
      "<p>possible true values: 1 on true<br/>"
      "possible false values: 0 off false</p>");
    return true;
  } else   if (realcmd=="set-line-numbers") {
    msg=i18n("<p>set-line-numbers <b>enable</b></p>"
      "<p>Sets the visibility of the line numbers pane.</p>"
      "<p> possible true values: 1 on true<br/>"
      "possible false values: 0 off false</p>");
    return true;
  } else   if (realcmd=="set-folding-markers") {
    msg=i18n("<p>set-folding-markers <b>enable</b></p>"
      "<p>Sets the visibility of the folding markers pane.</p>"
      "<p> possible true values: 1 on true<br/>"
      "possible false values: 0 off false</p>");
    return true;
  } else   if (realcmd=="set-icon-border") {
    msg=i18n("<p>set-icon-border <b>enable</b></p>"
      "<p>Sets the visibility of the icon border.</p>"
      "<p> possible true values: 1 on true<br/>"
      "possible false values: 0 off false</p>");
    return true;
  } else   if (realcmd=="set-word-wrap") {
    msg=i18n("<p>set-word-wrap <b>enable</b></p>"
      "<p>Enables dynamic word wrap according to <b>enable</b></p>"
      "<p> possible true values: 1 on true<br/>"
      "possible false values: 0 off false</p>");
    return true;
  } else   if (realcmd=="set-word-wrap-column") {
    msg=i18n("<p>set-word-wrap-column <b>width</b></p>"
      "<p>Sets the line width for hard wrapping to <b>width</b>. This is used if you are having your text wrapped automatically.</p>");
    return true;
  } else   if (realcmd=="set-replace-tabs-save") {
    msg=i18n("<p>set-replace-tabs-save <b>enable</b></p>"
      "<p>When enabled, tabs will be replaced with whitespace whenever the document is saved.</p>"
      "<p> possible true values: 1 on true<br/>"
      "possible false values: 0 off false</p>");
    return true;
  } else   if (realcmd=="set-highlight") {
    msg=i18n("<p>set-highlight <b>highlight</b></p>"
        "<p>Sets the syntax highlighting system for the document. The argument must be a valid highlight name, as seen in the Tools → Highlighting menu. This command provides an autocompletion list for its argument.</p>");
    return true;
  } else   if (realcmd=="set-mode") {
    msg=i18n("<p>set-mode <b>mode</b></p>"
      "<p>Sets the mode as seen in Tools - Mode</p>");
    return true;
  } else   if (realcmd=="set-show-indent") {
    msg=i18n("<p>set-show-indent <b>enable</b></p>"
      "<p>If enabled, indentation will be visualized by a vertical dotted line.</p>"
      "<p> possible true values: 1 on true<br/>"
      "possible false values: 0 off false</p>");
    return true;
  } else   if (realcmd=="print") {
    msg=i18n("<p>Open the Print dialog to print the current document.</p>");
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
  else if ( cmd == "fold" )
  {
    return (v->textFolding().newFoldingRange (range.isValid() ? range : v->selectionRange(), Kate::TextFolding::Persistent | Kate::TextFolding::Folded) != -1);
  }
  else if ( cmd == "tfold" )
  {
    return (v->textFolding().newFoldingRange (range.isValid() ? range : v->selectionRange(), Kate::TextFolding::Folded) != -1);
  }
  else if ( cmd == "unfold" )
  {
    QVector<QPair<qint64, Kate::TextFolding::FoldingRangeFlags> > startingRanges = v->textFolding().foldingRangesStartingOnLine (v->cursorPosition().line());
    bool unfolded = false;
    for (int i = 0; i < startingRanges.size(); ++i) {
      if (startingRanges[i].second & Kate::TextFolding::Folded) {
        unfolded = v->textFolding().unfoldRange (startingRanges[i].first) || unfolded;
      }
    }
    return unfolded;
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
      v->doc()->config()->setIndentationMode( args.join(" ") );
      v->doc()->rememberUserDidSetIndentationMode ();
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
            cmd == "set-indent-pasted-text" ||
            cmd == "set-line-numbers" ||
            cmd == "set-replace-tabs" ||
            cmd == "set-show-tabs" ||
            cmd == "set-word-wrap" ||
            cmd == "set-wrap-cursor" ||
            cmd == "set-replace-tabs-save" ||
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
      else if (cmd == "set-indent-pasted-text")
        config->setIndentPastedText( enable );
      else if ( cmd == "set-replace-tabs" )
        config->setReplaceTabsDyn( enable );
      else if ( cmd == "set-show-tabs" )
        config->setShowTabs( enable );
      else if ( cmd == "set-show-trailing-spaces" )
        config->setShowSpaces( enable );
      else if ( cmd == "set-word-wrap" )
        v->doc()->setWordWrap( enable );

      return true;
    }
    else
      KCC_ERR( i18n("Bad argument '%1'. Usage: %2 on|off|1|0|true|false",
                 args.first() ,  cmd ) );
  }
  else if ( cmd == "set-remove-trailing-spaces" ) {
    // need at least one item, otherwise args.first() crashes
    if ( args.count() != 1 )
      KCC_ERR( i18n("Usage: set-remove-trailing-spaces 0|-|none or 1|+|mod|modified or 2|*|all") );

    QString tmp = args.first().toLower().trimmed();
    if (tmp == "1" || tmp == "modified" || tmp == "mod" || tmp == "+") {
      v->doc()->config()->setRemoveSpaces(1);
    } else if (tmp == "2" || tmp == "all" || tmp == "*") {
      v->doc()->config()->setRemoveSpaces(2);
    } else {
      v->doc()->config()->setRemoveSpaces(0);
    }
  }

  // unlikely..
  KCC_ERR( i18n("Unknown command '%1'", cmd) );
}

bool KateCommands::CoreCommands::supportsRange(const QString &range)
{
  static QStringList l;

  if (l.isEmpty())
  l << "indent" << "unindent" << "cleanindent"
    << "comment" << "uncomment" << "kill-line" << "fold" << "tfold";

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
  else if ( cmd == "set-remove-trailing-spaces" )
  {
    QStringList l;
    l << "none" << "modified" << "all";

    KateCmdShellCompletion *co = new KateCmdShellCompletion();
    co->setItems( l );
    co->setIgnoreCase( true );
    return co;
  }
  else if ( cmd == "set-indent-mode" )
  {
    QStringList l = KateAutoIndent::listIdentifiers();
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
  l << mappingCommands() << "d" << "delete" << "j" << "c" << "change" << "<" << ">" << "y" << "yank" <<
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
  if (mappingCommands().contains(cmd))
  {
    if (cmd.endsWith("unmap"))
    {
      if (args.count() == 1)
      {
        KateGlobal::self()->viInputModeGlobal()->removeMapping(modeForMapCommand(cmd), args.at(0));
        return true;
      }
      else
      {
        msg = i18n("Missing argument. Usage: %1 <from>",  cmd );
        return false;
      }
    }
    if ( args.count() == 1 ) {
      msg = KateGlobal::self()->viInputModeGlobal()->getMapping( modeForMapCommand(cmd), args.at( 0 ), true );
      if ( msg.isEmpty() ) {
        msg = i18n( "No mapping found for \"%1\"", args.at(0) );
        return false;
      } else {
        msg = i18n( "\"%1\" is mapped to \"%2\"", args.at( 0 ), msg );
      }
    } else if ( args.count() == 2 ) {
      KateViGlobal::MappingRecursion mappingRecursion = (isMapCommandRecursive(cmd)) ? KateViGlobal::Recursive : KateViGlobal::NonRecursive;
      KateGlobal::self()->viInputModeGlobal()->addMapping( modeForMapCommand(cmd), args.at( 0 ), args.at( 1 ), mappingRecursion);
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
    QStringList l = KateGlobal::self()->viInputModeGlobal()->getMappings( KateViGlobal::NormalModeMapping );

    KateCmdShellCompletion *co = new KateCmdShellCompletion();
    co->setItems( l );
    co->setIgnoreCase( false );
    return co;
  }
  return 0L;
}

const QStringList& KateCommands::ViCommands::mappingCommands()
{
  static QStringList mappingsCommands;
  if (mappingsCommands.isEmpty())
  {
    mappingsCommands << "nmap"  << "nm"  << "noremap" << "nnoremap" << "nn" << "no"
                     << "vmap" << "vm" << "vnoremap" << "vn"
                     << "imap" << "im" << "inoremap" << "ino"
                     << "cmap" << "cm" << "cnoremap" << "cno";

    mappingsCommands << "nunmap" << "vunmap" << "iunmap" << "cunmap";
  }
  return mappingsCommands;
}

KateViGlobal::MappingMode KateCommands::ViCommands::modeForMapCommand(const QString& mapCommand)
{
  static QMap<QString, KateViGlobal::MappingMode> modeForMapCommand;
  if (modeForMapCommand.isEmpty())
  {
    // Normal is the default.
    modeForMapCommand.insert("vmap", KateViGlobal::VisualModeMapping);
    modeForMapCommand.insert("vm", KateViGlobal::VisualModeMapping);
    modeForMapCommand.insert("vnoremap", KateViGlobal::VisualModeMapping);
    modeForMapCommand.insert("vn", KateViGlobal::VisualModeMapping);
    modeForMapCommand.insert("imap", KateViGlobal::InsertModeMapping);
    modeForMapCommand.insert("im", KateViGlobal::InsertModeMapping);
    modeForMapCommand.insert("inoremap", KateViGlobal::InsertModeMapping);
    modeForMapCommand.insert("ino", KateViGlobal::InsertModeMapping);
    modeForMapCommand.insert("cmap", KateViGlobal::CommandModeMapping);
    modeForMapCommand.insert("cm", KateViGlobal::CommandModeMapping);
    modeForMapCommand.insert("cnoremap", KateViGlobal::CommandModeMapping);
    modeForMapCommand.insert("cno", KateViGlobal::CommandModeMapping);

    modeForMapCommand.insert("nunmap", KateViGlobal::NormalModeMapping);
    modeForMapCommand.insert("vunmap", KateViGlobal::VisualModeMapping);
    modeForMapCommand.insert("iunmap", KateViGlobal::InsertModeMapping);
    modeForMapCommand.insert("cunmap", KateViGlobal::CommandModeMapping);
  }
  return modeForMapCommand[mapCommand];
}

bool KateCommands::ViCommands::isMapCommandRecursive(const QString& mapCommand)
{
  static QMap<QString, bool> isMapCommandRecursive;
  {
    isMapCommandRecursive.insert("nmap", true);
    isMapCommandRecursive.insert("nm", true);
    isMapCommandRecursive.insert("vmap", true);
    isMapCommandRecursive.insert("vm", true);
    isMapCommandRecursive.insert("imap", true);
    isMapCommandRecursive.insert("im", true);
    isMapCommandRecursive.insert("cmap", true);
    isMapCommandRecursive.insert("cm", true);
  }
  return isMapCommandRecursive[mapCommand];
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

  int findBeginPos = -1;
  int findEndPos = -1;
  int replaceBeginPos = -1;
  int replaceEndPos = -1;
  QString delimiter;
  if (!parse(cmd, delimiter, findBeginPos, findEndPos, replaceBeginPos, replaceEndPos))
  {
    return false;
  }

  const QString searchParamsString = cmd.mid(cmd.lastIndexOf(delimiter));
  const bool noCase = searchParamsString.contains('i');
  const bool repeat = searchParamsString.contains('g');
  const bool interactive = searchParamsString.contains('c');

  QString find = cmd.mid(findBeginPos, findEndPos - findBeginPos + 1);
  kDebug(13025) << "SedReplace: find =" << find;

  QString replace = cmd.mid(replaceBeginPos, replaceEndPos - replaceBeginPos + 1);
  exchangeAbbrevs(replace);
  kDebug(13025) << "SedReplace: replace =" << replace;

  if (find.isEmpty())
  {
    // Nothing to do.
    return true;
  }

  KateView *kateView = static_cast<KateView*>(view);
  KateDocument *doc = kateView->doc();
  if ( !doc ) return false;
  // Only current line ...
  int startLine = kateView->cursorPosition().line();
  int endLine = kateView->cursorPosition().line();
  // ... unless a range was provided.
  if (r.isValid())
  {
    startLine = r.start().line();
    endLine = r.end().line();
  }

  QSharedPointer<InteractiveSedReplacer> interactiveSedReplacer(new InteractiveSedReplacer(doc, find, replace, !noCase, !repeat, startLine, endLine));

  if (interactive)
  {
    if (kateView->viInputMode() && KateViewConfig::global()->viInputModeEmulateCommandBar())
    {
      KateViEmulatedCommandBar *emulatedCommandBar = kateView->viModeEmulatedCommandBar();
      emulatedCommandBar->startInteractiveSearchAndReplace(interactiveSedReplacer);
      return true;
    }
    else
    {
      kDebug(13025) << "Interactive sedreplace is only currently supported with Vi mode plus Vi emulated command bar.";
      return false;
    }
  }

  kateView->setSearchPattern(find);

  interactiveSedReplacer->replaceAllRemaining();
  msg = interactiveSedReplacer->finalStatusReportMessage();

  return true;
}

bool KateCommands::SedReplace::parse(const QString& sedReplaceString, QString& destDelim, int& destFindBeginPos, int& destFindEndPos, int& destReplaceBeginPos, int& destReplaceEndPos)
{
  // valid delimiters are all non-word, non-space characters plus '_'
  QRegExp delim("^s\\s*([^\\w\\s]|_)");
  if ( delim.indexIn( sedReplaceString ) < 0 ) return false;

  QString d = delim.cap(1);
  kDebug(13025) << "SedReplace: delimiter is '" << d << "'";

  QRegExp splitter( QString("^s\\s*") + d + "((?:[^\\\\\\" + d + "]|\\\\.)*)\\"
      + d + "((?:[^\\\\\\" + d + "]|\\\\.)*)(\\" + d + "[igc]{0,3})?$" );
  if (splitter.indexIn(sedReplaceString) < 0) return false;

  const QString find = splitter.cap(1);
  const QString replace = splitter.cap(2);

  destDelim = d;
  destFindBeginPos = splitter.pos(1);
  destFindEndPos = splitter.pos(1) + find.length() - 1;
  destReplaceBeginPos = splitter.pos(2);
  destReplaceEndPos = splitter.pos(2) + replace.length() - 1;

  return true;
}

KateCommands::SedReplace::InteractiveSedReplacer::InteractiveSedReplacer(KateDocument* doc, const QString& findPattern, const QString& replacePattern, bool caseSensitive, bool onlyOnePerLine, int startLine, int endLine)
    : m_findPattern(findPattern),
      m_replacePattern(replacePattern),
      m_onlyOnePerLine(onlyOnePerLine),
      m_startLine(startLine),
      m_endLine(endLine),
      m_doc(doc),
      m_regExpSearch(doc, caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive),
      m_numReplacementsDone(0),
      m_numLinesTouched(0),
      m_lastChangedLineNum(-1)
{
  m_currentSearchPos = Cursor(startLine, 0);
}

Range KateCommands::SedReplace::InteractiveSedReplacer::currentMatch()
{
  QVector<Range> matches = fullCurrentMatch();

  if (matches.isEmpty()) {
    return Range::invalid();
  }

  if (matches.first().start().line() > m_endLine) {
    return Range::invalid();
  }

  return matches.first();
}

void KateCommands::SedReplace::InteractiveSedReplacer::skipCurrentMatch()
{
  const Range currentMatch = this->currentMatch();
  m_currentSearchPos = currentMatch.end();
  if (m_onlyOnePerLine && currentMatch.start().line() == m_currentSearchPos.line())
  {
    m_currentSearchPos = Cursor(m_currentSearchPos.line() + 1, 0);
  }
}

void KateCommands::SedReplace::InteractiveSedReplacer::replaceCurrentMatch()
{
  const Range currentMatch = this->currentMatch();
  const QString currentMatchText = m_doc->text(currentMatch);
  const QString replacementText = replacementTextForCurrentMatch();

  m_doc->editBegin();
  m_doc->removeText(currentMatch);
  m_doc->insertText(currentMatch.start(), replacementText);
  m_doc->editEnd();

  // Begin next search from directly after replacement.
  if (!replacementText.contains('\n')) {
    const int moveChar = currentMatch.isEmpty() ? 1 : 0; // if the search was for \s*, make sure we advance a char
    const int col = currentMatch.start().column() + replacementText.length() + moveChar;

    m_currentSearchPos = Cursor(currentMatch.start().line(), col);
  } else {
    m_currentSearchPos = Cursor(currentMatch.start().line() + replacementText.count('\n'),
                                replacementText.length() - replacementText.lastIndexOf('\n') - 1);
  }
  if (m_onlyOnePerLine)
  {
    // Drop down to next line.
    m_currentSearchPos = Cursor(m_currentSearchPos.line() + 1, 0);
  }

  // Adjust end line down by the number of new newlines just added, minus the number taken away.
  m_endLine += replacementText.count('\n');
  m_endLine -= currentMatchText.count('\n');

  m_numReplacementsDone++;
  if (m_lastChangedLineNum != currentMatch.start().line())
  {
    // Counting "swallowed" lines as being "touched".
    m_numLinesTouched += currentMatchText.count('\n') + 1;
  }
  m_lastChangedLineNum = m_currentSearchPos.line();
}

void KateCommands::SedReplace::InteractiveSedReplacer::replaceAllRemaining()
{
  m_doc->editBegin();
  while (currentMatch().isValid())
  {
    replaceCurrentMatch();
  }
  m_doc->editEnd();
}

QString KateCommands::SedReplace::InteractiveSedReplacer::currentMatchReplacementConfirmationMessage()
{
  return i18n("replace with %1?", replacementTextForCurrentMatch().replace('\n', "\\n"));
}

QString KateCommands::SedReplace::InteractiveSedReplacer::finalStatusReportMessage()
{
  return i18ncp("%2 is the translation of the next message",
               "1 replacement done on %2", "%1 replacements done on %2", m_numReplacementsDone,
               i18ncp("substituted into the previous message",
                      "1 line", "%1 lines", m_numLinesTouched));

}


const QVector< Range > KateCommands::SedReplace::InteractiveSedReplacer::fullCurrentMatch()
{
  if (m_currentSearchPos > m_doc->documentEnd()) {
    return QVector<Range>();
  }

  return m_regExpSearch.search(m_findPattern, Range(m_currentSearchPos, m_doc->documentEnd()));
}

QString KateCommands::SedReplace::InteractiveSedReplacer::replacementTextForCurrentMatch()
{
  const Range currentMatch = this->currentMatch();
  const QVector<KTextEditor::Range> captureRanges = fullCurrentMatch();
  QStringList captureTexts;
  foreach(const Range& captureRange, captureRanges)
  {
    captureTexts << m_doc->text(captureRange);
  }
  const QString replacementText = m_regExpSearch.buildReplacement(m_replacePattern, captureTexts, 0);
  return replacementText;

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
        "<p>Inserts a date/time string as defined by the specified format, or the format yyyy-MM-dd hh:mm:ss if none is specified.</p>"
        "<p>Possible format specifiers are:"
        "<table>"
            "<tr><td>d</td><td>The day as number without a leading zero (1-31).</td></tr>"
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
