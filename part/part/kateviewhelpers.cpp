/* This file is part of the KDE libraries
   Copyright (C) 2002 John Firebaugh <jfirebaugh@kde.org>
   Copyright (C) 2001 Anders Lund <anders@alweb.dk>
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>

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

#include "kateviewhelpers.h"
#include "kateviewhelpers.moc"

#include "katecmd.h"
#include <ktexteditor/attribute.h>
#include "katecodefoldinghelpers.h"
#include "kateconfig.h"
#include "katedocument.h"
#include "katerenderer.h"
#include "kateview.h"
#include "kateviewinternal.h"
#include "katelayoutcache.h"
#include "katetextlayout.h"

#include <kapplication.h>
#include <kglobalsettings.h>
#include <klocale.h>
#include <knotification.h>
#include <kglobal.h>
#include <kmenu.h>
#include <kiconloader.h>

#include <QtGui/QCursor>
#include <QtGui/QPainter>
#include <QtGui/QStyle>
#include <QtCore/QTimer>
#include <QtCore/QRegExp>
#include <QtCore/QTextCodec>
#include <QtGui/QKeyEvent>
#include <QtGui/QPainterPath>
#include <QtGui/QStyleOption>
#include <QtGui/QPalette>
#include <QtGui/QPen>
#include <QtGui/QBoxLayout>
#include <QtGui/QToolButton>

#include <math.h>

#include <kdebug.h>

#include <QtGui/QWhatsThis>

//BEGIN KateScrollBar
KateScrollBar::KateScrollBar (Qt::Orientation orientation, KateViewInternal* parent)
  : QScrollBar (orientation, parent->m_view)
  , m_middleMouseDown (false)
  , m_view(parent->m_view)
  , m_doc(parent->m_doc)
  , m_viewInternal(parent)
  , m_topMargin(0)
  , m_bottomMargin(0)
  , m_savVisibleLines(0)
  , m_showMarks(false)
{
  connect(this, SIGNAL(valueChanged(int)), this, SLOT(sliderMaybeMoved(int)));
  connect(m_doc, SIGNAL(marksChanged(KTextEditor::Document*)), this, SLOT(marksChanged()));

  styleChange(*style());
}

void KateScrollBar::mousePressEvent(QMouseEvent* e)
{
  if (e->button() == Qt::MidButton)
    m_middleMouseDown = true;

  QScrollBar::mousePressEvent(e);

  redrawMarks();
}

void KateScrollBar::mouseReleaseEvent(QMouseEvent* e)
{
  QScrollBar::mouseReleaseEvent(e);

  m_middleMouseDown = false;

  redrawMarks();
}

void KateScrollBar::mouseMoveEvent(QMouseEvent* e)
{
  QScrollBar::mouseMoveEvent(e);

  if (e->buttons() | Qt::LeftButton)
    redrawMarks();
}

void KateScrollBar::paintEvent(QPaintEvent *e)
{
  QScrollBar::paintEvent(e);

  QPainter painter(this);

  QStyleOptionSlider opt;
  opt.init(this);
  opt.subControls = QStyle::SC_None;
  opt.activeSubControls = QStyle::SC_None;
  opt.orientation = orientation();
  opt.minimum = minimum();
  opt.maximum = maximum();
  opt.sliderPosition = sliderPosition();
  opt.sliderValue = value();
  opt.singleStep = singleStep();
  opt.pageStep = pageStep();

  QRect rect = style()->subControlRect(QStyle::CC_ScrollBar, &opt, QStyle::SC_ScrollBarSlider, this);

  QHashIterator<int, QColor> it = m_lines;
  while (it.hasNext())
  {
    it.next();
    if (it.key() < rect.top() || it.key() > rect.bottom())
    {
      painter.setPen(it.value());
      painter.drawLine(0, it.key(), width(), it.key());
    }
  }
}

void KateScrollBar::resizeEvent(QResizeEvent *e)
{
  QScrollBar::resizeEvent(e);
  recomputeMarksPositions();
}

void KateScrollBar::styleChange(QStyle &s)
{
  QScrollBar::styleChange(s);

  // Calculate height of buttons
  QStyleOptionSlider opt;
  opt.init(this);
  opt.subControls = QStyle::SC_None;
  opt.activeSubControls = QStyle::SC_None;
  opt.orientation = this->orientation();
  opt.minimum = minimum();
  opt.maximum = maximum();
  opt.sliderPosition = sliderPosition();
  opt.sliderValue = value();
  opt.singleStep = singleStep();
  opt.pageStep = pageStep();

  m_topMargin = style()->subControlRect(QStyle::CC_ScrollBar, &opt, QStyle::SC_ScrollBarSubLine, this).height() + 2;
  m_bottomMargin = m_topMargin + style()->subControlRect(QStyle::CC_ScrollBar, &opt, QStyle::SC_ScrollBarAddLine, this).height() + 1;

  recomputeMarksPositions();
}

void KateScrollBar::sliderChange ( SliderChange change )
{
  // call parents implementation
  QScrollBar::sliderChange (change);

  if (change == QAbstractSlider::SliderValueChange)
  {
    redrawMarks();
  }
  else if (change == QAbstractSlider::SliderRangeChange)
  {
    recomputeMarksPositions();
  }
}

void KateScrollBar::marksChanged()
{
  recomputeMarksPositions();
}

void KateScrollBar::redrawMarks()
{
  if (!m_showMarks)
    return;

  update();
}

void KateScrollBar::recomputeMarksPositions()
{
  m_lines.clear();
  m_savVisibleLines = m_doc->visibleLines();

  int realHeight = frameGeometry().height() - m_topMargin - m_bottomMargin;

  const QHash<int, KTextEditor::Mark*> &marks = m_doc->marks();
  KateCodeFoldingTree *tree = m_doc->foldingTree();

  for (QHash<int, KTextEditor::Mark*>::const_iterator i = marks.constBegin(); i != marks.constEnd(); ++i)
  {
    KTextEditor::Mark *mark = i.value();

    uint line = mark->line;

    if (tree)
    {
      KateCodeFoldingNode *node = tree->findNodeForLine(line);

      while (node)
      {
        if (!node->isVisible())
          line = tree->getStartLine(node);
        node = node->getParentNode();
      }
    }

    line = m_doc->getVirtualLine(line);

    double d = (double)line / (m_savVisibleLines - 1);
    m_lines.insert(m_topMargin + (int)(d * realHeight),
                   QColor(KateRendererConfig::global()->lineMarkerColor((KTextEditor::MarkInterface::MarkTypes)mark->type)));
  }

  // with Qt4 we don't have the luxury of painting outside a paint event
  // and a paint event wipes the widget... so just update
  update();
}

void KateScrollBar::sliderMaybeMoved(int value)
{
  if (m_middleMouseDown) {
    // we only need to emit this signal once, as for the following slider
    // movements the signal sliderMoved() is already emitted.
    // Thus, set m_middleMouseDown to false right away.
    m_middleMouseDown = false;
    emit sliderMMBMoved(value);
  }
}
//END


//BEGIN KateCmdLineEditFlagCompletion
/**
 * This class provide completion of flags. It shows a short description of
 * each flag, and flags are appended.
 */
