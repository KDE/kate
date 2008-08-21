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
  attribute->setBackground(QColor(0xdd, 0xdd, 0xdd)); // FIXME: don't use hard coded colour
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
      c2.setColumn( ( c1 < c2 ) ? getLine().length() : 0 );
  }

  highlightRange->setRange( KTextEditor::Range( c1, c2 ) );
}

void KateViVisualMode::goToPos( KateViRange r )
{
  KTextEditor::Cursor cursor;

  cursor.setLine( r.endLine );
  cursor.setColumn( r.endColumn );

  m_viewInternal->updateCursor( cursor );

  if ( r.startLine != -1 && r.startColumn != -1 ) {
      m_start.setLine( r.startLine );
      m_start.setColumn( r.startColumn );
  }

  m_commandRange.startLine = m_start.line();
  m_commandRange.startColumn = m_start.column();
  m_commandRange.endLine = r.endLine;
  m_commandRange.endColumn = r.endColumn;

  highlight();
}

void KateViVisualMode::abort()
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

void KateViVisualMode::initializeCommands()
{
    m_commands.clear();
    m_motions.clear();
  //m_commands.push_back( new KateViNormalModeCommand( this, "a", &KateViNormalMode::commandEnterInsertModeAppend, false ) );
  //m_commands.push_back( new KateViNormalModeCommand( this, "A", &KateViNormalMode::commandEnterInsertModeAppendEOL, false ) );
  //m_commands.push_back( new KateViNormalModeCommand( this, "i", &KateViNormalMode::commandEnterInsertMode, false ) );
  //m_commands.push_back( new KateViNormalModeCommand( this, "v", &KateViNormalMode::commandEnterVisualMode, false ) );
  //m_commands.push_back( new KateViNormalModeCommand( this, "o", &KateViNormalMode::commandOpenNewLineUnder, false ) );
  //m_commands.push_back( new KateViNormalModeCommand( this, "O", &KateViNormalMode::commandOpenNewLineOver, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "J", &KateViNormalMode::commandJoinLines, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "c", &KateViNormalMode::commandChange, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "C", &KateViNormalMode::commandChangeToEOL, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "d", &KateViNormalMode::commandDelete, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "D", &KateViNormalMode::commandDeleteToEOL, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "x", &KateViNormalMode::commandDeleteChar, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "X", &KateViNormalMode::commandDeleteCharBackward, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "gq", &KateViNormalMode::commandFormatLines, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "gu", &KateViNormalMode::commandMakeLowercase, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "gU", &KateViNormalMode::commandMakeUppercase, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "y", &KateViNormalMode::commandYank, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "Y", &KateViNormalMode::commandYankToEOL, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "p", &KateViNormalMode::commandPaste, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "P", &KateViNormalMode::commandPasteBefore, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "r.", &KateViNormalMode::commandReplaceCharacter, true ) );
  m_commands.push_back( new KateViNormalModeCommand( this, ":", &KateViNormalMode::commandSwitchToCmdLine, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "/", &KateViNormalMode::commandSearch, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "u", &KateViNormalMode::commandUndo, false ) );
  //m_commands.push_back( new KateViNormalModeCommand( this, "<c-r>", &KateViNormalMode::commandRedo, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "U", &KateViNormalMode::commandRedo, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "m.", &KateViNormalMode::commandSetMark, true ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "n", &KateViNormalMode::commandFindNext, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "N", &KateViNormalMode::commandFindPrev, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, ">", &KateViNormalMode::commandIndentLines, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "<", &KateViNormalMode::commandUnindentLines, false ) );
  m_commands.push_back( new KateViNormalModeCommand( this, "<c-c>", &KateViNormalMode::commandAbort, false ) );

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
