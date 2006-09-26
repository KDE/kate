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

KateAutoIndent *KateAutoIndent::createIndenter (KateDocument *doc, const QString &name)
{
  if ( name == QString ("normal") )
    return new KateNormalIndent (doc);

  // handle script indenters
  KateIndentJScript *script = KateGlobal::self()->indentScriptManager()->script(name);
  if ( script )
    return new KateScriptIndent ( script, doc );

  // none
  return new KateAutoIndent (doc);
}

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

  return KateGlobal::self()->indentScriptManager()->scriptByIndex(mode-2)->internalName ();
}

QString KateAutoIndent::modeDescription (int mode)
{
  if (mode == 0 || mode >= modeCount ())
    return i18n ("None");

  if (mode == 1)
    return i18n ("Normal");

  return KateGlobal::self()->indentScriptManager()->scriptByIndex(mode-2)->niceName ();
}

uint KateAutoIndent::modeNumber (const QString &name)
{
  for (int i = 0; i < modeCount(); ++i)
    if (modeName(i) == name)
      return i;

  return 0;
}

bool KateAutoIndent::hasConfigPage (int /*mode*/)
{
//  if ( mode == KateDocumentConfig::imScriptIndent )
//    return true;

  return false;
}

IndenterConfigPage* KateAutoIndent::configPage(QWidget * /*parent*/, int /*mode*/)
{
//  if ( mode == KateDocumentConfig::imScriptIndent )
//    return new ScriptIndentConfigPage(parent, "script_indent_config_page");

  return 0;
}

KateAutoIndent::KateAutoIndent (KateDocument *_doc)
  : doc(_doc)
{
}

KateAutoIndent::~KateAutoIndent ()
{
}

void KateAutoIndent::updateConfig ()
{
  KateDocumentConfig *config = doc->config();

  useSpaces   = config->configFlags() & KateDocumentConfig::cfReplaceTabsDyn;
  keepProfile = config->configFlags() & KateDocumentConfig::cfKeepIndentProfile;
  keepExtra   = config->configFlags() & KateDocumentConfig::cfKeepExtraSpaces;
  tabWidth    = config->tabWidth();
  indentWidth = config->indentationWidth();
}

bool KateAutoIndent::changeIndent (KateView *view, const KTextEditor::Range &range, int change)
{
  // loop over all lines given...
  if (keepProfile && change < 0)
  {
    for (int line = range.start().line () < 0 ? 0 : range.start().line (); line <= qMin (range.end().line (), doc->lines()-1); ++line)
    {
      KateTextLine::Ptr textline = doc->plainKateTextLine(line);
  
      // textline not found, cu
      if (!textline)
        return false;
  
      // get indent width of current line
      int currentIndentInSpaces = textline->indentDepth (tabWidth);
  
      // oh oh, too less indent....
      if (currentIndentInSpaces < (indentWidth * (-change)))
      {
        kDebug () << "oh oh, can't unindent" << endl;
        return false;
      }
    }
  }

  // loop over all lines given...
  for (int line = range.start().line () < 0 ? 0 : range.start().line (); line <= qMin (range.end().line (), doc->lines()-1); ++line)
  {
    doIndent (view, line, change, true, keepExtra);
  }

  return true;
}

QString KateAutoIndent::tabString (int length) const
{
  QString s;
  length = qMin (length, 256); // sanity check for large values of pos

  if (!useSpaces)
  {
    while (length >= tabWidth)
    {
      s += '\t';
      length -= tabWidth;
    }
  }
  while (length > 0)
  {
    s += ' ';
    length--;
  }
  return s;
}

bool KateAutoIndent::doIndent ( KateView *view, int line, int change, bool relative, bool keepExtraSpaces )
{
  kDebug () << "doIndent: line: " << line << " change: " << change << " relative: " << relative << endl;

  KateTextLine::Ptr textline = doc->plainKateTextLine(line);

  // textline not found, cu
  if (!textline)
    return false;

  int indentlevel = change;
  int spacesToKeep = 0;

  // get indent width of current line
  int currentIndentInSpaces = (relative || keepExtraSpaces) ? textline->indentDepth (tabWidth) : 0;

  // calc the spaces to keep
  if (keepExtraSpaces)
    spacesToKeep = currentIndentInSpaces % indentWidth;

  // adjust indent level
  if (relative)
    indentlevel += currentIndentInSpaces / indentWidth;

  if (indentlevel < 0)
    indentlevel = 0;

  QString indentString = tabString (indentlevel * indentWidth);

  if (spacesToKeep > 0)
    indentString.append (QString (spacesToKeep, ' '));

  int first_char = textline->firstChar();

  if (first_char < 0)
    first_char = textline->length();

  doc->editStart (view);
  
  // remove the trailing spaces
  doc->editRemoveText (line, 0, first_char);

  // insert replacement stuff..
  doc->editInsertText (line, 0, indentString);

  doc->editEnd ();

  return true;
}
//END KateAutoIndent

//BEGIN KateNormalIndent
KateNormalIndent::KateNormalIndent (KateDocument *_doc) : KateAutoIndent (_doc)
{
}