class KateCmdLineEditFlagCompletion : public KCompletion
{
  public:
    KateCmdLineEditFlagCompletion() {;}

    QString makeCompletion( const QString & /*s*/ )
    {
      return QString();
    }

};
//END KateCmdLineEditFlagCompletion

//BEGIN KateCmdLineEdit
KateCmdLine::KateCmdLine (KateView *view, KateViewBar *viewBar)
    : KateViewBarWidget (viewBar)
{
    QVBoxLayout *topLayout = new QVBoxLayout ();
    centralWidget()->setLayout(topLayout);
    topLayout->setMargin(0);
    m_lineEdit = new KateCmdLineEdit (this, view);
    topLayout->addWidget (m_lineEdit);
    
    setFocusProxy (m_lineEdit);
}

KateCmdLine::~KateCmdLine()
{
}

KateCmdLineEdit::KateCmdLineEdit (KateCmdLine *bar, KateView *view)
  : KLineEdit ()
  , m_view (view)
  , m_bar (bar)
  , m_msgMode (false)
  , m_histpos( 0 )
  , m_cmdend( 0 )
  , m_command( 0L )
  , m_oldCompletionObject( 0L )
{
  connect (this, SIGNAL(returnPressed(const QString &)),
           this, SLOT(slotReturnPressed(const QString &)));

  completionObject()->insertItems (KateCmd::self()->commandList());
  setAutoDeleteCompletionObject( false );
}


QString KateCmdLineEdit::helptext( const QPoint & ) const
    {
      QString beg = "<qt background=\"white\"><div><table width=\"100%\"><tr><td bgcolor=\"brown\"><font color=\"white\"><b>Help: <big>";
      QString mid = "</big></b></font></td></tr><tr><td>";
      QString end = "</td></tr></table></div><qt>";

      QString t = text();
      QRegExp re( "\\s*help\\s+(.*)" );
      if ( re.indexIn( t ) > -1 )
      {
        QString s;
        // get help for command
        QString name = re.cap( 1 );
        if ( name == "list" )
        {
          return beg + i18n("Available Commands") + mid
              + KateCmd::self()->commandList().join(" ")
              + i18n("<p>For help on individual commands, do <code>'help &lt;command&gt;'</code></p>")
              + end;
        }
        else if ( ! name.isEmpty() )
        {
          KTextEditor::Command *cmd = KateCmd::self()->queryCommand( name );
          if ( cmd )
          {
            if ( cmd->help( (KTextEditor::View*)parentWidget(), name, s ) )
              return beg + name + mid + s + end;
            else
              return beg + name + mid + i18n("No help for '%1'",  name ) + end;
          }
          else
            return beg + mid + i18n("No such command <b>%1</b>", name) + end;
        }
      }

      return beg + mid + i18n(
          "<p>This is the Katepart <b>command line</b>.<br>"
          "Syntax: <code><b>command [ arguments ]</b></code><br>"
          "For a list of available commands, enter <code><b>help list</b></code><br>"
          "For help for individual commands, enter <code><b>help &lt;command&gt;</b></code></p>")
          + end;
    }



bool KateCmdLineEdit::event(QEvent *e) {
	if (e->type()==QEvent::WhatsThis)
		setWhatsThis(helptext(QPoint()));
	return KLineEdit::event(e);
}

void KateCmdLineEdit::slotReturnPressed ( const QString& text )
{
  if (text.isEmpty()) return;
  // silently ignore leading space
  uint n = 0;
  const uint textlen=text.length();
  while( (n<textlen) &&text[n].isSpace() )
    n++;

  if (n>=textlen) return;

  QString cmd = text.mid( n );

  // Built in help: if the command starts with "help", [try to] show some help
  if ( cmd.startsWith( "help" ) )
  {
    QWhatsThis::showText(mapToGlobal(QPoint(0,0)), helptext( QPoint() ) );
    clear();
    KateCmd::self()->appendHistory( cmd );
    m_histpos = KateCmd::self()->historyLength();
    m_oldText = QString ();
    return;
  }

  if (cmd.length () > 0)
  {
    KTextEditor::Command *p = KateCmd::self()->queryCommand (cmd);

    m_oldText = cmd;
    m_msgMode = true;

    if (p)
    {
      QString msg;

      if (p->exec (m_view, cmd, msg))
      {
        KateCmd::self()->appendHistory( cmd );
        m_histpos = KateCmd::self()->historyLength();
        m_oldText = QString ();

        if (msg.length() > 0)
          setText (i18n ("Success: ") + msg);
        else
          setText (i18n ("Success"));
      }
      else
      {
        if (msg.length() > 0)
          setText (i18n ("Error: ") + msg);
        else
          setText (i18n ("Command \"%1\" failed.",  cmd));
        KNotification::beep();
      }
    }
    else
    {
      setText (i18n ("No such command: \"%1\"",  cmd));
      KNotification::beep();
    }
  }

  // clean up
  if ( m_oldCompletionObject )
  {
    KCompletion *c = completionObject();
    setCompletionObject( m_oldCompletionObject );
    m_oldCompletionObject = 0;
    delete c;
    c = 0;
  }
  m_command = 0;
  m_cmdend = 0;

  m_view->setFocus ();
  QTimer::singleShot( 4000, this, SLOT(hideBar()) );
}

