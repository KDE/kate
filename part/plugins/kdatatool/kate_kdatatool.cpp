#include "kate_kdatatool.h"
#include "kate_kdatatool.moc"
#include <kgenericfactory.h>
#include <kaction.h>
#include <ktexteditor/view.h>
#include <kdebug.h>
#include <kdatatool.h>
#include <ktexteditor/document.h>
#include <ktexteditor/selectioninterface.h>
#include <kpopupmenu.h>
#include <ktexteditor/viewcursorinterface.h>
#include <ktexteditor/editinterface.h>

K_EXPORT_COMPONENT_FACTORY( ktexteditor_kdatatool, KGenericFactory<KTextEditor::KDataToolPlugin>( "ktexteditor_kdatatool" ) );

namespace KTextEditor {

KDataToolPlugin::KDataToolPlugin( QObject *parent, const char* name, const QStringList& )
        : KTextEditor::Plugin ( (KTextEditor::Document*) parent, name ) {
}


KDataToolPlugin::~KDataToolPlugin () {

}

void KDataToolPlugin::addView(KTextEditor::View *view)
{
  KDataToolPluginView *nview = new KDataToolPluginView (view);
  nview->setView (view);
//  m_views.append (nview);
}


KDataToolPluginView::KDataToolPluginView( KTextEditor::View *view )
	:m_menu(0){

	view->insertChildClient (this);
        setInstance( KGenericFactory<KDataToolPlugin>::instance() );


	m_menu= new KActionMenu(i18n("Datatools"),actionCollection(),"popup_dataTool");
	connect(m_menu->popupMenu(),SIGNAL(aboutToShow()),this,SLOT(aboutToShow()));
        setXMLFile( "ktexteditor_kdatatoolui.rc" );

	m_view=view;
}

KDataToolPluginView::~KDataToolPluginView(){
	delete m_menu;
}

void KDataToolPluginView::aboutToShow()
{
  kdDebug()<<"KTextEditor::KDataToolPluginView::aboutToShow"<<endl;
  QString word;
  m_singleWord = false;
  m_wordUnderCursor = QString::null;

  if ( selectionInterface(m_view->document())->hasSelection() )
  {
    word = selectionInterface(m_view->document())->selection();
    if ( word.find(' ') == -1 && word.find('\t') == -1 && word.find('\n') == -1 ) {
        m_singleWord = true;
    } else {
        m_singleWord = false;
    }
  } else {
    return;
    // fixme:
    // No selection -> use word under cursor
    //m_singleWord = true;
    //m_wordUnderCursor = word;
  }

  KInstance *inst=instance();

  QValueList<KDataToolInfo> tools;
  tools += KDataToolInfo::query( "QString", "text/plain", inst );
  if( m_singleWord )
    tools += KDataToolInfo::query( "QString", "application/x-singleword", inst );

  // unplug old actions, if any:
  KAction *ac;
  for ( ac = m_actionList.first(); ac; ac = m_actionList.next() ) {
	m_menu->remove(ac);
  }

  m_actionList = KDataToolAction::dataToolActionList( tools, this,
    SLOT( slotToolActivated( const KDataToolInfo &, const QString & ) ) );

  for ( ac = m_actionList.first(); ac; ac = m_actionList.next() ) {
	m_menu->insert(ac);
  }
}

void KDataToolPluginView::slotToolActivated( const KDataToolInfo &info, const QString &command )
{

  KDataTool* tool = info.createTool( );
  if ( !tool )
  {
      kdWarning() << "Could not create Tool !" << endl;
      return;
  }

  QString text;
  if ( selectionInterface(m_view->document())->hasSelection() )
    text = selectionInterface(m_view->document())->selection();
  else
    text = m_wordUnderCursor;

  QString mimetype = "text/plain";
  QString datatype = "QString";

  // If unsupported (and if we have a single word indeed), try application/x-singleword
  if ( !info.mimeTypes().contains( mimetype ) && m_singleWord )
    mimetype = "application/x-singleword";

  kdDebug() << "Running tool with datatype=" << datatype << " mimetype=" << mimetype << endl;

  QString origText = text;
  if ( tool->run( command, &text, datatype, mimetype) )
  {
    kdDebug() << "Tool ran. Text is now " << text << endl;
    if ( origText != text )
    {
       uint line, col;
       viewCursorInterface(m_view)->cursorPositionReal(&line, &col);
       if ( selectionInterface(m_view->document())->hasSelection() )
       {
         // replace selection with 'text'
         selectionInterface(m_view->document())->removeSelectedText();
	
         viewCursorInterface(m_view)->cursorPositionReal(&line, &col);
         editInterface(m_view->document())->insertText(line,col,text);
         // place cursor at the end:
/* No idea yet (Joseph Wenninger)
         for ( uint i = 0; i < text.length(); i++ ) {
           viewCursorInterface(m_view)->cursorRight();
         }
*/
       }
          // fixme: else case: use word under cursor
    }
  }

  delete tool;
}


};