void KateNormalIndent::userWrappedLine (KateView *view, const KTextEditor::Cursor &position)
{
  kDebug () << "MUHHHHHHHHHH" << endl;

 // no change, no work...
  if (position.line() <= 0)
    return;

  KateTextLine::Ptr textline = doc->plainKateTextLine(position.line()-1);

  // textline not found, cu
  if (!textline)
    return;

  doc->editStart (view);

  // insert the new initial indentation....
  doc->editInsertText (position.line(), 0, tabString (textline->indentDepth (tabWidth)));

  doc->editEnd ();
}
//END

//BEGIN KateScriptIndent
KateScriptIndent::KateScriptIndent(KateIndentJScript *script, KateDocument *doc)
  : KateNormalIndent(doc), m_script(script),
    m_canProcessNewLineSet(false), m_canProcessNewLine(false),
    m_canProcessLineSet(false), m_canProcessLine(false),
    m_canProcessIndentSet(false), m_canProcessIndent(false)
{
}

KateScriptIndent::~KateScriptIndent()
{
}

bool KateScriptIndent::canProcessNewLine() const
{
  if (m_canProcessNewLineSet)
    return m_canProcessNewLine;

  m_canProcessNewLine = m_script->canProcessNewLine();
  m_canProcessNewLineSet = true;
  return m_canProcessNewLine;
}

bool KateScriptIndent::canProcessLine() const
{
  if (m_canProcessLineSet)
    return m_canProcessLine;

  m_canProcessLine = m_script->canProcessLine();
  m_canProcessLineSet = true;
  return m_canProcessLine;
}

bool KateScriptIndent::canProcessIndent() const
{
  if (m_canProcessIndentSet)
    return m_canProcessIndent;

  m_canProcessIndent = m_script->canProcessIndent();
  m_canProcessIndentSet = true;
  return m_canProcessIndent;
}

void KateScriptIndent::processNewline( KateView *view, KateDocCursor &begin, bool needContinue )
{
  QString errorMsg;

//   QTime t;
//   t.start();
  if( !m_script->processNewline( view, begin, needContinue , errorMsg ) )
    kDebug(13051) << m_script->filePath() << ":" << endl << errorMsg << endl;
  // set cursor to the position at which the script set the cursor
  begin.setPosition(view->cursorPosition());
//   kDebug(13050) << "ScriptIndent::processNewline - TIME/ms: " << t.elapsed() << endl;
}

void KateScriptIndent::processChar( KateView *view, QChar c )
{
  QString errorMsg;

//   QTime t;
//   t.start();
  if( !m_script->processChar( view, c, errorMsg ) )
    kDebug(13051) << m_script->filePath() << ":" << endl << errorMsg << endl;
//   kDebug(13050) << "ScriptIndent::processChar - TIME in ms: " << t.elapsed() << endl;
}

void KateScriptIndent::processLine (KateView *view, KateDocCursor &line)
{
  QString errorMsg;

  if( !m_script->processLine( view, line, errorMsg ) )
    kDebug(13051) << m_script->filePath() << ":" << endl << errorMsg << endl;
  // set cursor to the position at which the script set the cursor
  line.setPosition(view->cursorPosition());
}

void KateScriptIndent::processSection (KateView *view, const KateDocCursor &begin,
                                       const KateDocCursor &end)
{
  QString errorMsg;

  if( !m_script->processSection( view, begin, end, errorMsg ) )
    kDebug(13051) << m_script->filePath() << ":" << endl << errorMsg << endl;
}

void KateScriptIndent::indent( KateView *view, uint line, int levels )
{
  if (canProcessIndent()) {
    QString errorMsg;
    if( !m_script->processIndent( view, line, levels, errorMsg ) )
      kDebug(13051) << m_script->filePath() << ":" << endl << errorMsg << endl;
  } else {
    //KateNormalIndent::indent(view, line, levels);
  }
}

// TODO: return sth. like m_script->internalName(); (which is the filename)
QString KateScriptIndent::modeName () { return m_script->internalName (); }
//END KateScriptIndent

//BEGIN KateViewIndentAction
KateViewIndentationAction::KateViewIndentationAction(KateDocument *_doc, const QString& text, KActionCollection* parent, const char* name)
       : KActionMenu (text, parent, name), doc(_doc)
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

//BEGIN ScriptIndentConfigPage, THIS IS ONLY A TEST! :)
#include <qlabel.h>
ScriptIndentConfigPage::ScriptIndentConfigPage ( QWidget *parent)
    : IndenterConfigPage(parent)
{
  QLabel* hello = new QLabel("Hello world! Dummy for testing purpose.", this);
  hello->show();
}

ScriptIndentConfigPage::~ScriptIndentConfigPage ()
{
}

void ScriptIndentConfigPage::apply ()
{
  kDebug(13030) << "ScriptIndentConfigPagE::apply() was called, save config options now!" << endl;
}

//END ScriptIndentConfigPage

// kate: space-indent on; indent-width 2; replace-tabs on;