void KateCmdLineEdit::hideBar () // unless i have focus ;)
{
  if ( ! hasFocus() ) {
     m_bar->hideBar ();
  }
}

void KateCmdLineEdit::focusInEvent ( QFocusEvent *ev )
{
  if (m_msgMode)
  {
    m_msgMode = false;
    setText (m_oldText);
    selectAll();
  }

  KLineEdit::focusInEvent (ev);
}

void KateCmdLineEdit::keyPressEvent( QKeyEvent *ev )
{
  if (ev->key() == Qt::Key_Escape)
  {
    m_view->setFocus ();
    hideBar();
  }
  else if ( ev->key() == Qt::Key_Up )
    fromHistory( true );
  else if ( ev->key() == Qt::Key_Down )
    fromHistory( false );

  uint cursorpos = cursorPosition();
  KLineEdit::keyPressEvent (ev);

  // during typing, let us see if we have a valid command
  if ( ! m_cmdend || cursorpos <= m_cmdend  )
  {
    QChar c;
    if ( ! ev->text().isEmpty() )
      c = ev->text()[0];

    if ( ! m_cmdend && ! c.isNull() ) // we have no command, so lets see if we got one
    {
      if ( ! c.isLetterOrNumber() && c != '-' && c != '_' )
      {
        m_command = KateCmd::self()->queryCommand( text().trimmed() );
        if ( m_command )
        {
          //kDebug(13025)<<"keypress in commandline: We have a command! "<<m_command<<". text is '"<<text()<<"'"<<endl;
          // if the typed character is ":",
          // we try if the command has flag completions
          m_cmdend = cursorpos;
          //kDebug(13025)<<"keypress in commandline: Set m_cmdend to "<<m_cmdend<<endl;
        }
        else
          m_cmdend = 0;
      }
    }
    else // since cursor is inside the command name, we reconsider it
    {
      kDebug(13025)<<"keypress in commandline: \\W -- text is "<<text()<<endl;
      m_command = KateCmd::self()->queryCommand( text().trimmed() );
      if ( m_command )
      {
        //kDebug(13025)<<"keypress in commandline: We have a command! "<<m_command<<endl;
        QString t = text();
        m_cmdend = 0;
        bool b = false;
        for ( ; (int)m_cmdend < t.length(); m_cmdend++ )
        {
          if ( t[m_cmdend].isLetter() )
            b = true;
          if ( b && ( ! t[m_cmdend].isLetterOrNumber() && t[m_cmdend] != '-' && t[m_cmdend] != '_' ) )
            break;
        }

        if ( c == ':' && cursorpos == m_cmdend )
        {
          // check if this command wants to complete flags
          //kDebug(13025)<<"keypress in commandline: Checking if flag completion is desired!"<<endl;
        }
      }
      else
      {
        // clean up if needed
        if ( m_oldCompletionObject )
        {
          KCompletion *c = completionObject();
          setCompletionObject( m_oldCompletionObject );
          m_oldCompletionObject = 0;
          delete c;
          c = 0;
        }

        m_cmdend = 0;
      }
    }

    // if we got a command, check if it wants to do semething.
    if ( m_command )
    {
      //kDebug(13025)<<"Checking for CommandExtension.."<<endl;
      KTextEditor::CommandExtension *ce = dynamic_cast<KTextEditor::CommandExtension*>(m_command);
      if ( ce )
      {
        KCompletion *cmpl = ce->completionObject( m_view, text().left( m_cmdend ).trimmed() );
        if ( cmpl )
        {
        // save the old completion object and use what the command provides
        // instead. We also need to prepend the current command name + flag string
        // when completion is done
          //kDebug(13025)<<"keypress in commandline: Setting completion object!"<<endl;
          if ( ! m_oldCompletionObject )
            m_oldCompletionObject = completionObject();

          setCompletionObject( cmpl );
        }
      }
    }
  }
  else if ( m_command )// check if we should call the commands processText()
  {
    KTextEditor::CommandExtension *ce = dynamic_cast<KTextEditor::CommandExtension*>( m_command );
    if ( ce && ce->wantsToProcessText( text().left( m_cmdend ).trimmed() )
         && ! ( ev->text().isNull() || ev->text().isEmpty() ) )
      ce->processText( m_view, text() );
  }
}

void KateCmdLineEdit::fromHistory( bool up )
{
  if ( ! KateCmd::self()->historyLength() )
    return;

  QString s;

  if ( up )
  {
    if ( m_histpos > 0 )
    {
      m_histpos--;
      s = KateCmd::self()->fromHistory( m_histpos );
    }
  }
  else
  {
    if ( m_histpos < ( KateCmd::self()->historyLength() - 1 ) )
    {
      m_histpos++;
      s = KateCmd::self()->fromHistory( m_histpos );
    }
    else
    {
      m_histpos = KateCmd::self()->historyLength();
      setText( m_oldText );
    }
  }
  if ( ! s.isEmpty() )
  {
    // Select the argument part of the command, so that it is easy to overwrite
    setText( s );
    static QRegExp reCmd = QRegExp(".*[\\w\\-]+(?:[^a-zA-Z0-9_-]|:\\w+)(.*)");
    if ( reCmd.indexIn( text() ) == 0 )
      setSelection( text().length() - reCmd.cap(1).length(), reCmd.cap(1).length() );
  }
}
//END KateCmdLineEdit

