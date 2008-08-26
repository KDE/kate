#include "katevivisualmode.h"
#include "katesmartrange.h"
#include "katevirange.h"

KateViVisualMode::KateViVisualMode( KateView *view, KateViewInternal *viewInternal )
  : KateViNormalMode( view, viewInternal )
{
  m_start.setPosition( -1, -1 );
  m_topRange = m_view->doc()->newSmartRange(m_view->doc()->documentRange());
  static_cast<KateSmartRange*>(m_topRange)->setInternal();
  m_topRange->setInsertBehavior(KTextEditor::SmartRange::ExpandLeft | KTextEditor::SmartRange::ExpandRight);

  m_view->addInternalHighlight(m_topRange);

  m_visualLine = false;

  KTextEditor::Range r;
  highlightRange = m_view->doc()->newSmartRange( r, m_topRange );
  attribute = KTextEditor::Attribute::Ptr(new KTextEditor::Attribute());
  attribute->setBackground( m_viewInternal->palette().highlight() );
  attribute->setForeground( m_viewInternal->palette().highlightedText() );
  highlightRange->setInsertBehavior(KTextEditor::SmartRange::DoNotExpand);

  initializeCommands();
}

KateViVisualMode::~KateViVisualMode()
{
}

void KateViVisualMode::highlight() const
{
  // FIXME: HACK to avoid highlight bug: remove highlighing and re-set it
  highlightRange->setAttribute(static_cast<KTextEditor::Attribute::Ptr>(0));
  highlightRange->setAttribute(attribute);

  KTextEditor::Cursor c1 = m_start;
  KTextEditor::Cursor c2 = m_view->cursorPosition();

  if ( m_visualLine ) {
      c1.setColumn( ( c1 < c2 ) ? 0 : getLine( m_start.line() ).length() );
      c2.setColumn( ( c1 < c2  ? getLine().length() : 0 ) );
  } else if ( c1 > c2 && c1.column() != 0 ) {
    c1.setColumn( c1.column()+1 );
  }

  highlightRange->setRange( KTextEditor::Range( c1, c2 ) );
}

void KateViVisualMode::goToPos( KateViRange r )
{
  KTextEditor::Cursor cursor = m_view->cursorPosition();

  if ( r.startLine != -1 && r.startColumn != -1 && cursor < m_start ) {
    cursor.setLine( r.startLine );
    cursor.setColumn( r.startColumn );
  } else {
    cursor.setLine( r.endLine );
    cursor.setColumn( r.endColumn );
  }

  m_viewInternal->updateCursor( cursor );

  m_commandRange.startLine = m_start.line();
  m_commandRange.startColumn = m_start.column();
  m_commandRange.endLine = r.endLine;
  m_commandRange.endColumn = r.endColumn;

  highlight();
}

void KateViVisualMode::reset()
{
    // remove highlighting
    highlightRange->setAttribute(static_cast<KTextEditor::Attribute::Ptr>(0));

    m_awaitingMotionOrTextObject.push_back( 0 ); // search for text objects/motion from char 0

    m_visualLine = false;

    // only switch to normal mode if still in visual mode. commands like c, s, ...
    // can have switched to insert mode
    if ( m_view->getCurrentViMode() == VisualMode || m_view->getCurrentViMode() == VisualLineMode ) {
        m_view->viEnterNormalMode();
    }
}

void KateViVisualMode::init()
{
    m_start = m_view->cursorPosition();
    highlightRange->setRange( KTextEditor::Range( m_start, m_view->cursorPosition() ) );
    highlightRange->setAttribute(attribute);
    highlight();

    m_awaitingMotionOrTextObject.push_back( 0 ); // search for text objects/motion from char 0
}


void KateViVisualMode::setVisualLine( bool l )
{
  m_visualLine = l;
  highlight();
}

