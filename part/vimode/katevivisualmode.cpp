#include "katevivisualmode.h"
#include "katesmartrange.h"
#include "katevirange.h"

KateViVisualMode::KateViVisualMode( KateView *view, KateViewInternal *viewInternal )
  : KateViNormalMode( view, viewInternal )
{
  start.setPosition( -1, -1 );
  m_topRange = m_view->doc()->newSmartRange(m_view->doc()->documentRange());
  static_cast<KateSmartRange*>(m_topRange)->setInternal();
  m_topRange->setInsertBehavior(KTextEditor::SmartRange::ExpandLeft | KTextEditor::SmartRange::ExpandRight);

  m_view->addInternalHighlight(m_topRange);

  // Retrieve the SmartInterface
  KTextEditor::SmartInterface* smart = qobject_cast<KTextEditor::SmartInterface*>( m_view->doc() );

  KTextEditor::Range r;
  highlightRange = m_view->doc()->newSmartRange( r, m_topRange );
  attribute = KTextEditor::Attribute::Ptr(new KTextEditor::Attribute());
  attribute->setBackground(QColor(0xdd, 0xdd, 0xdd)); // FIXME: don't use hard coded colour
  highlightRange->setInsertBehavior(KTextEditor::SmartRange::DoNotExpand);
}

KateViVisualMode::~KateViVisualMode()
{
}

void KateViVisualMode::highlight()
{
  // FIXME: HACK to avoid highlight bug - remove highlighing and re-set it
  highlightRange->setAttribute(static_cast<KTextEditor::Attribute::Ptr>(0));
  highlightRange->setAttribute(attribute);

  highlightRange->setRange( KTextEditor::Range( start, m_view->cursorPosition() ) );
}

void KateViVisualMode::goToPos( KateViRange r )
{
  KTextEditor::Cursor cursor;
  cursor.setLine( r.endLine );
  cursor.setColumn( r.endColumn );

  m_viewInternal->updateCursor( cursor );
  highlight();
}

void KateViVisualMode::esc()
{
    // remove highlighting
    highlightRange->setAttribute(static_cast<KTextEditor::Attribute::Ptr>(0));
    m_view->viEnterNormalMode();
}

void KateViVisualMode::init()
{
    start = m_view->cursorPosition();
    highlightRange->setAttribute(attribute);
}