//BEGIN KateIconBorder
using namespace KTextEditor;

static const char * bookmark_xpm[] = {
"14 13 82 1",
"   c None",
".  c #F27D01",
"+  c #EF7901",
"@  c #F3940F",
"#  c #EE8F12",
"$  c #F9C834",
"%  c #F5C33A",
"&  c #F09110",
"*  c #FCEE3E",
"=  c #FBEB3F",
"-  c #E68614",
";  c #FA8700",
">  c #F78703",
",  c #F4920E",
"'  c #F19113",
")  c #F6C434",
"!  c #FDF938",
"~  c #FDF839",
"{  c #F1BC3A",
"]  c #E18017",
"^  c #DA7210",
"/  c #D5680B",
"(  c #CA5404",
"_  c #FD8F06",
":  c #FCB62D",
"<  c #FDE049",
"[  c #FCE340",
"}  c #FBE334",
"|  c #FDF035",
"1  c #FEF834",
"2  c #FCEF36",
"3  c #F8DF32",
"4  c #F7DC3D",
"5  c #F5CE3E",
"6  c #DE861B",
"7  c #C64C03",
"8  c #F78C07",
"9  c #F8B019",
"0  c #FDE12D",
"a  c #FEE528",
"b  c #FEE229",
"c  c #FBD029",
"d  c #E18814",
"e  c #CB5605",
"f  c #EF8306",
"g  c #F3A00E",
"h  c #FBC718",
"i  c #FED31C",
"j  c #FED11D",
"k  c #F8B91C",
"l  c #E07D0D",
"m  c #CB5301",
"n  c #ED8A0E",
"o  c #F7A90D",
"p  c #FEC113",
"q  c #FEC013",
"r  c #F09B0E",
"s  c #D35E03",
"t  c #EF9213",
"u  c #F9A208",
"v  c #FEAA0C",
"w  c #FCA10B",
"x  c #FCA70B",
"y  c #FEAF0B",
"z  c #F39609",
"A  c #D86203",
"B  c #F08C0D",
"C  c #FA9004",
"D  c #F17F04",
"E  c #E36D04",
"F  c #E16F03",
"G  c #EE8304",
"H  c #F88C04",
"I  c #DC6202",
"J  c #E87204",
"K  c #E66A01",
"L  c #DC6001",
"M  c #D15601",
"N  c #DA5D01",
"O  c #D25200",
"P  c #DA5F00",
"Q  c #BC3C00",
"      .+      ",
"      @#      ",
"      $%      ",
"     &*=-     ",
" ;>,')!~{]^/( ",
"_:<[}|11234567",
" 890aaaaabcde ",
"  fghiiijklm  ",
"   nopqpqrs   ",
"   tuvwxyzA   ",
"   BCDEFGHI   ",
"   JKL  MNO   ",
"   P      Q   "};

const int halfIPW = 8;

KateIconBorder::KateIconBorder ( KateViewInternal* internalView, QWidget *parent )
  : QWidget(parent)
  , m_view( internalView->m_view )
  , m_doc( internalView->m_doc )
  , m_viewInternal( internalView )
  , m_iconBorderOn( false )
  , m_lineNumbersOn( false )
  , m_foldingMarkersOn( false )
  , m_dynWrapIndicatorsOn( false )
  , m_dynWrapIndicators( 0 )
  , m_cachedLNWidth( 0 )
  , m_maxCharWidth( 0 )
  , iconPaneWidth (16)
  , m_blockRange(0)
  , m_lastBlockLine(-1)
{

        for (int i=0;i<MAXFOLDINGCOLORS;i++) {
          int r,g,b;
  /*
          if (info.depth<16) {
            r=0xff;
            g=0xff;
            b=0xff-0x10*info.depth;
          } else if (info.depth<32) {
            r=0xff-0x10*(info.depth-16); g=0xff;b=0;
          } else if (info.depth<48) {
            //r=0;g=0xff;b=0x10*(info.depth-32);
            r=0;g=0xff-0x10*(info.depth-32);b=0;
          } else {
            //r=0;g=0xff;b=0xff;
            r=0;g=0;b=0;
          }
  */
          if (i==0) {
            r=0xff;
            g=0xff;
            b=0xff;
          } else
          if (i<3) {
            r=0xff;
            g=0xff;
            b=0xff-0x40-0x40*i;
          } else if (i<7) {
            r=0xff-0x40*(i-3); g=0xff;b=0;
          } else if (i<15) {
            //r=0;g=0xff;b=0x10*(info.depth-32);
            //r=0;g=0xff-0x40*(i-7);b=0;    // 0x40 creates negative values!
            r=0;g=0xff-0x10*(i-7);b=0;
          } else {
            //r=0;g=0xff;b=0xff;
            r=0;g=0;b=0;
          }
          m_foldingColors[i]=QBrush(QColor(r,g,b,0x88),Qt::SolidPattern /*Qt::Dense4Pattern*/);
          m_foldingColorsSolid[i]=QBrush(QColor(r,g,b),Qt::SolidPattern);
       }

  setAttribute( Qt::WA_StaticContents );
  setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Minimum );
  setMouseTracking(true);
  m_doc->setMarkDescription( MarkInterface::markType01, i18n("Bookmark") );
  m_doc->setMarkPixmap( MarkInterface::markType01, QPixmap((const char**)bookmark_xpm) );

  updateFont();
}

KateIconBorder::~KateIconBorder() {delete m_blockRange;}

void KateIconBorder::setIconBorderOn( bool enable )
{
  if( enable == m_iconBorderOn )
    return;

  m_iconBorderOn = enable;

  updateGeometry();

  QTimer::singleShot( 0, this, SLOT(update()) );
}

