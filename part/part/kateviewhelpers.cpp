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
#include "katefont.h"

#include <kapplication.h>
#include <kglobalsettings.h>
#include <klocale.h>
#include <knotifyclient.h>
#include <kglobal.h>
#include <kcharsets.h>
#include <kmenu.h>
#include <kmenu.h>

#include <qcursor.h>
#include <qpainter.h>
#include <qstyle.h>
#include <qtimer.h>
#include <qregexp.h>
#include <qtextcodec.h>
#include <QMouseEvent>
#include <QPainterPath>

#include <math.h>

#include <kdebug.h>

#include <QWhatsThis>

//BEGIN KateScrollBar
KateScrollBar::KateScrollBar (Qt::Orientation orientation, KateViewInternal* parent, const char* name)
  : QScrollBar (orientation, parent->m_view, name)
  , m_middleMouseDown (false)
  , m_view(parent->m_view)
  , m_doc(parent->m_doc)
  , m_viewInternal(parent)
  , m_topMargin(-1)
  , m_bottomMargin(-1)
  , m_savVisibleLines(0)
  , m_showMarks(false)
{
  connect(this, SIGNAL(valueChanged(int)), this, SLOT(sliderMaybeMoved(int)));
  connect(m_doc, SIGNAL(marksChanged(KTextEditor::Document*)), this, SLOT(marksChanged()));
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

  if (e->state() | Qt::LeftButton)
    redrawMarks();
}

void KateScrollBar::paintEvent(QPaintEvent *e)
{
  QPainter painter(this);

  QScrollBar::paintEvent(e);

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
  m_topMargin = -1;
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
  if (m_topMargin == -1)
    watchScrollBarSize();

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

void KateScrollBar::watchScrollBarSize()
{
#ifdef __GNUC__
#warning port this to QT 4
#endif
/*
  int savMax = maxValue();
  setMaxValue(0);
  QRect rect = sliderRect();
  setMaxValue(savMax);

  m_topMargin = rect.top();
  m_bottomMargin = frameGeometry().height() - rect.bottom();
*/
}

void KateScrollBar::sliderMaybeMoved(int value)
{
  if (m_middleMouseDown)
    emit sliderMMBMoved(value);
}
//END


//BEGIN KateCmdLineFlagCompletion
/**
 * This class provide completion of flags. It shows a short description of
 * each flag, and flags are appended.
 */
class KateCmdLineFlagCompletion : public KCompletion
{
  public:
    KateCmdLineFlagCompletion() {;}

    QString makeCompletion( const QString & /*s*/ )
    {
      return QString::null;
    }

};
//END KateCmdLineFlagCompletion

//BEGIN KateCmdLine
KateCmdLine::KateCmdLine (KateView *view)
  : KLineEdit (view)
  , m_view (view)
  , m_msgMode (false)
  , m_histpos( 0 )
  , m_cmdend( 0 )
  , m_command( 0L )
  , m_oldCompletionObject( 0L )
{
  connect (this, SIGNAL(returnPressed(const QString &)),
           this, SLOT(slotReturnPressed(const QString &)));

  completionObject()->insertItems (KateCmd::self()->cmds());
  setAutoDeleteCompletionObject( false );
}


QString KateCmdLine::helptext( const QPoint & ) const
    {
      QString beg = "<qt background=\"white\"><div><table width=\"100%\"><tr><td bgcolor=\"brown\"><font color=\"white\"><b>Help: <big>";
      QString mid = "</big></b></font></td></tr><tr><td>";
      QString end = "</td></tr></table></div><qt>";

      QString t = text();
      QRegExp re( "\\s*help\\s+(.*)" );
      if ( re.search( t ) > -1 )
      {
        QString s;
        // get help for command
        QString name = re.cap( 1 );
        if ( name == "list" )
        {
          return beg + i18n("Available Commands") + mid
              + KateCmd::self()->cmds().join(" ")
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
              return beg + name + mid + i18n("No help for '%1'").arg( name ) + end;
          }
          else
            return beg + mid + i18n("No such command <b>%1</b>").arg(name) + end;
        }
      }

      return beg + mid + i18n(
          "<p>This is the Katepart <b>command line</b>.<br>"
          "Syntax: <code><b>command [ arguments ]</b></code><br>"
          "For a list of available commands, enter <code><b>help list</b></code><br>"
          "For help for individual commands, enter <code><b>help &lt;command&gt;</b></code></p>")
          + end;
    }



