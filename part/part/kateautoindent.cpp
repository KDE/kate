/* This file is part of the KDE libraries
   Copyright (C) 2003 Jesse Yurkovich <yurkjes@iit.edu>
   Copyright (C) 2004 >Anders Lund <anders@alweb.dk> (KateVarIndent class)
   Copyright (C) 2005 Dominik Haumann <dhdev@gmx.de> (basic support for config page)

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

#include "kateautoindent.h"
#include "kateautoindent.moc"

#include "kateconfig.h"
#include "katehighlight.h"
#include "kateglobal.h"
#include "katejscript.h"
#include "kateview.h"
#include "kateextendedattribute.h"
#include "katedocument.h"

#include <klocale.h>
#include <kdebug.h>
#include <kmenu.h>

#include <cctype>

//BEGIN KateAutoIndent

QStringList KateAutoIndent::listModes ()
{
  QStringList l;

  for (int i = 0; i < modeCount(); ++i)
    l << modeDescription(i);

  return l;
}

int KateAutoIndent::modeCount ()
{
  // inbuild modes + scripts
  return 2 +  KateGlobal::self()->indentScriptManager()->scripts();
}


QString KateAutoIndent::modeName (int mode)
{
  if (mode == 0 || mode >= modeCount ())
    return QString ("none");

  if (mode == 1)
    return QString ("normal");

  return KateGlobal::self()->indentScriptManager()->scriptByIndex(mode-2)->basename ();
}

QString KateAutoIndent::modeDescription (int mode)
{
  if (mode == 0 || mode >= modeCount ())
    return i18n ("None");

  if (mode == 1)
    return i18n ("Normal");

  return KateGlobal::self()->indentScriptManager()->scriptByIndex(mode-2)->name();
}

uint KateAutoIndent::modeNumber (const QString &name)
{
  for (int i = 0; i < modeCount(); ++i)
    if (modeName(i) == name)
      return i;

  return 0;
}

KateAutoIndent::KateAutoIndent (KateDocument *_doc)
  : doc(_doc), m_normal (false), m_script (0)
{
  // don't call updateConfig() here, document might is not ready for that....
}

KateAutoIndent::~KateAutoIndent ()
{
}

QString KateAutoIndent::tabString (int length) const
{
  QString s;
  length = qMin (length, 256); // sanity check for large values of pos

  if (!useSpaces)
  {
    s.append (QString (length / tabWidth, '\t'));
    length = length % tabWidth;
  }
  s.append (QString (length, ' '));

  return s;
}

bool KateAutoIndent::doIndent ( KateView *view, int line, int change, bool relative, bool keepExtraSpaces )
{
  kDebug (13060) << "doIndent: line: " << line << " change: " << change << " relative: " << relative << endl;

  KateTextLine::Ptr textline = doc->plainKateTextLine(line);

  // textline not found, cu
  if (!textline)
    return false;

  int extraSpaces = 0;

  // get indent width of current line
  int indentDepth = (relative || keepExtraSpaces) ? textline->indentDepth (tabWidth) : 0;

  // for relative change, check for extra spaces
  if (relative)
  {
    extraSpaces = indentDepth % indentWidth;

    indentDepth += change;

    // if keepExtraSpaces is off, snap to a multiple of the indentWidth
    if (!keepExtraSpaces && extraSpaces > 0)
    {
      if (change < 0)
        indentDepth += indentWidth - extraSpaces;
      else
        indentDepth -= extraSpaces;
    }
  } else {
    indentDepth = change;
  }

  // sanity check
  if (indentDepth < 0)
    indentDepth = 0;

  QString indentString = tabString (indentDepth);

  int first_char = textline->firstChar();

  if (first_char < 0)
    first_char = textline->length();

  // remove trailing spaces, then insert the leading indentation
  doc->editStart (view);
  doc->editRemoveText (line, 0, first_char);
  doc->editInsertText (line, 0, indentString);
  doc->editEnd ();

  return true;
}

void KateAutoIndent::keepIndent ( KateView *view, int line )
{
  // no line in front, no work...
  if (line <= 0)
    return;

  KateTextLine::Ptr textline = doc->plainKateTextLine(line-1);

  // textline not found, cu
  if (!textline)
    return;

  doIndent (view, line, textline->indentDepth (tabWidth), false);
}

void KateAutoIndent::scriptIndent (KateView *view, const KTextEditor::Cursor &position, QChar typedChar)
{
  int newIndentInChars = m_script->indent (view, position, typedChar, indentWidth);

  // handle negative values special
  if (newIndentInChars < -1)
    return;

  // reuse indentation of the previous line, just like the "normal" indenter
  if (newIndentInChars == -1)
  {
    // keep indent of previous line
    keepIndent (view, position.line());

    return;
  }

  // we got a positive or zero indent to use...
  doIndent (view, position.line(), newIndentInChars, false);
}

void KateAutoIndent::setMode (const QString &name)
{
  // bail out, already set correct mode...
  if (m_mode == name)
    return;

  // cleanup
  m_script = 0;
  m_normal = false;

  // first, catch easy stuff... normal mode and none, easy...
  if ( name.isEmpty() || name == QString ("none") || name == QString ("normal") )
  {
    m_normal = (name == QString ("normal"));
    m_mode = (name == QString ("normal")) ? QString ("normal") : QString ("none");
    return;
  }

  // handle script indenters, if any for this name...
  KateIndentJScript *script = KateGlobal::self()->indentScriptManager()->script(name);
  if ( script )
  {
    m_script = script;
    m_mode = name;
    return;
  }

  // default: none
  m_mode = QString ("none");
}

void KateAutoIndent::updateConfig ()
{
  KateDocumentConfig *config = doc->config();

  useSpaces   = config->configFlags() & KateDocumentConfig::cfReplaceTabsDyn;
  keepExtra   = config->configFlags() & KateDocumentConfig::cfKeepExtraSpaces;
  tabWidth    = config->tabWidth();
  indentWidth = config->indentationWidth();
}


bool KateAutoIndent::changeIndent (KateView *view, const KTextEditor::Range &range, int change)
{
  // loop over all lines given...
  for (int line = range.start().line () < 0 ? 0 : range.start().line ();
       line <= qMin (range.end().line (), doc->lines()-1); ++line)
  {
    doIndent (view, line, change * indentWidth, true, keepExtra);
  }

  return true;
}

void KateAutoIndent::indent (KateView *view, const KTextEditor::Range &range)
{
  // no script, do nothing...
  if (!m_script)
    return;

  doc->pushEditState();
  doc->editStart (view);
  // loop over all lines given...
  for (int line = range.start().line () < 0 ? 0 : range.start().line ();
       line <= qMin (range.end().line (), doc->lines()-1); ++line)
  {
    // let the script indent for us...
    scriptIndent (view, KTextEditor::Cursor (line, 0), QChar());
  }
  doc->editEnd ();
  doc->popEditState();
}

void KateAutoIndent::userTypedChar (KateView *view, const KTextEditor::Cursor &position, QChar typedChar)
{
  // normal mode
  if (m_normal)
  {
    // only indent on new line, per default
    if (typedChar != '\n')
      return;

    // keep indent of previous line
    keepIndent (view, position.line());

    return;
  }

  // no script, do nothing...
  if (!m_script)
    return;

  // does the script allow this char as trigger?
  if (typedChar != '\n' && !m_script->triggerCharacters(view).contains(typedChar))
    return;

  // let the script indent for us...
  scriptIndent (view, position, typedChar);
}
//END KateAutoIndent

//BEGIN KateViewIndentAction
KateViewIndentationAction::KateViewIndentationAction(KateDocument *_doc, const QString& text, QObject *parent)
       : KActionMenu (text, parent), doc(_doc)
{
  connect(menu(),SIGNAL(aboutToShow()),this,SLOT(slotAboutToShow()));

}

void KateViewIndentationAction::slotAboutToShow()
{
  QStringList modes = KateAutoIndent::listModes ();

  menu()->clear ();
  for (int z=0; z<modes.size(); ++z) {
    QAction *action = menu()->addAction( '&' + KateAutoIndent::modeDescription(z).replace('&', "&&") );
    action->setCheckable( true );
    action->setData( z );

    if ( doc->config()->indentationMode() == KateAutoIndent::modeName (z) )
      action->setChecked( true );
  }

  disconnect( menu(), SIGNAL( triggered( QAction* ) ), this, SLOT( setMode( QAction* ) ) );
  connect( menu(), SIGNAL( triggered( QAction* ) ), this, SLOT( setMode( QAction* ) ) );
}

void KateViewIndentationAction::setMode (QAction *action)
{
  // set new mode
  doc->config()->setIndentationMode(KateAutoIndent::modeName (action->data().toInt()));
}
//END KateViewIndentationAction

// kate: space-indent on; indent-width 2; replace-tabs on;