void KateIconBorder::setLineNumbersOn( bool enable )
{
  if( enable == m_lineNumbersOn )
    return;

  m_lineNumbersOn = enable;
  m_dynWrapIndicatorsOn = (m_dynWrapIndicators == 1) ? enable : m_dynWrapIndicators;

  updateGeometry();

  QTimer::singleShot( 0, this, SLOT(update()) );
}

void KateIconBorder::setDynWrapIndicators( int state )
{
  if (state == m_dynWrapIndicators )
    return;

  m_dynWrapIndicators = state;
  m_dynWrapIndicatorsOn = (state == 1) ? m_lineNumbersOn : state;

  updateGeometry ();

  QTimer::singleShot( 0, this, SLOT(update()) );
}

void KateIconBorder::setFoldingMarkersOn( bool enable )
{
  if( enable == m_foldingMarkersOn )
    return;

  m_foldingMarkersOn = enable;

  updateGeometry();

  QTimer::singleShot( 0, this, SLOT(update()) );
}

QSize KateIconBorder::sizeHint() const
{
  int w = 0;

  if (m_iconBorderOn)
    w += iconPaneWidth + 1;

  if (m_lineNumbersOn || (m_view->dynWordWrap() && m_dynWrapIndicatorsOn)) {
    w += lineNumberWidth();
  }

  if (m_foldingMarkersOn)
    w += iconPaneWidth;

  w += 4;

  return QSize( w, 0 );
}

// This function (re)calculates the maximum width of any of the digit characters (0 -> 9)
// for graceful handling of variable-width fonts as the linenumber font.
void KateIconBorder::updateFont()
{
  QFontMetrics fm = m_view->renderer()->config()->fontMetrics();
  m_maxCharWidth = 0;
  // Loop to determine the widest numeric character in the current font.
  // 48 is ascii '0'
  for (int i = 48; i < 58; i++) {
    int charWidth = fm.width( QChar(i) );
    m_maxCharWidth = qMax(m_maxCharWidth, charWidth);
  }

  // the icon pane scales with the font...
  iconPaneWidth = fm.height();

  updateGeometry();

  QTimer::singleShot( 0, this, SLOT(update()) );
}

int KateIconBorder::lineNumberWidth() const
{
  int width = m_lineNumbersOn ? ((int)log10((double)(m_view->doc()->lines())) + 1) * m_maxCharWidth + 4 : 0;

  if (m_view->dynWordWrap() && m_dynWrapIndicatorsOn) {
    // HACK: 16 == style().scrollBarExtent().width()
    width = qMax(16 + 4, width);

    if (m_cachedLNWidth != width || m_oldBackgroundColor != m_view->renderer()->config()->iconBarColor()) {
      int w = 16;// HACK: 16 == style().scrollBarExtent().width() style().scrollBarExtent().width();
      int h = m_view->renderer()->config()->fontMetrics().height();

      QSize newSize(w, h);
      if ((m_arrow.size() != newSize || m_oldBackgroundColor != m_view->renderer()->config()->iconBarColor()) && !newSize.isEmpty()) {
        m_arrow = QPixmap(newSize);

        QPainter p(&m_arrow);
        p.fillRect( 0, 0, w, h, m_view->renderer()->config()->iconBarColor() );

        h = m_view->renderer()->config()->fontMetrics().ascent();

        p.setPen(m_view->renderer()->attribute(0)->foreground().color());

        QPainterPath path;
        path.moveTo(w/2, h/2);
        path.lineTo(w/2, 0);
        path.lineTo(w/4, h/4);
        path.lineTo(0, 0);
        path.lineTo(0, h/2);
        path.lineTo(w/2, h-1);
        path.lineTo(w*3/4, h-1);
        path.lineTo(w-1, h*3/4);
        path.lineTo(w*3/4, h/2);
        path.lineTo(0, h/2);
        p.drawPath(path);
      }
    }
  }

  return width;
}

const QBrush& KateIconBorder::foldingColor(KateLineInfo *info,int realLine, bool solid) {
  int depth;
  if (info!=0) {
    depth=info->depth;
  } else {
    KateLineInfo tmp;
    m_doc->lineInfo(&tmp,realLine);
    depth=tmp.depth;
  }

  if (solid) {
    if (depth<MAXFOLDINGCOLORS)
      return m_foldingColorsSolid[depth];
    else
      return m_foldingColorsSolid[MAXFOLDINGCOLORS-1];
  } else {
    if (depth<MAXFOLDINGCOLORS)
      return m_foldingColors[depth];
    else
      return m_foldingColors[MAXFOLDINGCOLORS-1];
  }

}

void KateIconBorder::paintEvent(QPaintEvent* e)
{
  paintBorder(e->rect().x(), e->rect().y(), e->rect().width(), e->rect().height());
}

static void paintTriangle (QPainter &painter, QColor baseColor, int xOffset, int yOffset, int width, int height, bool open)
{
  float size = qMin (width, height);

  QColor c = baseColor.dark ();

  // just test if that worked, else use light..., black on black is evil...
  if (c == baseColor)
    c = baseColor.light ();

  QPen pen;
  pen.setJoinStyle (Qt::RoundJoin);
  pen.setColor (c);
  pen.setWidthF (1.5);
  painter.setPen ( pen );

  painter.setBrush ( c );

  // let some border, if possible
  size *= 0.6;

  float halfSize = size / 2;
  float halfSizeP = halfSize * 0.6;
  QPointF middle (xOffset + (float)width / 2, yOffset + (float)height / 2);

  if (open)
  {
    QPointF points[3] = { middle+QPointF(-halfSize, -halfSizeP), middle+QPointF(halfSize, -halfSizeP), middle+QPointF(0, halfSizeP) };
    painter.drawConvexPolygon(points, 3);
  }
  else
  {
    QPointF points[3] = { middle+QPointF(-halfSizeP, -halfSize), middle+QPointF(-halfSizeP, halfSize), middle+QPointF(halfSizeP, 0) };
    painter.drawConvexPolygon(points, 3);
  }
}

