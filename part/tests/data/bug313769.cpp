/* This file is part of the KDE libraries
   Copyright (C) 1999 Jochen Wilhelmy <digisnap@cs.tu-berlin.de>
*/
//BEGIN includes
#include "kateview.h"
#include "kateview.moc"

//#define VIEW_RANGE_DEBUG

//END includes

void KateView::blockFix(KTextEditor::Range& range)
{
  if (range.start().column() > range.end().column())
  {
    int tmp = range.start().column();
    range.start().setColumn(range.end().column());
    range.end().setColumn(tmp);
  }
}

KateView::KateView( KateDocument *doc, QWidget *parent )
{
  setComponentData ( KateGlobal::self()->componentData () );

  // selection if for this view only and will invalidate if becoming empty
  m_selection.setView (this);

  // use z depth defined in moving ranges interface
  m_selection.setZDepth (-100000.0);

  KateGlobal::self()->registerView( this );

  KTextEditor::ViewBarContainer *viewBarContainer=qobject_cast<KTextEditor::ViewBarContainer*>( KateGlobal::self()->container() );
  QWidget *bottomBarParent=viewBarContainer?viewBarContainer->getViewBarParent(this,KTextEditor::ViewBarContainer::BottomBar):0;
  QWidget *topBarParent=viewBarContainer?viewBarContainer->getViewBarParent(this,KTextEditor::ViewBarContainer::TopBar):0;

  m_bottomViewBar=new KateViewBar (bottomBarParent!=0,KTextEditor::ViewBarContainer::BottomBar,bottomBarParent?bottomBarParent:this,this);
  m_topViewBar=new KateViewBar (topBarParent!=0,KTextEditor::ViewBarContainer::TopBar,topBarParent?topBarParent:this,this);

  // ugly workaround:
  // Force the layout to be left-to-right even on RTL deskstop, as discussed
  // on the mailing list. This will cause the lines and icons panel to be on
  // the left, even for Arabic/Hebrew/Farsi/whatever users.
  setLayoutDirection ( Qt::LeftToRight );

  // layouting ;)
  m_vBox = new QVBoxLayout (this);
  m_vBox->setMargin (0);
  m_vBox->setSpacing (0);

  // add top viewbar...
  if (topBarParent)
    viewBarContainer->addViewBarToLayout(this,m_topViewBar,KTextEditor::ViewBarContainer::TopBar);
  else
    m_vBox->addWidget(m_topViewBar);

  m_bottomViewBar->installEventFilter(m_viewInternal);

  // add KateMessageWidget for KTE::MessageInterface immediately above view
  m_topMessageWidget = new KateMessageWidget(this);
  m_vBox->addWidget(m_topMessageWidget);
  m_topMessageWidget->hide();

  // add hbox: KateIconBorder | KateViewInternal | KateScrollBar
  QHBoxLayout *hbox = new QHBoxLayout ();
  m_vBox->addLayout (hbox, 100);
  hbox->setMargin (0);
  hbox->setSpacing (0);

  QStyleOption option;
  option.initFrom(this);

  if (style()->styleHint(QStyle::SH_ScrollView_FrameOnlyAroundContents, &option, this)) {
