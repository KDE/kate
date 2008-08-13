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

  KTextEditor::Range r( 0,2, 4, 40 );
  highlightRange = m_view->doc()->newSmartRange( r, m_topRange );
  attribute = KTextEditor::Attribute::Ptr(new KTextEditor::Attribute());
}

KateViVisualMode::~KateViVisualMode()
{
}

void KateViVisualMode::highlight()
{
  highlightRange->setInsertBehavior(KTextEditor::SmartRange::DoNotExpand);
  attribute->setBackground(QColor(0xdd, 0xdd, 0xdd));
  highlightRange->setAttribute(attribute);
}

//bool KateViVisualMode::handleKeypress( QKeyEvent *e )
//{
//  int keyCode = e->key();
//  QString text = e->text();
//
//  // ignore modifier keys alone
//  if ( keyCode == Qt::Key_Shift || keyCode == Qt::Key_Control
//      || keyCode == Qt::Key_Alt || keyCode == Qt::Key_Meta ) {
//    return false;
//  }
//
//  if ( keyCode == Qt::Key_H ) {
//    kDebug( 13070 ) << "                    H";
//    m_viewInternal->cursorLeft();
//    highlight();
//  }
//  else if ( keyCode == Qt::Key_L ) {
//    kDebug( 13070 ) << "                    L";
//    m_viewInternal->cursorRight();
//    highlight();
//  }
//
//  if ( keyCode == Qt::Key_Escape ) {
//    m_view->viEnterNormalMode();
//    return true;
//  }
//
//  return false;
//}

void KateViVisualMode::goToPos( KateViRange r )
{
}