void KateIconBorder::paintBorder (int /*x*/, int y, int /*width*/, int height)
{
  uint h = m_view->renderer()->config()->fontMetrics().height();
  uint startz = (y / h);
  uint endz = startz + 1 + (height / h);
  uint lineRangesSize = m_viewInternal->cache()->viewCacheLineCount();

  // center the folding boxes
  int m_px = (h - 11) / 2;
  if (m_px < 0)
    m_px = 0;

  int lnWidth( 0 );
  if ( m_lineNumbersOn || (m_view->dynWordWrap() && m_dynWrapIndicatorsOn) ) // avoid calculating unless needed ;-)
  {
    lnWidth = lineNumberWidth();
    if ( lnWidth != m_cachedLNWidth || m_oldBackgroundColor != m_view->renderer()->config()->iconBarColor() )
    {
      // we went from n0 ->n9 lines or vice verca
      // this causes an extra updateGeometry() first time the line numbers
      // are displayed, but sizeHint() is supposed to be const so we can't set
      // the cached value there.
      m_cachedLNWidth = lnWidth;
      m_oldBackgroundColor = m_view->renderer()->config()->iconBarColor();
      updateGeometry();
      update ();
      return;
    }
  }

  int w( this->width() );                     // sane value/calc only once

  QPainter p ( this );
  p.setRenderHints (QPainter::Antialiasing);
  p.setFont ( m_view->renderer()->config()->font() ); // for line numbers
  // the line number color is for the line numbers, vertical separator lines
  // and for for the code folding lines.
  p.setPen ( m_view->renderer()->config()->lineNumberColor() );
  p.setBrush ( m_view->renderer()->config()->lineNumberColor() );

  KateLineInfo oldInfo;
  if (startz < lineRangesSize)
  {
    if ((m_viewInternal->cache()->viewLine(startz).line()-1) < 0)
      oldInfo.topLevel = true;
    else
      m_doc->lineInfo(&oldInfo,m_viewInternal->cache()->viewLine(startz).line()-1);
  }

  for (uint z=startz; z <= endz; z++)
  {
    int y = h * z;
    int realLine = -1;

    if (z < lineRangesSize)
      realLine = m_viewInternal->cache()->viewLine(z).line();

    int lnX ( 0 );

    p.fillRect( 0, y, w-4, h, m_view->renderer()->config()->iconBarColor() );
    p.fillRect( w-4, y, 4, h, m_view->renderer()->config()->backgroundColor() );

    // icon pane
    if( m_iconBorderOn )
    {
      p.setPen ( m_view->renderer()->config()->lineNumberColor() );
      p.setBrush ( m_view->renderer()->config()->lineNumberColor() );

      p.drawLine(lnX+iconPaneWidth, y, lnX+iconPaneWidth, y+h);

      if( (realLine > -1) && (m_viewInternal->cache()->viewLine(z).startCol() == 0) )
      {
        uint mrk ( m_doc->mark( realLine ) ); // call only once

        if ( mrk )
        {
          for( uint bit = 0; bit < 32; bit++ )
          {
            MarkInterface::MarkTypes markType = (MarkInterface::MarkTypes)(1<<bit);
            if( mrk & markType )
            {
              QPixmap px_mark (m_doc->markPixmap( markType ));

              if (!px_mark.isNull())
              {
                // center the mark pixmap
                int x_px = (iconPaneWidth - px_mark.width()) / 2;
                if (x_px < 0)
                  x_px = 0;

                int y_px = (h - px_mark.height()) / 2;
                if (y_px < 0)
                  y_px = 0;

                p.drawPixmap( lnX+x_px, y+y_px, px_mark);
              }
            }
          }
        }
      }

      lnX += iconPaneWidth + 1;
    }

    // line number
    if( m_lineNumbersOn || (m_view->dynWordWrap() && m_dynWrapIndicatorsOn) )
    {
      lnX +=2;

      if (realLine > -1)
        if (m_viewInternal->cache()->viewLine(z).startCol() == 0) {
          if (m_lineNumbersOn)
            p.drawText( lnX + 1, y, lnWidth-4, h, Qt::AlignRight|Qt::AlignVCenter, QString("%1").arg( realLine + 1 ) );
        } else if (m_view->dynWordWrap() && m_dynWrapIndicatorsOn) {
          p.drawPixmap(lnX + lnWidth - m_arrow.width() - 4, y, m_arrow);
        }

      lnX += lnWidth;
    }

    // folding markers
    if( m_foldingMarkersOn )
    {
      if( realLine > -1 )
      {
        KateLineInfo info;
        m_doc->lineInfo(&info,realLine);

        QBrush brush (foldingColor(&info,realLine,true));
        p.fillRect(lnX, y, iconPaneWidth, h, brush);

        if (!info.topLevel)
        {
          if (info.startsVisibleBlock && (m_viewInternal->cache()->viewLine(z).startCol() == 0))
          {
            paintTriangle (p, brush.color(), lnX, y, iconPaneWidth, h, true);
          }
          else if (info.startsInVisibleBlock && m_viewInternal->cache()->viewLine(z).startCol() == 0)
          {
            paintTriangle (p, brush.color(), lnX, y, iconPaneWidth, h, false);
          }
          else
          {
           // p.drawLine(lnX+halfIPW,y,lnX+halfIPW,y+h-1);

           // if (info.endsBlock && !m_viewInternal->cache()->viewLine(z).wrap())
            //  p.drawLine(lnX+halfIPW,y+h-1,lnX+iconPaneWidth-2,y+h-1);
          }
        }

        oldInfo = info;
      }

      lnX += iconPaneWidth;
    }
  }
}

