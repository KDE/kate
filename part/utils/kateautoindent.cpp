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
#include "kateindentscript.h"
#include "katescriptmanager.h"
#include "kateview.h"
#include "kateextendedattribute.h"
#include "katedocument.h"

#include <klocale.h>
#include <kdebug.h>
#include <kmenu.h>

#include <cctype>

const QString MODE_NONE = QLatin1String("none");
const QString MODE_NORMAL = QLatin1String("normal");

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
  return 2 +  KateGlobal::self()->scriptManager()->indentationScripts();
}


QString KateAutoIndent::modeName (int mode)
{
  if (mode == 0 || mode >= modeCount ())
    return MODE_NONE;

  if (mode == 1)
    return MODE_NORMAL;

  return KateGlobal::self()->scriptManager()->indentationScriptByIndex(mode-2)->information().baseName;
}

QString KateAutoIndent::modeDescription (int mode)
{
  if (mode == 0 || mode >= modeCount ())
    return i18nc ("Autoindent mode", "None");

  if (mode == 1)
    return i18nc ("Autoindent mode", "Normal");

  return KateGlobal::self()->scriptManager()->indentationScriptByIndex(mode-2)->information().name;
}

QString KateAutoIndent::modeRequiredStyle(int mode)
{
  if (mode == 0 || mode == 1 || mode >= modeCount())
    return QString();

  return KateGlobal::self()->scriptManager()->indentationScriptByIndex(mode-2)->information().requiredStyle;
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

QString KateAutoIndent::tabString (int length, int align) const
{
  QString s;
  length = qMin (length, 256); // sanity check for large values of pos
  int spaces = qBound(0, align - length, 256);

  if (!useSpaces)
  {
    s.append (QString (length / tabWidth, '\t'));
    length = length % tabWidth;
  }
  s.append(QString(length + spaces, ' '));

  return s;
}

bool KateAutoIndent::doIndent(KateView *view, int line, int indentDepth, int align)
{
  kDebug (13060) << "doIndent: line: " << line << " indentDepth: " << indentDepth << " align: " << align;

  KateTextLine::Ptr textline = doc->plainKateTextLine(line);

  // textline not found, cu
  if (!textline)
    return false;

  // sanity check
  if (indentDepth < 0)
    indentDepth = 0;

  QString indentString = tabString (indentDepth, align);

  int first_char = textline->firstChar();

  if (first_char < 0)
    first_char = textline->length();

  // remove leading whitespace, then insert the leading indentation
  doc->editStart (view);
  doc->editRemoveText (line, 0, first_char);
  doc->editInsertText (line, 0, indentString);
  doc->editEnd ();

  return true;
}

bool KateAutoIndent::doIndentRelative(KateView *view, int line, int change)
{
  kDebug (13060) << "doIndentRelative: line: " << line << " change: " << change;

  KateTextLine::Ptr textline = doc->plainKateTextLine(line);

  // get indent width of current line
  int indentDepth = textline->indentDepth (tabWidth);
  int extraSpaces = indentDepth % indentWidth;

  // add change
  indentDepth += change;

  // if keepExtra is off, snap to a multiple of the indentWidth
  if (!keepExtra && extraSpaces > 0)
  {
    if (change < 0)
      indentDepth += indentWidth - extraSpaces;
    else
      indentDepth -= extraSpaces;
  }

  // do indent
  return doIndent(view, line, indentDepth);
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

  doIndent (view, line, textline->indentDepth (tabWidth));
}

void KateAutoIndent::scriptIndent (KateView *view, const KTextEditor::Cursor &position, QChar typedChar)
{
  QPair<int, int> result = m_script->indent (view, position, typedChar, indentWidth);
  int newIndentInChars = result.first;

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

  int align = result.second;
  if (align > 0)
    kDebug (13060) << "Align: " << align;

  // we got a positive or zero indent to use...
  doIndent (view, position.line(), newIndentInChars, align);
}

bool KateAutoIndent::isStyleProvided(KateIndentScript *script)
{
  QString requiredStyle = script->information().requiredStyle;
  return (requiredStyle.isEmpty() || requiredStyle == doc->highlight()->style());
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
  if ( name.isEmpty() || name == MODE_NONE )
  {
    m_mode = MODE_NONE;
    return;
  }
  
  if ( name == MODE_NORMAL )
  {
    m_normal = true;
    m_mode = MODE_NORMAL;
    return;
  }
  
  // handle script indenters, if any for this name...
  KateIndentScript *script = KateGlobal::self()->scriptManager()->indentationScript(name);
  if ( script )
  {
    if (isStyleProvided(script))
    {
      m_script = script;
      m_mode = name;
      
      kDebug( 13060 ) << "mode: " << name << "accepted";
      return;
    }
    else
    {
      kWarning( 13060 ) << "mode" << name << "requires a different highlight style";
    }
  }
  else 
  {
    kWarning( 13060 ) << "mode" << name << "does not exist";
  }
  
  // Fall back to normal
  m_normal = true;
  m_mode = MODE_NORMAL;
}

void KateAutoIndent::checkRequiredStyle()
{
  if (m_script)
  {
    if (!isStyleProvided(m_script))
    {
      kDebug( 13060 ) << "mode" << m_mode << "requires a different highlight style";
      doc->config()->setIndentationMode(MODE_NORMAL);
    }
  }
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
  QList<int> skippedLines;

  // loop over all lines given...
  for (int line = range.start().line () < 0 ? 0 : range.start().line ();
       line <= qMin (range.end().line (), doc->lines()-1); ++line)
  {
    // don't indent empty lines
    if (doc->line(line).isEmpty())
    {
      skippedLines.append (line);
      continue;
    }
    // don't indent the last line when the cursor is on the first column
    if (line == range.end().line() && range.end().column() == 0)
    {
      skippedLines.append (line);
      continue;
    }

    doIndentRelative(view, line, change * indentWidth);
  }

  if (skippedLines.count() > range.numberOfLines())
  {
    // all lines were empty, so indent them nevertheless
    foreach (int line, skippedLines)
      doIndentRelative(view, line, change * indentWidth);
  }

  return true;
}

void KateAutoIndent::indent (KateView *view, const KTextEditor::Range &range)
{
  // no script, do nothing...
  if (!m_script)
    return;

  doc->pushEditState();
  doc->editStart();
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
  if (typedChar != '\n' && !m_script->triggerCharacters().contains(typedChar))
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

    QString requiredStyle = KateAutoIndent::modeRequiredStyle(z);
    action->setEnabled(requiredStyle.isEmpty() || requiredStyle == doc->highlight()->style());

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
