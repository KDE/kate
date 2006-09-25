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
  : QObject(), doc(_doc)
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

void KateAutoIndent::fullIndent ( KateView *view, int line, int indentation )
{
  kDebug () << "fullIndent: line: " << line << " level: " << indentation << endl;

  KateTextLine::Ptr textline = doc->plainKateTextLine(line);

  // textline not found, cu
  if (!textline)
    return;

  if (indentation < 0)
    indentation = 0;

  int first_char = textline->firstChar();

  if (first_char < 0)
    first_char = textline->length();

  doc->editStart (view);
  
  // remove the trailing spaces
  doc->editRemoveText (line, 0, first_char);

  // insert replacement stuff..
  doc->editInsertText (line, 0, tabString (indentation));

  doc->editEnd ();
}

void KateAutoIndent::changeIndent ( KateView *view, int line, int change )
{
  // no change, no work...
  if (change == 0)
    return;

  KateTextLine::Ptr textline = doc->plainKateTextLine(line);

  // textline not found, cu
  if (!textline)
    return;

  // reindent...
  fullIndent (view, line, textline->indentDepth(tabWidth) + change * indentWidth);
}
//END KateAutoIndent

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

//BEGIN KateNormalIndent

KateNormalIndent::KateNormalIndent (KateDocument *_doc)
 : KateAutoIndent (_doc)
{
  connect(_doc, SIGNAL(highlightingChanged()), this, SLOT(updateConfig()));
}

KateNormalIndent::~KateNormalIndent ()
{
}

void KateNormalIndent::userWrappedLine (KateView *view, const KTextEditor::Cursor &position)
{
  kDebug () << "MUHHHHHHHHHH" << endl;

 // no change, no work...
  if (position.line() <= 0)
  {
    fullIndent (view, position.line(), 0);
  }

  KateTextLine::Ptr textline = doc->plainKateTextLine(position.line()-1);

  // textline not found, cu
  if (!textline)
    return;

  doc->editStart (view);

  // insert the new initial indentation....
  doc->editInsertText (position.line(), 0, tabString (textline->indentDepth (tabWidth)));

  doc->editEnd ();
}

# if 0
bool KateNormalIndent::isBalanced (KateDocCursor &begin, const KateDocCursor &end, QChar open, QChar close, uint &pos) const
{
  int parenOpen = 0;
  bool atLeastOne = false;
  bool getNext = false;

  pos = doc->plainKateTextLine(begin.line())->firstChar();

  // Iterate one-by-one finding opening and closing chars
  // Assume that open and close are 'Symbol' characters
  while (begin < end)
  {
    QChar c = begin.currentChar();
    if (begin.currentAttrib() == symbolAttrib)
    {
      if (c == open)
      {
        if (!atLeastOne)
        {
          atLeastOne = true;
          getNext = true;
          pos = measureIndent(begin) + 1;
        }
        parenOpen++;
      }
      else if (c == close)
      {
        parenOpen--;
      }
    }
    else if (getNext && !c.isSpace())
    {
      getNext = false;
      pos = measureIndent(begin);
    }

    if (atLeastOne && parenOpen <= 0)
      return true;

    begin.moveForward(1);
  }

  return (atLeastOne) ? false : true;
}

bool KateNormalIndent::skipBlanks (KateDocCursor &cur, KateDocCursor &max, bool newline) const
{
  int curLine = cur.line();
  if (newline)
    cur.moveForward(1);

  if (cur >= max)
    return false;

  do
  {
    uchar attrib = cur.currentAttrib();
    const QString hlFile = doc->highlight()->hlKeyForAttrib( attrib );

    if (attrib != commentAttrib && attrib != regionAttrib && attrib != alertAttrib && !hlFile.endsWith("doxygen.xml"))
    {
      QChar c = cur.currentChar();
      if (!c.isNull() && !c.isSpace())
        break;
    }

    if (!cur.moveForward(1))
    {
      cur = max;
      break;
    }
    // Make sure col is 0 if we spill into next line  i.e. count the '\n' as a character
    if (curLine != cur.line())
    {
      if (!newline)
        break;
      curLine = cur.line();
      cur.setColumn(0);
    }
  } while (cur < max);

  if (cur > max)
    cur = max;
  return true;
}