KateIconBorder::BorderArea KateIconBorder::positionToArea( const QPoint& p ) const
{
  int x = 0;
  if( m_iconBorderOn ) {
    x += iconPaneWidth;
    if( p.x() <= x )
      return IconBorder;
  }
  if( m_lineNumbersOn || m_dynWrapIndicators ) {
    x += lineNumberWidth();
    if( p.x() <= x )
      return LineNumbers;
  }
  if( m_foldingMarkersOn ) {
    x += iconPaneWidth;
    if( p.x() <= x )
      return FoldingMarkers;
  }
  return None;
}

void KateIconBorder::mousePressEvent( QMouseEvent* e )
{
  const KateTextLayout& t = m_viewInternal->yToKateTextLayout(e->y());
  if (t.isValid()) {
    m_lastClickedLine = t.line();
    if ( positionToArea( e->pos() ) != IconBorder )
    {
      QMouseEvent forward( QEvent::MouseButtonPress,
        QPoint( 0, e->y() ), e->button(), e->buttons(),e->modifiers() );
      m_viewInternal->mousePressEvent( &forward );
    }
    return e->accept();
  }

  QWidget::mousePressEvent(e);
}

void KateIconBorder::showBlock(int line) {
  //kDebug()<<"showBlock: 1"<<endl;
  if (line==m_lastBlockLine) return;
  //kDebug()<<"showBlock: 2"<<endl;
  m_lastBlockLine=line;
  delete m_blockRange;
  m_blockRange=0;
  KateCodeFoldingTree *tree=m_doc->foldingTree();
  if (tree) {
    //kDebug()<<"showBlock: 3"<<endl;
    KateCodeFoldingNode *node = tree->findNodeForLine(line);
    KTextEditor::Cursor beg;
    KTextEditor::Cursor end;
    if (node != tree->rootNode () && node->getBegin(tree,&beg) && node->getEnd(tree,&end)) {
      kDebug()<<"BEGIN"<<beg<<"END"<<end<<endl;
      m_blockRange=m_doc->newSmartRange(KTextEditor::Range(beg,end));
      KTextEditor::Attribute::Ptr attr(new KTextEditor::Attribute());
      attr->setBackground(foldingColor(0,line,false));
      m_blockRange->setAttribute(attr);
      m_doc->addHighlightToView(m_view,m_blockRange,false);
    }
  }
}

void KateIconBorder::hideBlock() {
  m_lastBlockLine=-1;
  delete m_blockRange;
  m_blockRange=0;
}

void KateIconBorder::mouseMoveEvent( QMouseEvent* e )
{
  const KateTextLayout& t = m_viewInternal->yToKateTextLayout(e->y());
  if (t.isValid()) {
    if ( positionToArea( e->pos() ) == FoldingMarkers) showBlock(t.line());
    else hideBlock();
    if ( positionToArea( e->pos() ) != IconBorder )
    {
      QMouseEvent forward( QEvent::MouseMove,
        QPoint( 0, e->y() ), e->button(), e->buttons(),e->modifiers() );
      m_viewInternal->mouseMoveEvent( &forward );
    }
  }

  QWidget::mouseMoveEvent(e);
}

void KateIconBorder::mouseReleaseEvent( QMouseEvent* e )
{
  int cursorOnLine = m_viewInternal->yToKateTextLayout(e->y()).line();

  if (cursorOnLine == m_lastClickedLine &&
      cursorOnLine <= m_doc->lastLine() )
  {
    BorderArea area = positionToArea( e->pos() );
    if( area == IconBorder) {
      if (e->button() == Qt::LeftButton) {
        if( m_doc->editableMarks() & KateViewConfig::global()->defaultMarkType() ) {
          if( m_doc->mark( cursorOnLine ) & KateViewConfig::global()->defaultMarkType() )
            m_doc->removeMark( cursorOnLine, KateViewConfig::global()->defaultMarkType() );
          else
            m_doc->addMark( cursorOnLine, KateViewConfig::global()->defaultMarkType() );
          } else {
            showMarkMenu( cursorOnLine, QCursor::pos() );
          }
        }
        else
        if (e->button() == Qt::RightButton) {
          showMarkMenu( cursorOnLine, QCursor::pos() );
        }
    }

    if ( area == FoldingMarkers) {
      KateLineInfo info;
      m_doc->lineInfo(&info,cursorOnLine);
      if ((info.startsVisibleBlock) || (info.startsInVisibleBlock)) {
        emit toggleRegionVisibility(cursorOnLine);
      }
    }
  }

  QMouseEvent forward( QEvent::MouseButtonRelease,
    QPoint( 0, e->y() ), e->button(), e->buttons(),e->modifiers() );
  m_viewInternal->mouseReleaseEvent( &forward );
}

void KateIconBorder::mouseDoubleClickEvent( QMouseEvent* e )
{
  QMouseEvent forward( QEvent::MouseButtonDblClick,
    QPoint( 0, e->y() ), e->button(), e->buttons(),e->modifiers() );
  m_viewInternal->mouseDoubleClickEvent( &forward );
}