void KateViVisualMode::initializeCommands()
{
  m_commands.clear();
  m_motions.clear();
  m_commands.push_back( new KateViCommand( this, "J", &KateViNormalMode::commandJoinLines, false ) );
  m_commands.push_back( new KateViCommand( this, "c", &KateViNormalMode::commandChange, false ) );
  m_commands.push_back( new KateViCommand( this, "C", &KateViNormalMode::commandChangeToEOL, false ) );
  m_commands.push_back( new KateViCommand( this, "d", &KateViNormalMode::commandDelete, false ) );
  m_commands.push_back( new KateViCommand( this, "D", &KateViNormalMode::commandDeleteToEOL, false ) );
  m_commands.push_back( new KateViCommand( this, "x", &KateViNormalMode::commandDeleteChar, false ) );
  m_commands.push_back( new KateViCommand( this, "X", &KateViNormalMode::commandDeleteCharBackward, false ) );
  m_commands.push_back( new KateViCommand( this, "gq", &KateViNormalMode::commandFormatLines, false ) );
  m_commands.push_back( new KateViCommand( this, "gu", &KateViNormalMode::commandMakeLowercase, false ) );
  m_commands.push_back( new KateViCommand( this, "gU", &KateViNormalMode::commandMakeUppercase, false ) );
  m_commands.push_back( new KateViCommand( this, "y", &KateViNormalMode::commandYank, false ) );
  m_commands.push_back( new KateViCommand( this, "Y", &KateViNormalMode::commandYankToEOL, false ) );
  m_commands.push_back( new KateViCommand( this, "p", &KateViNormalMode::commandPaste, false ) );
  m_commands.push_back( new KateViCommand( this, "P", &KateViNormalMode::commandPasteBefore, false ) );
  m_commands.push_back( new KateViCommand( this, "r.", &KateViNormalMode::commandReplaceCharacter, true ) );
  m_commands.push_back( new KateViCommand( this, ":", &KateViNormalMode::commandSwitchToCmdLine, false ) );
  m_commands.push_back( new KateViCommand( this, "/", &KateViNormalMode::commandSearch, false ) );
  m_commands.push_back( new KateViCommand( this, "u", &KateViNormalMode::commandUndo, false ) );
  m_commands.push_back( new KateViCommand( this, "U", &KateViNormalMode::commandRedo, false ) );
  m_commands.push_back( new KateViCommand( this, "m.", &KateViNormalMode::commandSetMark, true ) );
  m_commands.push_back( new KateViCommand( this, "n", &KateViNormalMode::commandFindNext, false ) );
  m_commands.push_back( new KateViCommand( this, "N", &KateViNormalMode::commandFindPrev, false ) );
  m_commands.push_back( new KateViCommand( this, ">", &KateViNormalMode::commandIndentLines, false ) );
  m_commands.push_back( new KateViCommand( this, "<", &KateViNormalMode::commandUnindentLines, false ) );
  m_commands.push_back( new KateViCommand( this, "<c-c>", &KateViNormalMode::commandAbort, false ) );
  m_commands.push_back( new KateViCommand( this, "ga", &KateViNormalMode::commandPrintCharacterCode, false, false, false ) );
  m_commands.push_back( new KateViCommand( this, "v", &KateViNormalMode::commandEnterVisualMode, false, false, false ) );
  m_commands.push_back( new KateViCommand( this, "V", &KateViNormalMode::commandEnterVisualLineMode, false, false, false ) );

  // regular motions
  m_motions.push_back( new KateViMotion( this, "h", &KateViNormalMode::motionLeft ) );
  m_motions.push_back( new KateViMotion( this, "<left>", &KateViNormalMode::motionLeft ) );
  m_motions.push_back( new KateViMotion( this, "<backspace>", &KateViNormalMode::motionLeft ) );
  m_motions.push_back( new KateViMotion( this, "j", &KateViNormalMode::motionDown ) );
  m_motions.push_back( new KateViMotion( this, "<down>", &KateViNormalMode::motionDown ) );
  m_motions.push_back( new KateViMotion( this, "k", &KateViNormalMode::motionUp ) );
  m_motions.push_back( new KateViMotion( this, "<up>", &KateViNormalMode::motionUp ) );
  m_motions.push_back( new KateViMotion( this, "l", &KateViNormalMode::motionRight ) );
  m_motions.push_back( new KateViMotion( this, "<right>", &KateViNormalMode::motionRight ) );
  m_motions.push_back( new KateViMotion( this, " ", &KateViNormalMode::motionRight ) );
  m_motions.push_back( new KateViMotion( this, "$", &KateViNormalMode::motionToEOL ) );
  m_motions.push_back( new KateViMotion( this, "<end>", &KateViNormalMode::motionToEOL ) );
  m_motions.push_back( new KateViMotion( this, "0", &KateViNormalMode::motionToColumn0 ) );
  m_motions.push_back( new KateViMotion( this, "<home>", &KateViNormalMode::motionToColumn0 ) );
  m_motions.push_back( new KateViMotion( this, "^", &KateViNormalMode::motionToFirstCharacterOfLine ) );
  m_motions.push_back( new KateViMotion( this, "f.", &KateViNormalMode::motionFindChar, true ) );
  m_motions.push_back( new KateViMotion( this, "F.", &KateViNormalMode::motionFindCharBackward, true ) );
  m_motions.push_back( new KateViMotion( this, "t.", &KateViNormalMode::motionToChar, true ) );
  m_motions.push_back( new KateViMotion( this, "T.", &KateViNormalMode::motionToCharBackward, true ) );
  m_motions.push_back( new KateViMotion( this, "gg", &KateViNormalMode::motionToLineFirst ) );
  m_motions.push_back( new KateViMotion( this, "G", &KateViNormalMode::motionToLineLast ) );
  m_motions.push_back( new KateViMotion( this, "w", &KateViNormalMode::motionWordForward ) );
  m_motions.push_back( new KateViMotion( this, "W", &KateViNormalMode::motionWORDForward ) );
  m_motions.push_back( new KateViMotion( this, "b", &KateViNormalMode::motionWordBackward ) );
  m_motions.push_back( new KateViMotion( this, "B", &KateViNormalMode::motionWORDBackward ) );
  m_motions.push_back( new KateViMotion( this, "e", &KateViNormalMode::motionToEndOfWord ) );
  m_motions.push_back( new KateViMotion( this, "E", &KateViNormalMode::motionToEndOfWORD ) );
  m_motions.push_back( new KateViMotion( this, "|", &KateViNormalMode::motionToScreenColumn ) );
  m_motions.push_back( new KateViMotion( this, "%", &KateViNormalMode::motionToMatchingItem ) );
  m_motions.push_back( new KateViMotion( this, "`.", &KateViNormalMode::motionToMark, true ) );
  m_motions.push_back( new KateViMotion( this, "'.", &KateViNormalMode::motionToMarkLine, true ) );
  m_motions.push_back( new KateViMotion( this, "[[", &KateViNormalMode::motionToPreviousBraceBlockStart ) );
  m_motions.push_back( new KateViMotion( this, "]]", &KateViNormalMode::motionToNextBraceBlockStart ) );
  m_motions.push_back( new KateViMotion( this, "[]", &KateViNormalMode::motionToPreviousBraceBlockEnd ) );
  m_motions.push_back( new KateViMotion( this, "][", &KateViNormalMode::motionToNextBraceBlockEnd ) );

  // text objects
  m_motions.push_back( new KateViMotion( this, "iw", &KateViNormalMode::textObjectInnerWord ) );
  m_motions.push_back( new KateViMotion( this, "aw", &KateViNormalMode::textObjectAWord ) );
  m_motions.push_back( new KateViMotion( this, "i\"", &KateViNormalMode::textObjectInnerQuoteDouble ) );
  m_motions.push_back( new KateViMotion( this, "a\"", &KateViNormalMode::textObjectAQuoteDouble ) );
  m_motions.push_back( new KateViMotion( this, "i'", &KateViNormalMode::textObjectInnerQuoteSingle ) );
  m_motions.push_back( new KateViMotion( this, "a'", &KateViNormalMode::textObjectAQuoteSingle ) );
  m_motions.push_back( new KateViMotion( this, "i[()]", &KateViNormalMode::textObjectInnerParen, true ) );
  m_motions.push_back( new KateViMotion( this, "a[()]", &KateViNormalMode::textObjectAParen, true ) );
  m_motions.push_back( new KateViMotion( this, "i[\\[\\]]", &KateViNormalMode::textObjectInnerBracket, true ) );
  m_motions.push_back( new KateViMotion( this, "a[\\[\\]]", &KateViNormalMode::textObjectABracket, true ) );
}