uint KateNormalIndent::measureIndent (KateDocCursor &cur) const
{
  return doc->plainKateTextLine(cur.line())->toVirtualColumn(cur.column(), tabWidth);
}



/*
  Optimize the leading whitespace for a single line.
  If change is > 0, it adds indentation units (indentationChars)
  if change is == 0, it only optimizes
  If change is < 0, it removes indentation units
  This will be used to indent, unindent, and optimal-fill a line.
  If excess space is removed depends on the flag cfKeepExtraSpaces
  which has to be set by the user
*/
void KateNormalIndent::optimizeLeadingSpace(uint line, int change)
{
  KateTextLine::Ptr textline = doc->plainKateTextLine(line);

  int first_char = textline->firstChar();

  if (first_char < 0)
    first_char = textline->length();

  int space =  textline->toVirtualColumn(first_char, tabWidth) + change * indentWidth;
  if (space < 0)
    space = 0;

  if (!keepExtra)
  {
    uint extra = space % indentWidth;

    space -= extra;
    if (extra && change < 0) {
      // otherwise it unindents too much (e.g. 12 chars when indentation is 8 chars wide)
      space += indentWidth;
    }
  }

  QString new_space = tabString(space);
  int length = new_space.length();

  int change_from;
  for (change_from = 0; change_from < first_char && change_from < length; change_from++) {
    if (textline->at(change_from) != new_space[change_from])
      break;
  }

  if (change_from < first_char)
    doc->removeText(KTextEditor::Range(line, change_from, line, first_char));

  if (change_from < length)
    doc->insertText(KTextEditor::Cursor(line, change_from), new_space.right(length - change_from));
}
#endif
void KateNormalIndent::processNewline (KateView *view, KateDocCursor &begin, bool /*needContinue*/)
{
#if 0
  int line = begin.line() - 1;
  int pos = begin.column();

  while ((line > 0) && (pos < 0)) // search a not empty text line
    pos = doc->plainKateTextLine(--line)->firstChar();

  begin.setColumn(0);

  if (pos > 0)
  {
    QString filler = doc->text(KTextEditor::Range(line, 0, line, pos));
    doc->insertText(begin, filler);
    begin.setColumn(filler.length());
  }
#endif
}

void KateNormalIndent::indent ( KateView *v, uint line, int change)
{
#if 0
  if (!v->selection())
  {
    // single line
    optimizeLeadingSpace(line, change);
  }
  else
  {
    int sl = v->selectionRange().start().line();
    int el = v->selectionRange().end().line();
    int ec = v->selectionRange().end().column();

    if ((ec == 0) && ((el-1) >= 0))
    {
      el--; /* */
    }

    if (keepProfile && change < 0) {
      // unindent so that the existing indent profile doesn't get screwed
      // if any line we may unindent is already full left, don't do anything
      int adjustedChange = -change;

      for (line = sl; (int) line <= el && adjustedChange > 0; line++) {
        KateTextLine::Ptr textLine = doc->plainKateTextLine(line);
        int firstChar = textLine->firstChar();
        if (firstChar >= 0 && (v->lineSelected(line) || v->lineHasSelected(line))) {
          int maxUnindent = textLine->toVirtualColumn(firstChar, tabWidth) / indentWidth;
          if (maxUnindent < adjustedChange)
            adjustedChange = maxUnindent;
        }
      }

      change = -adjustedChange;
    }

    for (line = sl; (int) line <= el; line++) {
      if (v->lineSelected(line) || v->lineHasSelected(line)) {
        optimizeLeadingSpace(line, change);
      }
    }
  }
#endif
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
    KateNormalIndent::indent(view, line, levels);
  }
}

// TODO: return sth. like m_script->internalName(); (which is the filename)
QString KateScriptIndent::modeName () { return m_script->internalName (); }
//END KateScriptIndent

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