void KateIconBorder::showMarkMenu( uint line, const QPoint& pos )
{
  KMenu markMenu;
  KMenu selectDefaultMark;

  QVector<int> vec( 33 );
  int i=1;

  for( uint bit = 0; bit < 32; bit++ ) {
    MarkInterface::MarkTypes markType = (MarkInterface::MarkTypes)(1<<bit);
    if( !(m_doc->editableMarks() & markType) )
      continue;

    QAction *mA;
    QAction *dMA;
    if( !m_doc->markDescription( markType ).isEmpty() ) {
      mA=markMenu.addAction( m_doc->markDescription( markType ));
      dMA=selectDefaultMark.addAction( m_doc->markDescription( markType ));
    } else {
      mA=markMenu.addAction( i18n("Mark Type %1",  bit + 1 ));
      dMA=selectDefaultMark.addAction( i18n("Mark Type %1",  bit + 1 ));
    }
    mA->setData(i);
    mA->setCheckable(true);
    dMA->setData(i+100);
    dMA->setCheckable(true);
    if( m_doc->mark( line ) & markType )
      mA->setChecked(true );

    if( markType & KateViewConfig::global()->defaultMarkType() )
      dMA->setChecked(true );

    vec[i++] = markType;
  }

  if( markMenu.actions().count() == 0 )
    return;

  if( markMenu.actions().count() > 1 )
    markMenu.addAction( i18n("Set Default Mark Type" ))->setMenu(&selectDefaultMark);

  QAction *rA = markMenu.exec( pos );
  if( !rA )
    return;
  int result=rA->data().toInt();
  if ( result > 100)
  {
     KateViewConfig::global()->setDefaultMarkType (vec[result-100]);
     // flush config, otherwise it isn't nessecarily done
     KConfigGroup cg(KGlobal::config(), "Kate View Defaults");
     KateViewConfig::global()->writeConfig(cg);
  }
  else
  {
    MarkInterface::MarkTypes markType = (MarkInterface::MarkTypes) vec[result];
    if( m_doc->mark( line ) & markType ) {
      m_doc->removeMark( line, markType );
    } else {
        m_doc->addMark( line, markType );
    }
  }
}
//END KateIconBorder

KateViewEncodingAction::KateViewEncodingAction(KateDocument *_doc, KateView *_view, const QString& text, QObject *parent)
       : KCodecAction(text, parent,true), doc(_doc), view (_view)
{
  connect(this,SIGNAL(triggered(KEncodingDetector::AutoDetectScript)),this,SLOT(setScriptForEncodingAutoDetection(KEncodingDetector::AutoDetectScript)));
  connect(this,SIGNAL(triggered(const QString&)),this,SLOT(setEncoding(const QString&)));
  connect(menu(),SIGNAL(aboutToShow()),this,SLOT(slotAboutToShow()));
}

void KateViewEncodingAction::slotAboutToShow()
{
  if (doc->scriptForEncodingAutoDetection()==KEncodingDetector::None)
  {
    if (!setCurrentCodec(doc->encoding()))
      kWarning() << "KateViewEncodingAction: cannot set current "<<doc->encoding() << endl;
  }
  else
    setCurrentAutoDetectScript(doc->scriptForEncodingAutoDetection());
}

void KateViewEncodingAction::setEncoding (const QString &e)
{
  doc->setEncoding(e);
  //this is done in setEncoding()
  //doc->setScriptForEncodingAutoDetection(KEncodingDetector::None);
  view->reloadFile();

}
void KateViewEncodingAction::setScriptForEncodingAutoDetection (KEncodingDetector::AutoDetectScript script)
{
  if (script==KEncodingDetector::SemiautomaticDetection)
  {
    doc->setEncoding("");
#ifdef DECODE_DEBUG
    kWarning() << "KEncodingDetector::SemiautomaticDetection " <<doc->encoding() << endl;
#endif
  }
  else
    doc->setScriptForEncodingAutoDetection(script);
  view->reloadFile();
}



//BEGIN KateViewBar related classes

KateViewBarWidget::KateViewBarWidget (KateViewBar *viewBar)
 : QWidget (), m_viewBar (viewBar)
{
  m_viewBar->addBarWidget (this);

  QHBoxLayout *layout = new QHBoxLayout;

  // NOTE: Here be cosmetics.
  layout->setMargin(2);

  // hide button
  QToolButton *hideButton = new QToolButton(this);
  hideButton->setAutoRaise(true);
  hideButton->setIcon(KIcon("process-stop"));
  connect(hideButton, SIGNAL(clicked()), this, SLOT(hideBar()));
  layout->addWidget(hideButton);
  layout->setAlignment( hideButton, Qt::AlignLeft|Qt::AlignTop );

  // widget to be used as parent for the real content
  m_centralWidget = new QWidget ();
  layout->addWidget(m_centralWidget);

  setLayout(layout);
  setFocusProxy(m_centralWidget);
}

void KateViewBarWidget::showBar ()
{
  m_viewBar->showBarWidget (this);
}

void KateViewBarWidget::hideBar ()
{
  // let the barwidget do some stuff on hide, perhaps even say: no, don't hide me...
  if (!hideIsTriggered ())
    return;

  m_viewBar->hideBarWidget ();
}



KateStackedLayout::KateStackedLayout(QWidget* parent)
  : QStackedLayout(parent)
{}

QSize KateStackedLayout::sizeHint() const
{
  if (currentWidget())
    return currentWidget()->sizeHint();
  return QStackedLayout::sizeHint();
}

QSize KateStackedLayout::minimumSize() const
{
  if (currentWidget())
    return currentWidget()->minimumSize();
  return QStackedLayout::minimumSize();
}



KateViewBar::KateViewBar (KateView *view)
 : QWidget (view), m_view (view)
{
  m_stack = new KateStackedLayout(this);
  hide ();
}

void KateViewBar::addBarWidget (KateViewBarWidget *newBarWidget)
{
  // add new widget, invisible...
  m_stack->addWidget (newBarWidget);

  kDebug(13025)<<"add barwidget " << newBarWidget <<endl;
}

void KateViewBar::showBarWidget (KateViewBarWidget *barWidget)
{
  // raise correct widget
  m_stack->setCurrentWidget (barWidget);
  kDebug(13025)<<"show barwidget " << barWidget <<endl;
  show ();
}

void KateViewBar::hideBarWidget ()
{
  hide();
  kDebug(13025)<<"hide barwidget"<<endl;
}

void KateViewBar::keyPressEvent(QKeyEvent* event)
{
  if (event->key() == Qt::Key_Escape) {
    hideBarWidget();
    return;
  }
  QWidget::keyPressEvent(event);
}

void KateViewBar::hideEvent(QHideEvent* event)
{
  if (!event->spontaneous())
    m_view->setFocus();
}

//END KateViewBar related classes

// kate: space-indent on; indent-width 2; replace-tabs on;
