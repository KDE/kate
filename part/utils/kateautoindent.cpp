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
  return 2 + KateGlobal::self()->scriptManager()->indentationScriptCount();
}


QString KateAutoIndent::modeName (int mode)
{
  if (mode == 0 || mode >= modeCount ())
    return MODE_NONE;

  if (mode == 1)
    return MODE_NORMAL;

  return KateGlobal::self()->scriptManager()->indentationScriptByIndex(mode-2)->indentHeader().baseName();
}

QString KateAutoIndent::modeDescription (int mode)
{
  if (mode == 0 || mode >= modeCount ())
    return i18nc ("Autoindent mode", "None");

  if (mode == 1)
    return i18nc ("Autoindent mode", "Normal");

  return KateGlobal::self()->scriptManager()->indentationScriptByIndex(mode-2)->indentHeader().name();
}

QString KateAutoIndent::modeRequiredStyle(int mode)
{
  if (mode == 0 || mode == 1 || mode >= modeCount())
    return QString();

  return KateGlobal::self()->scriptManager()->indentationScriptByIndex(mode-2)->indentHeader().requiredStyle();
}

uint KateAutoIndent::modeNumber (const QString &name)
{
  for (int i = 0; i < modeCount(); ++i)
    if (modeName(i) == name)
      return i;

  return 0;
}

KateAutoIndent::KateAutoIndent (KateDocument *_doc)
  : QObject(_doc), doc(_doc), m_script (0)
{
  // don't call updateConfig() here, document might is not ready for that....

  // on script reload, the script pointer is invalid -> force reload
  connect(KateGlobal::self()->scriptManager(), SIGNAL(reloaded()),
          this, SLOT(reloadScript()));
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

bool KateAutoIndent::doIndent(int line, int indentDepth, int align)
{
  kDebug (13060) << "doIndent: line: " << line << " indentDepth: " << indentDepth << " align: " << align;

  Kate::TextLine textline = doc->plainKateTextLine(line);

  // textline not found, cu
  if (!textline)
    return false;

  // sanity check
  if (indentDepth < 0)
    indentDepth = 0;

  const QString oldIndentation = textline->leadingWhitespace();

  // Preserve existing "tabs then spaces" alignment if and only if:
  //  - no alignment was passed to doIndent and
  //  - we aren't using spaces for indentation and
  //  - we aren't rounding indentation up to the next multiple of the indentation width and
  //  - we aren't using a combination to tabs and spaces for alignment, or in other words
  //    the indent width is a multiple of the tab width.
  bool preserveAlignment = !useSpaces && keepExtra && indentWidth % tabWidth == 0;
  if (align == 0 && preserveAlignment)
  {
    // Count the number of consecutive spaces at the end of the existing indentation
    int i = oldIndentation.size() - 1;
    while (i >= 0 && oldIndentation.at(i) == ' ')
      --i;
    // Use the passed indentDepth as the alignment, and set the indentDepth to
    // that value minus the number of spaces found (but don't let it get negative).
    align = indentDepth;
    indentDepth = qMax(0, align - (oldIndentation.size() - 1 - i));
  }

  QString indentString = tabString(indentDepth, align);

  // remove leading whitespace, then insert the leading indentation
  doc->editStart ();
  doc->editRemoveText (line, 0, oldIndentation.length());
  doc->editInsertText (line, 0, indentString);
  doc->editEnd ();

  return true;
}

bool KateAutoIndent::doIndentRelative(int line, int change)
{
  kDebug (13060) << "doIndentRelative: line: " << line << " change: " << change;

  Kate::TextLine textline = doc->plainKateTextLine(line);

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
  return doIndent(line, indentDepth);
}

void KateAutoIndent::keepIndent ( int line )
{
  // no line in front, no work...
  if (line <= 0)
    return;

  Kate::TextLine prevTextLine = doc->plainKateTextLine(line-1);
  Kate::TextLine textLine     = doc->plainKateTextLine(line);

  // textline not found, cu
  if (!prevTextLine || !textLine)
    return;

  const QString previousWhitespace = prevTextLine->leadingWhitespace();

  // remove leading whitespace, then insert the leading indentation
  doc->editStart ();

  if (!keepExtra)
  {
    const QString currentWhitespace = textLine->leadingWhitespace();
    doc->editRemoveText (line, 0, currentWhitespace.length());
  }

  doc->editInsertText (line, 0, previousWhitespace);
  doc->editEnd ();
}

void KateAutoIndent::reloadScript()
{
  // small trick to force reload
  m_script = 0; // prevent dangling pointer
  QString currentMode = m_mode;
  m_mode = QString();
  setMode(currentMode);
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
    keepIndent (position.line());

    return;
  }

  int align = result.second;
  if (align > 0)
    kDebug (13060) << "Align: " << align;

  // we got a positive or zero indent to use...
  doIndent (position.line(), newIndentInChars, align);
}

bool KateAutoIndent::isStyleProvided(const KateIndentScript *script, const KateHighlighting *highlight)
{
  QString requiredStyle = script->indentHeader().requiredStyle();
  return (requiredStyle.isEmpty() || requiredStyle == highlight->style());
}

void KateAutoIndent::setMode (const QString &name)
{
  // bail out, already set correct mode...
  if (m_mode == name)
    return;

  // cleanup
  m_script = 0;

  // first, catch easy stuff... normal mode and none, easy...
  if ( name.isEmpty() || name == MODE_NONE )
  {
    m_mode = MODE_NONE;
    return;
  }

  if ( name == MODE_NORMAL )
  {
    m_mode = MODE_NORMAL;
    return;
  }

  // handle script indenters, if any for this name...
  KateIndentScript *script = KateGlobal::self()->scriptManager()->indentationScript(name);
  if ( script )
  {
    if (isStyleProvided(script, doc->highlight()))
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
  m_mode = MODE_NORMAL;
}

void KateAutoIndent::checkRequiredStyle()
{
  if (m_script)
  {
    if (!isStyleProvided(m_script, doc->highlight()))
    {
      kDebug( 13060 ) << "mode" << m_mode << "requires a different highlight style";
      doc->config()->setIndentationMode(MODE_NORMAL);
    }
  }
}

void KateAutoIndent::updateConfig ()
{
  KateDocumentConfig *config = doc->config();

  useSpaces   = config->replaceTabsDyn();
  keepExtra   = config->keepExtraSpaces();
  tabWidth    = config->tabWidth();
  indentWidth = config->indentationWidth();
}


bool KateAutoIndent::changeIndent (const KTextEditor::Range &range, int change)
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

    doIndentRelative(line, change * indentWidth);
  }

  if (skippedLines.count() > range.numberOfLines())
  {
    // all lines were empty, so indent them nevertheless
    foreach (int line, skippedLines)
      doIndentRelative(line, change * indentWidth);
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
  if (m_mode == MODE_NORMAL)
  {
    // only indent on new line, per default
    if (typedChar != '\n')
      return;

    // keep indent of previous line
    keepIndent (position.line());

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
  actionGroup = new QActionGroup(menu());
}

void KateViewIndentationAction::slotAboutToShow()
{
  QStringList modes = KateAutoIndent::listModes ();

  menu()->clear ();
  foreach (QAction *action, actionGroup->actions()) {
    actionGroup->removeAction(action);
  }
  for (int z=0; z<modes.size(); ++z) {
    QAction *action = menu()->addAction( '&' + KateAutoIndent::modeDescription(z).replace('&', "&&") );
    actionGroup->addAction(action);
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