bool KateCmdLine::event(QEvent *e) {
	if (e->type()==QEvent::WhatsThis)
		setWhatsThis(helptext(QPoint()));
	return KLineEdit::event(e);
}

void KateCmdLine::slotReturnPressed ( const QString& text )
{

  // silently ignore leading space
  uint n = 0;
  while( text[n].isSpace() )
    n++;

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
          setText (i18n ("Command \"%1\" failed.").arg (cmd));
        KNotifyClient::beep();
      }
    }
    else
    {
      setText (i18n ("No such command: \"%1\"").arg (cmd));
      KNotifyClient::beep();
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
  QTimer::singleShot( 4000, this, SLOT(hideMe()) );
}

void KateCmdLine::hideMe () // unless i have focus ;)
{
  if ( isVisibleTo(parentWidget()) && ! hasFocus() ) {
     m_view->toggleCmdLine ();
  }
}

void KateCmdLine::focusInEvent ( QFocusEvent *ev )
{
  if (m_msgMode)
  {
    m_msgMode = false;
    setText (m_oldText);
    selectAll();
  }

  KLineEdit::focusInEvent (ev);
}

void KateCmdLine::keyPressEvent( QKeyEvent *ev )
{
  if (ev->key() == Qt::Key_Escape)
  {
    m_view->setFocus ();
    hideMe();
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
          //kdDebug(13025)<<"keypress in commandline: We have a command! "<<m_command<<". text is '"<<text()<<"'"<<endl;
          // if the typed character is ":",
          // we try if the command has flag completions
          m_cmdend = cursorpos;
          //kdDebug(13025)<<"keypress in commandline: Set m_cmdend to "<<m_cmdend<<endl;
        }
        else
          m_cmdend = 0;
      }
    }
    else // since cursor is inside the command name, we reconsider it
    {
      kdDebug(13025)<<"keypress in commandline: \\W -- text is "<<text()<<endl;
      m_command = KateCmd::self()->queryCommand( text().trimmed() );
      if ( m_command )
      {
        //kdDebug(13025)<<"keypress in commandline: We have a command! "<<m_command<<endl;
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
          //kdDebug(13025)<<"keypress in commandline: Checking if flag completion is desired!"<<endl;
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
      //kdDebug(13025)<<"Checking for CommandExtension.."<<endl;
      KTextEditor::CommandExtension *ce = dynamic_cast<KTextEditor::CommandExtension*>(m_command);
      if ( ce )
      {
        KCompletion *cmpl = ce->completionObject( m_view, text().left( m_cmdend ).trimmed() );
        if ( cmpl )
        {
        // save the old completion object and use what the command provides
        // instead. We also need to prepend the current command name + flag string
        // when completion is done
          //kdDebug(13025)<<"keypress in commandline: Setting completion object!"<<endl;
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

void KateCmdLine::fromHistory( bool up )
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
    if ( reCmd.search( text() ) == 0 )
      setSelection( text().length() - reCmd.cap(1).length(), reCmd.cap(1).length() );
  }
}
//END KateCmdLine

//BEGIN KateIconBorder
using namespace KTextEditor;

static const char* const plus_xpm[] = {
"11 11 3 1",
"       c None",
".      c #000000",
"+      c #FFFFFF",
"...........",
".+++++++++.",
".+++++++++.",
".++++.++++.",
".++++.++++.",
".++.....++.",
".++++.++++.",
".++++.++++.",
".+++++++++.",
".+++++++++.",
"..........."};

static const char* const minus_xpm[] = {
"11 11 3 1",
"       c None",
".      c #000000",
"+      c #FFFFFF",
"...........",
".+++++++++.",
".+++++++++.",
".+++++++++.",
".+++++++++.",
".++.....++.",
".+++++++++.",
".+++++++++.",
".+++++++++.",
".+++++++++.",
"..........."};

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

const int iconPaneWidth = 16;
const int halfIPW = 8;

KateIconBorder::KateIconBorder ( KateViewInternal* internalView, QWidget *parent )
  : QWidget(parent, Qt::WStaticContents | Qt::WNoAutoErase | Qt::WResizeNoErase )
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
  , minus_px ((const char**)minus_xpm)
  , plus_px ((const char**)plus_xpm)
{
  setSizePolicy( QSizePolicy(  QSizePolicy::Fixed, QSizePolicy::Minimum ) );

  m_doc->setMarkDescription( MarkInterface::markType01, i18n("Bookmark") );
  m_doc->setMarkPixmap( MarkInterface::markType01, QPixmap((const char**)bookmark_xpm) );

  updateFont();
}

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
  const QFontMetrics *fm = m_view->renderer()->config()->fontMetrics();
  m_maxCharWidth = 0;
  // Loop to determine the widest numeric character in the current font.
  // 48 is ascii '0'
  for (int i = 48; i < 58; i++) {
    int charWidth = fm->width( QChar(i) );
    m_maxCharWidth = qMax(m_maxCharWidth, charWidth);
  }
}

int KateIconBorder::lineNumberWidth() const
{
  int width = m_lineNumbersOn ? ((int)log10((double)(m_view->doc()->lines())) + 1) * m_maxCharWidth + 4 : 0;

  if (m_view->dynWordWrap() && m_dynWrapIndicatorsOn) {
    // HACK: 16 == style().scrollBarExtent().width()
    width = qMax(16 + 4, width);

    if (m_cachedLNWidth != width || m_oldBackgroundColor != m_view->renderer()->config()->iconBarColor()) {
      int w = 16;// HACK: 16 == style().scrollBarExtent().width() style().scrollBarExtent().width();
      int h = m_view->renderer()->config()->fontMetrics()->height();

      QSize newSize(w, h);
      if ((m_arrow.size() != newSize || m_oldBackgroundColor != m_view->renderer()->config()->iconBarColor()) && !newSize.isEmpty()) {
        m_arrow.resize(newSize);

        QPainter p(&m_arrow);
        p.fillRect( 0, 0, w, h, m_view->renderer()->config()->iconBarColor() );

        h = m_view->renderer()->config()->fontMetrics()->ascent();

        p.setPen(m_view->renderer()->attribute(0)->foreground());
        //p.drawLine(w/2, h/2, w/2, 0);

#if 1
        QPainterPath path;
        path.lineTo(w/4, h/4);
        path.lineTo(0, 0);
        path.lineTo(0, h/2);
        path.lineTo(w/2, h-1);
        path.lineTo(w*3/4, h-1);
        path.lineTo(w-1, h*3/4);
        path.lineTo(w*3/4, h/2);
        path.lineTo(0, h/2);

        p.drawPath(path);
#else
        path.lineTo(w*3/4, h/4);
        path.lineTo(w-1,0);
        path.lineTo(w-1, h/2);
        path.lineTo(w/2, h-1);
        path.lineTo(w/4,h-1);
        path.lineTo(0, h*3/4);
        path.lineTo(w/4, h/2);
        path.lineTo(w-1, h/2);
#endif
      }
    }
  }

  return width;
}

void KateIconBorder::paintEvent(QPaintEvent* e)
{
  paintBorder(e->rect().x(), e->rect().y(), e->rect().width(), e->rect().height());
}

void KateIconBorder::paintBorder (int /*x*/, int y, int /*width*/, int height)
{
  uint h = m_view->renderer()->config()->fontStruct()->fontHeight;
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
  p.setFont ( *m_view->renderer()->config()->font() ); // for line numbers
  // the line number color is for the line numbers, vertical separator lines
  // and for for the code folding lines.
  p.setPen ( m_view->renderer()->config()->lineNumberColor() );

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

        if (!info.topLevel)
        {
          if (info.startsVisibleBlock && (m_viewInternal->cache()->viewLine(z).startCol() == 0))
          {
            if (oldInfo.topLevel)
              p.drawLine(lnX+halfIPW,y+m_px,lnX+halfIPW,y+h-1);
            else
              p.drawLine(lnX+halfIPW,y,lnX+halfIPW,y+h-1);

            p.drawPixmap(lnX+3,y+m_px,minus_px);
          }
          else if (info.startsInVisibleBlock)
          {
            if (m_viewInternal->cache()->viewLine(z).startCol() == 0)
            {
              if (oldInfo.topLevel)
                p.drawLine(lnX+halfIPW,y+m_px,lnX+halfIPW,y+h-1);
              else
                p.drawLine(lnX+halfIPW,y,lnX+halfIPW,y+h-1);

              p.drawPixmap(lnX+3,y+m_px,plus_px);
            }
            else
            {
              p.drawLine(lnX+halfIPW,y,lnX+halfIPW,y+h-1);
            }

            if (!m_viewInternal->cache()->viewLine(z).wrap())
              p.drawLine(lnX+halfIPW,y+h-1,lnX+iconPaneWidth-2,y+h-1);
          }
          else
          {
            p.drawLine(lnX+halfIPW,y,lnX+halfIPW,y+h-1);

            if (info.endsBlock && !m_viewInternal->cache()->viewLine(z).wrap())
              p.drawLine(lnX+halfIPW,y+h-1,lnX+iconPaneWidth-2,y+h-1);
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

void KateIconBorder::mouseMoveEvent( QMouseEvent* e )
{
  const KateTextLayout& t = m_viewInternal->yToKateTextLayout(e->y());
  if (t.isValid()) {
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
      mA=markMenu.addAction( i18n("Mark Type %1").arg( bit + 1 ));
      dMA=selectDefaultMark.addAction( i18n("Mark Type %1").arg( bit + 1 ));
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
     KConfig *config = KGlobal::config();
     config->setGroup("Kate View Defaults");
     KateViewConfig::global()->writeConfig( config );
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

KateViewEncodingAction::KateViewEncodingAction(KateDocument *_doc, KateView *_view, const QString& text, KActionCollection* parent, const char* name)
       : KActionMenu (text, parent, name), doc(_doc), view (_view)
{
  m_actions=new QActionGroup(this);
  m_actions->setExclusive(true);

  QStringList modes (KGlobal::charsets()->descriptiveEncodingNames());
  for (int z=0; z<modes.size(); ++z)
  {
    QAction *a=m_actions->addAction( modes[z]);
    a->setCheckable(true);
    a->setData(z);
  }
  popupMenu()->addActions(m_actions->actions());
  connect(m_actions,SIGNAL(triggered(QAction*)),this,SLOT(setMode(QAction*)));
  connect(popupMenu(),SIGNAL(aboutToShow()),this,SLOT(slotAboutToShow()));
}

void KateViewEncodingAction::slotAboutToShow()
{
  QStringList modes (KGlobal::charsets()->descriptiveEncodingNames());
  QString name=doc->config()->codec()->name();
  int id=-1;
  int i=0;
  foreach(const QString &cname,modes) {
    bool found = false;
    QTextCodec *codecForEnc = KGlobal::charsets()->codecForName(KGlobal::charsets()->encodingForName(cname), found);
    if (codecForEnc && (codecForEnc->name()==name)) {
      id=i;
      break;
    }
    i++;
  }
  popupMenu()->clear();
  kdDebug()<<id<<endl<<m_actions->actions().size()<<endl;
  if ( (id<0) || (id>=m_actions->actions().size()) || (m_actions->actions()[id]->data().toInt()!=id)) {
    popupMenu()->addAction(i18n("Encoding management error"))->setEnabled(false);
  } else {
    popupMenu()->addActions(m_actions->actions());
    m_actions->actions()[id]->setChecked(true);
  }
}

void KateViewEncodingAction::setMode (QAction* a)
{
  kdDebug()<<"setMode"<<endl;
  QStringList modes (KGlobal::charsets()->descriptiveEncodingNames());
  doc->setEncoding( KGlobal::charsets()->encodingForName( modes[a->data().toInt()] ) );
  view->reloadFile();
}

// kate: space-indent on; indent-width 2; replace-tabs on;
