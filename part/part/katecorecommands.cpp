/* This file is part of the KDE libraries
   Copyright (C) 2003 Anders Lund <anders@alweb.dk>

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

#include "katecorecommands.h"
#include "kateview.h"
#include "katedocument.h"

#include <klocale.h>

#include <qregexp.h>

QStringList KateCoreCommands::cmds()
{
  QStringList l;
  l << "indent" << "unindent" << "cleanindent"
    << "comment" << "uncomment"
    << "set-tab-width" << "set-replace-tabs"
    << "set-indent-char" << "set-indent-width" << "set-indent"
    << "set-line-numbers" << "set-folding-markers" << "set-icon-border";
  return l;
}

bool KateCoreCommands::exec(Kate::View *view,
                            const QString &_cmd,
                            QString &errorMsg)
{
#define KCC_ERR(s) { errorMsg=s; return false; }
  KateView *v = dynamic_cast<KateView*>( view );
  if ( ! v )
    KCC_ERR( i18n("Could not access view") );

  //create a list of args
  QStringList args( QStringList::split( QRegExp("\\s+"), _cmd ) );
  QString cmd ( args.first() );
  args.remove( args.first() );

  // ALL commands that takes no arguments.
  if ( cmd == "indent" )
  {
    v->indent();
    return true;
  }
  else if ( cmd == "unindent" )
  {
    v->unIndent();
    return true;
  }
  else if ( cmd == "cleanindent" )
  {
    v->cleanIndent();
    return true;
  }
  else if ( cmd == "comment" )
  {
    v->comment();
    return true;
  }
  else if ( cmd == "uncomment" )
  {
    v->uncomment();
    return true;
  }
  // ALL commands that takes exactly one integer argument.
  else if ( cmd == "set-tab-width" ||
            "set-indent-width")
  {
    // find a integer value > 0
    if ( ! args.count() )
      KCC_ERR( i18n("Missing argument. Usage: %1 <value>").arg( cmd ) );
    bool ok;
    int val ( args.first().toInt( &ok ) );
    if ( !ok )
      KCC_ERR( i18n("Failed to convert argument '%1' to integer.")
                .arg( args.first() ) );

    if ( cmd == "set-tab-width" )
    {
      if ( val < 2 )
        KCC_ERR( i18n("Width must be at least 1.") );
      v->setTabWidth( val );
    }
    else if ( cmd == "set-indent-width" )
    {
      if ( val < 2 )
        KCC_ERR( i18n("Width must be at least 1.") );
      v->doc()->setIndentationWidth( val );
    }
    return true;
  }
  // ALL commands that takes 1 boolean argument.
  else if ( cmd == "set-icon-border" ||
            cmd == "set-folding-markers" ||
            cmd == "set-line-numbers" ||
            cmd == "set-replace-tabs" ) //FIXME
  {
    if ( ! args.count() )
      KCC_ERR( i18n("Usage: %1 on|off|1|0|true|false").arg( cmd ) );
    bool enable;
    if ( getBoolArg( args.first(), &enable ) )
    {
      if ( cmd == "set-icon-border" )
        v->setIconBorder( enable );
      else if (cmd == "set-folding-markers")
        v->setFoldingMarkersOn( enable );
      else if ( cmd == "set-line-numbers" )
        v->setLineNumbersOn( enable );
      return true;
    }
    else
      KCC_ERR( i18n("Bad argument '%1'. Usage: %2 on|off|1|0|true|false").arg( args.first() ).arg( cmd ) );
  }
  // Commands that take string or mixed arguments
  else if ( cmd == "set-indent-char" )
  {
  }
  else if ( cmd == "set-indent" )
  {
  }

  KCC_ERR( i18n("Unknown command '%1'").arg(cmd) );
}

// this returns wheather the string s could be converted to
// a bool value, one of on|off|1|0|true|false. the argument val is
// set to the extracted value in case of success
bool KateCoreCommands::getBoolArg( QString s, bool *val  )
{
  bool res( false );
  s = s.lower();
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
