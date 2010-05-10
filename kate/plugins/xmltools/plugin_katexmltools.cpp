/***************************************************************************
  pluginKatexmltools.cpp

  List elements, attributes, attribute values and entities allowed by DTD.
  Needs a DTD in XML format ( as produced by dtdparse ) for most features.

  copyright			: ( C ) 2001-2002 by Daniel Naber
  email				: daniel.naber@t-online.de

  Copyright (C) 2005 by Anders Lund <anders@alweb.dk>
 ***************************************************************************/

/***************************************************************************
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or ( at your option ) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ***************************************************************************/

/*
README:
The basic idea is this: certain keyEvents(), namely [<&" ], trigger a completion box.
This is intended as a help for editing. There are some cases where the XML
spec is not followed, e.g. one can add the same attribute twice to an element.
Also see the user documentation. If backspace is pressed after a completion popup
was closed, the popup will re-open. This way typos can be corrected and the popup
will reappear, which is quite comfortable.

FIXME for jowenn if he has time:
-Ctrl-Z doesn't work if completion is visible
-Typing with popup works, but right/left cursor keys and start/end don't, i.e.
 they should be ignored by the completion ( ? )
-popup not completely visible if it's long and appears at the bottom of the screen

FIXME:
-( docbook ) <author lang="">: insert space between the quotes, press "de" and return -> only "d" inserted
-Correctly support more than one view:
 charactersInteractivelyInserted( ..) is tied to kv->document()
 but filterInsertString( .. ) is tied to kv
-The "Insert Element" dialog isn't case insensitive, but it should be
-fix upper/lower case problems ( start typing lowercase if the tag etc. is upper case )
-See the "fixme"'s in the code

TODO:
-check for mem leaks
-add "Go to opening/parent tag"?
-check doctype to get top-level element
-can undo behaviour be improved?, e.g. the plugins internal deletions of text
 don't have to be an extra step
-don't offer entities if inside tag but outside attribute value

-Support for more than one namespace at the same time ( e.g. XSLT + XSL-FO )?
=>This could also be handled in the XSLT DTD fragment, as described in the XSLT 1.0 spec,
 but then at <xsl:template match="/"><html> it will only show you HTML elements!
=>So better "Assign meta DTD" and "Add meta DTD", the latter will expand the current meta DTD
-Option to insert empty element in <empty/> form
-Show expanded entities with QChar::QChar( int rc ) + unicode font
-Don't ignore entities defined in the document's prologue
-Only offer 'valid' elements, i.e. don't take the elements as a set but check
 if the DTD is matched ( order, number of occurrences, ... )

-Maybe only read the meta DTD file once, then store the resulting QMap on disk ( using QDataStream )?
 We'll then have to compare timeOf_cacheFile <-> timeOf_metaDtd.
-Try to use libxml
*/

#include "plugin_katexmltools.h"
#include "plugin_katexmltools.moc"

#include <assert.h>

#include <qdatetime.h>
#include <qdom.h>
#include <qfile.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qregexp.h>
#include <qstring.h>
#include <qtimer.h>
#include <QLabel>
#include <QVBoxLayout>

#include <kaction.h>
#include <kactioncollection.h>
#include <kapplication.h>
#include <klineedit.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <kglobal.h>
#include <khistorycombobox.h>
#include <kcomponentdata.h>
#include <kio/job.h>
#include <kio/jobuidelegate.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <kgenericfactory.h>

K_EXPORT_COMPONENT_FACTORY( katexmltoolsplugin, KGenericFactory<PluginKateXMLTools>( "katexmltools" ) )

using Kate::application;


PluginKateXMLTools::PluginKateXMLTools( QObject* parent, const QStringList& )
    : Kate::Plugin ( (Kate::Application *)parent )
{
}

PluginKateXMLTools::~PluginKateXMLTools()
{
}

Kate::PluginView *PluginKateXMLTools::createView(Kate::MainWindow *mainWindow)
{
    return new PluginKateXMLToolsView(mainWindow);
}


PluginKateXMLToolsView::PluginKateXMLToolsView(Kate::MainWindow *win)
  : Kate::PluginView ( win ), Kate::XMLGUIClient ( KGenericFactory<PluginKateXMLTools>::componentData() ),
    m_model ( this )
{
  //kDebug() << "PluginKateXMLTools constructor called";


  KAction *actionInsert = new KAction ( i18n("&Insert Element..."), this );
  actionInsert->setShortcut( Qt::CTRL+Qt::Key_Return );
  connect( actionInsert, SIGNAL(triggered()), &m_model, SLOT(slotInsertElement()) );
  actionCollection()->addAction( "xml_tool_insert_element", actionInsert );

  KAction *actionClose = new KAction ( i18n("&Close Element"), this );
  actionClose->setShortcut( Qt::CTRL+Qt::Key_Less );
  connect( actionClose, SIGNAL(triggered()), &m_model, SLOT(slotCloseElement()) );
  actionCollection()->addAction( "xml_tool_close_element", actionClose );

  KAction *actionAssignDTD = new KAction ( i18n("Assign Meta &DTD..." ), this );
  connect( actionAssignDTD, SIGNAL(triggered()), &m_model, SLOT(getDTD()) );
  actionCollection()->addAction( "xml_tool_assign", actionAssignDTD );

  setXMLFile( "plugins/katexmltools/ui.rc" );
  win->guiFactory()->addClient( this );

  connect( application()->documentManager(), SIGNAL(documentDeleted(KTextEditor::Document *)),
            &m_model, SLOT(slotDocumentDeleted(KTextEditor::Document *)) );

}

PluginKateXMLToolsView::~PluginKateXMLToolsView()
{
  //kDebug() << "xml tools descructor 1...";
  //TODO: unregister the model
}

PluginKateXMLToolsCompletionModel::PluginKateXMLToolsCompletionModel( QObject *parent ) : CodeCompletionModel2 (parent)
{
  m_dtdString.clear();
  m_urlString.clear();
  m_docToAssignTo = 0L;

  m_mode = none;
  m_correctPos = 0;

  m_lastLine = 0;
  m_lastCol = 0;
  m_allowed = QStringList();
  m_popupOpenCol = -1;

  setHasGroups( false );
}

PluginKateXMLToolsCompletionModel::~PluginKateXMLToolsCompletionModel()
{
  qDeleteAll( m_dtds );
  m_dtds.clear();
}

void PluginKateXMLToolsCompletionModel::slotDocumentDeleted( KTextEditor::Document *doc )
{
  // Remove the document from m_DTDs, and also delete the PseudoDTD
  // if it becomes unused.
  if ( m_docDtds[ doc ] )
  {
    kDebug()<<"XMLTools:slotDocumentDeleted: documents: "<<m_docDtds.count()<<", DTDs: "<<m_dtds.count();
    PseudoDTD *dtd = m_docDtds.take( doc );

    if ( m_docDtds.key( dtd ) )
      return;

    QHash<QString, PseudoDTD *>::iterator it;
    for ( it = m_dtds.begin() ; it != m_dtds.end() ; ++it )
    {
      if ( it.value() == dtd )
      {
        m_dtds.erase(it);
        delete dtd;
        return;
      }
    }
  }
}

void PluginKateXMLToolsCompletionModel::backspacePressed()
{
  kDebug() << "xml tools backspacePressed";

  if ( !application()->activeMainWindow() )
    return;

  KTextEditor::View *kv = application()->activeMainWindow()->activeView();
  if( ! kv )
  {
    kDebug() << "Warning: no KTextEditor::View";
    return;
  }
  int line, col;
  kv->cursorPosition().position ( line, col );

  //kDebug() << "++ redisplay popup? line:" << line << ", col: " << col;
  if( m_lastLine == line && col == m_lastCol )
  {
    int len = col - m_popupOpenCol;
    if( len < 0 )
    {
      kDebug() << "**Warning: len < 0";
      return;
    }
    //connectSlots( kv );
    //TODO:redisplay the popup here
  }
}

void PluginKateXMLToolsCompletionModel::emptyKeyEvent()
{
//  keyEvent( 0, 0, QString() );
}

void PluginKateXMLToolsCompletionModel::completionInvoked( KTextEditor::View *kv,
                                                           const KTextEditor::Range &range, InvocationType invocationType )
{
  kDebug() << "xml tools completionInvoked";
  //kDebug() << "xml tools keyEvent: '" << s;

  KTextEditor::Document *doc = kv->document();
  if( ! m_docDtds[ doc ] )
    // no meta DTD assigned yet
    return;

  // debug to test speed:
  //QTime t; t.start();

  beginResetModel();
  m_allowed.clear();

  // get char on the left of the cursor:
  KTextEditor::Cursor curpos = kv->cursorPosition();
  uint line = curpos.line(), col = curpos.column();

  QString lineStr = kv->document()->line( line );
  QString leftCh = lineStr.mid( col-1, 1 );
  QString secondLeftCh = lineStr.mid( col-2, 1 );

  if( leftCh == "&" )
  {
    kDebug() << "Getting entities";
    m_allowed = m_docDtds[doc]->entities("" );
    m_mode = entities;
  }
  else if( leftCh == "<" )
  {
    kDebug() << "*outside tag -> get elements";
    QString parentElement = getParentElement( *kv, 1 );
    kDebug() << "parent: " << parentElement;
    m_allowed = m_docDtds[doc]->allowedElements(parentElement );
    m_mode = elements;
  }
  // TODO: optionally close parent tag if not left=="/>"
  else if( leftCh == " " || (isQuote(leftCh) && secondLeftCh == "=") )
  {
    // TODO: check secondLeftChar, too?! then you don't need to trigger
    // with space and we yet save CPU power
    QString currentElement = insideTag( *kv );
    QString currentAttribute;
    if( ! currentElement.isEmpty() )
      currentAttribute = insideAttribute( *kv );

    kDebug() << "Tag: " << currentElement;
    kDebug() << "Attr: " << currentAttribute;

    if( ! currentElement.isEmpty() && ! currentAttribute.isEmpty() )
    {
      kDebug() << "*inside attribute -> get attribute values";
      m_allowed = m_docDtds[doc]->attributeValues(currentElement, currentAttribute );
      if( m_allowed.count() == 1 &&
          (m_allowed[0] == "CDATA" || m_allowed[0] == "ID" || m_allowed[0] == "IDREF" ||
          m_allowed[0] == "IDREFS" || m_allowed[0] == "ENTITY" || m_allowed[0] == "ENTITIES" ||
          m_allowed[0] == "NMTOKEN" || m_allowed[0] == "NMTOKENS" || m_allowed[0] == "NAME") )
      {
        // these must not be taken literally, e.g. don't insert the string "CDATA"
        m_allowed.clear();
      }
      else
      {
        m_mode = attributevalues;
      }
    }
    else if( ! currentElement.isEmpty() )
    {
      kDebug() << "*inside tag -> get attributes";
      m_allowed = m_docDtds[doc]->allowedAttributes(currentElement );
      m_mode = attributes;
    }
  }

  //kDebug() << "time elapsed (ms): " << t.elapsed();
  kDebug() << "Allowed strings: " << m_allowed.count();

  if( m_allowed.count() >= 1 && m_allowed[0] != "__EMPTY" )
  {
    m_allowed = sortQStringList( m_allowed );
    //connectSlots( kv );
//    kv->showCompletionBox( stringListToCompletionEntryList( m_allowed ), 0, false );
    m_popupOpenCol = col;
  }
  setRowCount( m_allowed.count() );
  endResetModel();
}

QVariant PluginKateXMLToolsCompletionModel::data(const QModelIndex &idx, int role) const
{
  //kDebug() << "XMLToolsCompletionModel::data index " << idx << " role " << role;
  if( (idx.column() == KTextEditor::CodeCompletionModel::Name) && (role == Qt::DisplayRole) )
    return m_allowed.at( idx.row() );
  else
    return QVariant();
}


bool PluginKateXMLToolsCompletionModel::shouldStartCompletion( KTextEditor::View *view,
                                                               const QString &insertedText,
                                                               bool userInsertion,
                                                               const KTextEditor::Cursor &position )
{
  Q_UNUSED( view )
  Q_UNUSED( userInsertion )
  Q_UNUSED( position )
  const QString triggerChars = "&< '\""; // these are subsequently handled by completionInvoked()

  return triggerChars.contains( insertedText.right(1) );
}


/**
 * Load the meta DTD. In case of success set the 'ready'
 * flag to true, to show that we're is ready to give hints about the DTD.
 */
void PluginKateXMLToolsCompletionModel::getDTD()
{
  if ( !application()->activeMainWindow() )
    return;

  KTextEditor::View *kv = application()->activeMainWindow()->activeView();
  if( ! kv )
  {
    kDebug() << "Warning: no KTextEditor::View";
    return;
  }

  // ### replace this with something more sane
  // Start where the supplied XML-DTDs are fed by default unless
  // user changed directory last time:

  QString defaultDir = KGlobal::dirs()->findResourceDir("data", "katexmltools/" ) + "katexmltools/";
  if( m_urlString.isNull() ) {
    m_urlString = defaultDir;
  }
  KUrl url;

  // Guess the meta DTD by looking at the doctype's public identifier.
  // XML allows comments etc. before the doctype, so look further than
  // just the first line.
  // Example syntax:
  // <!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "DTD/xhtml1-transitional.dtd">
  uint checkMaxLines = 200;
  QString documentStart = kv->document()->text( KTextEditor::Range(0, 0, checkMaxLines+1, 0) );
  QRegExp re( "<!DOCTYPE\\s+(.*)\\s+PUBLIC\\s+[\"'](.*)[\"']", Qt::CaseInsensitive );
  re.setMinimal( true );
  int matchPos = re.indexIn( documentStart );
  QString filename;
  QString doctype;
  QString topElement;

  if( matchPos != -1 ) {
    topElement = re.cap( 1 );
    doctype = re.cap( 2 );
    kDebug() << "Top element: " << topElement;
    kDebug() << "Doctype match: " << doctype;
    // XHTML:
    if( doctype == "-//W3C//DTD XHTML 1.0 Transitional//EN" )
      filename = "xhtml1-transitional.dtd.xml";
    else if( doctype == "-//W3C//DTD XHTML 1.0 Strict//EN" )
      filename = "xhtml1-strict.dtd.xml";
    else if( doctype == "-//W3C//DTD XHTML 1.0 Frameset//EN" )
      filename = "xhtml1-frameset.dtd.xml";
    // HTML 4.0:
    else if ( doctype == "-//W3C//DTD HTML 4.01 Transitional//EN" )
      filename = "html4-loose.dtd.xml";
    else if ( doctype == "-//W3C//DTD HTML 4.01//EN" )
      filename = "html4-strict.dtd.xml";
    // KDE Docbook:
    else if ( doctype == "-//KDE//DTD DocBook XML V4.1.2-Based Variant V1.1//EN" )
      filename = "kde-docbook.dtd.xml";
  }
  else if( documentStart.indexOf("<xsl:stylesheet" ) != -1 &&
             documentStart.indexOf( "xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\"") != -1 )
  {
    /* XSLT doesn't have a doctype/DTD. We look for an xsl:stylesheet tag instead.
      Example:
      <xsl:stylesheet version="1.0"
      xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
      xmlns="http://www.w3.org/TR/xhtml1/strict">
    */
    filename = "xslt-1.0.dtd.xml";
    doctype = "XSLT 1.0";
  }
  else
    kDebug() << "No doctype found";

  if( filename.isEmpty() )
  {
    // no meta dtd found for this file
    url = KFileDialog::getOpenUrl(m_urlString, "*.xml",
                                  0, i18n( "Assign Meta DTD in XML Format") );
  }
  else
  {
    url.setFileName( defaultDir + filename );
    KMessageBox::information(0, i18n("The current file has been identified "
        "as a document of type \"%1\". The meta DTD for this document type "
            "will now be loaded.", doctype ),
        i18n( "Loading XML Meta DTD" ),
        QString::fromLatin1( "DTDAssigned") );
  }

  if( url.isEmpty() )
    return;

  m_urlString = url.url();	// remember directory for next time

  if ( m_dtds[ m_urlString ] )
    assignDTD( m_dtds[ m_urlString ], kv->document() );
  else
  {
    m_dtdString = "";
    m_docToAssignTo = kv->document();

    KApplication::setOverrideCursor( Qt::WaitCursor );
    KIO::Job *job = KIO::get( url );
    connect( job, SIGNAL(result(KJob *)), this, SLOT(slotFinished(KJob *)) );
    connect( job, SIGNAL(data(KIO::Job *, const QByteArray &)),
             this, SLOT(slotData(KIO::Job *, const QByteArray &)) );
  }
  kDebug()<<"XMLTools::getDTD: Documents: "<<m_docDtds.count()<<", DTDs: "<<m_dtds.count();
}

void PluginKateXMLToolsCompletionModel::slotFinished( KJob *job )
{
  if( job->error() )
  {
    //kDebug() << "XML Plugin error: DTD in XML format (" << filename << " ) could not be loaded";
    static_cast<KIO::Job*>(job)->ui()->showErrorMessage();
  }
  else if ( static_cast<KIO::TransferJob *>(job)->isErrorPage() )
  {
    // catch failed loading loading via http:
    KMessageBox::error(0, i18n("The file '%1' could not be opened. "
        "The server returned an error.", m_urlString ),
        i18n( "XML Plugin Error") );
  }
  else
  {
    PseudoDTD *dtd = new PseudoDTD();
    dtd->analyzeDTD( m_urlString, m_dtdString );

    m_dtds.insert( m_urlString, dtd );
    assignDTD( dtd, m_docToAssignTo );

    // clean up a bit
    m_docToAssignTo = 0;
    m_dtdString = "";
  }
  QApplication::restoreOverrideCursor();
}

void PluginKateXMLToolsCompletionModel::slotData( KIO::Job *, const QByteArray &data )
{
  m_dtdString += QString( data );
}

void PluginKateXMLToolsCompletionModel::assignDTD( PseudoDTD *dtd, KTextEditor::Document *doc )
{
  m_docDtds.insert( doc, dtd );

  KTextEditor::CodeCompletionInterface *cci = qobject_cast<KTextEditor::CodeCompletionInterface *>(doc->activeView()); //TODO:perhaps foreach views()?

  if( cci ) {
    cci->registerCompletionModel( this );
    cci->setAutomaticInvocationEnabled( true );
    kDebug() << "PluginKateXMLToolsView: completion model registered";
  }
  else {
    kWarning() << "PluginKateXMLToolsView: completion interface unavailable";
  }
}

/**
 * Offer a line edit with completion for possible elements at cursor position and insert the
 * tag one chosen/entered by the user, plus its closing tag. If there's a text selection,
 * add the markup around it.
 */
void PluginKateXMLToolsCompletionModel::slotInsertElement()
{
  if ( !application()->activeMainWindow() )
    return;

  KTextEditor::View *kv = application()->activeMainWindow()->activeView();
  if( ! kv )
  {
    kDebug() << "Warning: no KTextEditor::View";
    return;
  }

  PseudoDTD *dtd = m_docDtds[kv->document()];
  QString parentElement = getParentElement( *kv, 0 );
  QStringList allowed;

  if( dtd )
    allowed = dtd->allowedElements(parentElement );

  InsertElement *dialog = new InsertElement(
      ( QWidget *)application()->activeMainWindow()->activeView(), "insertXml" );
  QString text = dialog->showDialog( allowed );
  delete dialog;

  if( !text.isEmpty() )
  {
    QStringList list = text.split( QChar(' ') );
    QString pre;
    QString post;
    // anders: use <tagname/> if the tag is required to be empty.
    // In that case maybe we should not remove the selection? or overwrite it?
    int adjust = 0; // how much to move cursor.
    // if we know that we have attributes, it goes
    // just after the tag name, otherwise between tags.
    if ( dtd && dtd->allowedAttributes(list[0]).count() )
      adjust++;   // the ">"

    if ( dtd && dtd->allowedElements(list[0]).contains("__EMPTY") )
    {
      pre = '<' + text + "/>";
      if ( adjust )
        adjust++; // for the "/"
    }
    else
    {
      pre = '<' + text + '>';
      post ="</" + list[0] + '>';
    }

    QString marked;
    if ( ! post.isEmpty() )
      marked = kv->selectionText();

    if( marked.length() > 0 )
      kv->removeSelectionText();

    kv->insertText( pre + marked + post );
  }
}

/**
 * Insert a closing tag for the nearest not-closed parent element.
 */
void PluginKateXMLToolsCompletionModel::slotCloseElement()
{
  if ( !application()->activeMainWindow() )
    return;

  KTextEditor::View *kv = application()->activeMainWindow()->activeView();
  if( ! kv )
  {
    kDebug() << "Warning: no KTextEditor::View";
    return;
  }
  QString parentElement = getParentElement( *kv, 0 );

  //kDebug() << "parentElement: '" << parentElement << "'";
  QString closeTag = "</" + parentElement + '>';
  if( ! parentElement.isEmpty() )
    kv->insertText( closeTag );
}


// modify the completion string before it gets inserted
void PluginKateXMLToolsCompletionModel::executeCompletionItem2( KTextEditor::Document *document,
                                                                const KTextEditor::Range &word,
                                                                const QModelIndex &index ) const
{
  KTextEditor::Range toReplace = word;

  QString text = data( index.sibling( index.row(), Name ), Qt::DisplayRole ).toString();

  kDebug() << "executeCompletionItem text: " << text;

  KTextEditor::View *kv = document->activeView();
  if( ! kv )
  {
    kDebug() << "Warning (filterInsertString() ): no KTextEditor::View";
    return;
  }

  int line, col;
  kv->cursorPosition().position( line, col );
  QString lineStr = document->line( line );
  QString leftCh = lineStr.mid( col-1, 1 );
  QString rightCh = lineStr.mid( col, 1 );

  int posCorrection = 0;	// where to move the cursor after completion ( >0 = move right )
  if( m_mode == entities )
  {
    // This is a bit ugly, but entities are case-sensitive
    // and we want the correct completion even if the user started typing
    // e.g. in lower case but the entity is in upper case
    // document->removeText( line, col - (ce->text.length() - text->length()), line, col );
    text = text + ';';
  }

  else if( m_mode == attributes )
  {
    text = text + "=\"\"";
    posCorrection = -1;
    if( !rightCh.isEmpty() && rightCh != ">" && rightCh != "/" && rightCh != " " )
    {	// TODO: other whitespaces
      // add space in front of the next attribute
      text = text + ' ';
      posCorrection--;
    }
  }

  else if( m_mode == attributevalues )
  {
    // TODO: support more than one line
    uint startAttValue = 0;
    uint endAttValue = 0;

    // find left quote:
    for( startAttValue = col; startAttValue > 0; startAttValue-- )
    {
      QString ch = lineStr.mid( startAttValue-1, 1 );
      if( isQuote(ch) )
        break;
    }

    // find right quote:
    for( endAttValue = col; endAttValue <= (uint) lineStr.length(); endAttValue++ )
    {
      QString ch = lineStr.mid( endAttValue-1, 1 );
      if( isQuote(ch) )
        break;
    }

    // replace the current contents of the attribute
    if( startAttValue < endAttValue )
      toReplace = KTextEditor::Range( line, startAttValue, line, endAttValue-1 );
  }

  else if( m_mode == elements )
  {
    // anders: if the tag is marked EMPTY, insert in form <tagname/>
    QString str;
    bool isEmptyTag =m_docDtds[document]->allowedElements(text).contains( "__EMPTY" );
    if ( isEmptyTag )
      str = text + "/>";
    else
      str = text + "></" + text + '>';

    // Place the cursor where it is most likely wanted:
    // always inside the tag if the tag is empty AND the DTD indicates that there are attribs)
    // outside for open tags, UNLESS there are mandatory attributes
    if ( m_docDtds[document]->requiredAttributes(text).count()
         || ( isEmptyTag && m_docDtds[document]->allowedAttributes(text).count() ) )
      posCorrection = text.length() - str.length();
    else if ( ! isEmptyTag )
      posCorrection = text.length() - str.length() + 1;

    text = str;
  }

  document->replaceText( toReplace, text );

  // move the cursor to desired position
  KTextEditor::Cursor curPos = kv->cursorPosition();
  curPos.setColumn( curPos.column() + posCorrection );
  kv->setCursorPosition( curPos );
}


// ========================================================================
// Pseudo-XML stuff:

/**
 * Check if cursor is inside a tag, that is
 * if "<" occurs before ">" occurs ( on the left side of the cursor ).
 * Return the tag name, return "" if we cursor is outside a tag.
 */
QString PluginKateXMLToolsCompletionModel::insideTag( KTextEditor::View &kv )
{
  int line, col;
  kv.cursorPosition().position( line, col );
  int y = line;	// another variable because uint <-> int

  do {
    QString lineStr = kv.document()->line(y );
    for( uint x = col; x > 0; x-- )
    {
      QString ch = lineStr.mid( x-1, 1 );
      if( ch == ">" )   // cursor is outside tag
        return "";

      if( ch == "<" )
      {
        QString tag;
        // look for white space on the right to get the tag name
        for( int z = x; z <= lineStr.length() ; ++z )
        {
          ch = lineStr.mid( z-1, 1 );
          if( ch.at(0).isSpace() || ch == "/" || ch == ">" )
            return tag.right( tag.length()-1 );

          if( z == lineStr.length() )
          {
            tag += ch;
            return tag.right( tag.length()-1 );
          }

          tag += ch;
        }
      }
    }
    y--;
    col = kv.document()->line(y).length();
  } while( y >= 0 );

  return "";
}

/**
 * Check if cursor is inside an attribute value, that is
 * if '="' is on the left, and if it's nearer than "<" or ">".
 *
 * @Return the attribute name or "" if we're outside an attribute
 * value.
 *
 * Note: only call when insideTag() == true.
 * TODO: allow whitespace around "="
 */
QString PluginKateXMLToolsCompletionModel::insideAttribute( KTextEditor::View &kv )
{
  int line, col;
  kv.cursorPosition().position( line, col );
  int y = line;	// another variable because uint <-> int
  uint x = 0;
  QString lineStr = "";
  QString ch = "";

  do {
    lineStr = kv.document()->line(y );
    for( x = col; x > 0; x-- )
    {
      ch = lineStr.mid( x-1, 1 );
      QString chLeft = lineStr.mid( x-2, 1 );
      // TODO: allow whitespace
      if( isQuote(ch) && chLeft == "=" )
        break;
      else if( isQuote(ch) && chLeft != "=" )
        return "";
      else if( ch == "<" || ch == ">" )
        return "";
    }
    y--;
    col = kv.document()->line(y).length();
  } while( !isQuote(ch) );

  // look for next white space on the left to get the tag name
  QString attr = "";
  for( int z = x; z >= 0; z-- )
  {
    ch = lineStr.mid( z-1, 1 );

    if( ch.at(0).isSpace() )
      break;

    if( z == 0 )
    {   // start of line == whitespace
      attr += ch;
      break;
    }

    attr = ch + attr;
  }

  return attr.left( attr.length()-2 );
}

/**
 * Find the parent element for the current cursor position. That is,
 * go left and find the first opening element that's not closed yet,
 * ignoring empty elements.
 * Examples: If cursor is at "X", the correct parent element is "p":
 * <p> <a x="xyz"> foo <i> test </i> bar </a> X
 * <p> <a x="xyz"> foo bar </a> X
 * <p> foo <img/> bar X
 * <p> foo bar X
 */
QString PluginKateXMLToolsCompletionModel::getParentElement( KTextEditor::View &kv, int skipCharacters )
{
  enum {
    parsingText,
    parsingElement,
    parsingElementBoundary,
    parsingNonElement,
    parsingAttributeDquote,
    parsingAttributeSquote,
    parsingIgnore
  } parseState;
  parseState = (skipCharacters > 0) ? parsingIgnore : parsingText;

  int nestingLevel = 0;

  int line, col;
  kv.cursorPosition().position( line, col );
  QString str = kv.document()->line(line );

  while( true )
  {
    // move left a character
    if( !col-- )
    {
      do
      {
        if( !line-- ) return QString(); // reached start of document
        str = kv.document()->line(line );
        col = str.length();
      } while( !col );
      --col;
    }

    ushort ch = str.at( col).unicode();

    switch( parseState )
    {
      case parsingIgnore:
        // ignore the specified number of characters
        parseState = ( --skipCharacters > 0 ) ? parsingIgnore : parsingText;
        break;

      case parsingText:
        switch( ch )
        {
          case '<':
            // hmm... we were actually inside an element
            return QString();

          case '>':
            // we just hit an element boundary
            parseState = parsingElementBoundary;
            break;
        }
        break;

      case parsingElement:
        switch( ch )
        {
          case '"': // attribute ( double quoted )
            parseState = parsingAttributeDquote;
            break;

            case '\'': // attribute ( single quoted )
              parseState = parsingAttributeSquote;
              break;

              case '/': // close tag
                parseState = parsingNonElement;
                ++nestingLevel;
                break;

          case '<':
            // we just hit the start of the element...
            if( nestingLevel-- ) break;

            QString tag = str.mid( col + 1 );
            for( uint pos = 0, len = tag.length(); pos < len; ++pos ) {
              ch = tag.at( pos).unicode();
              if( ch == ' ' || ch == '\t' || ch == '>' ) {
                tag.truncate( pos );
                break;
              }
            }
            return tag;
        }
        break;

      case parsingElementBoundary:
        switch( ch )
        {
          case '?': // processing instruction
          case '-': // comment
          case '/': // empty element
            parseState = parsingNonElement;
            break;

          case '"':
            parseState = parsingAttributeDquote;
            break;

          case '\'':
            parseState = parsingAttributeSquote;
            break;

          case '<': // empty tag ( bad XML )
            parseState = parsingText;
            break;

          default:
            parseState = parsingElement;
        }
        break;

      case parsingAttributeDquote:
        if( ch == '"' ) parseState = parsingElement;
        break;

      case parsingAttributeSquote:
        if( ch == '\'' ) parseState = parsingElement;
        break;

      case parsingNonElement:
        if( ch == '<' ) parseState = parsingText;
        break;
    }
  }
}

/**
 * Return true if the tag is neither a closing tag
 * nor an empty tag, nor a comment, nor processing instruction.
 */
bool PluginKateXMLToolsCompletionModel::isOpeningTag( QString tag )
{
  return ( !isClosingTag(tag) && !isEmptyTag(tag ) &&
      !tag.startsWith( "<?") && !tag.startsWith("<!") );
}

/**
 * Return true if the tag is a closing tag. Return false
 * if the tag is an opening tag or an empty tag ( ! )
 */
bool PluginKateXMLToolsCompletionModel::isClosingTag( QString tag )
{
  return ( tag.startsWith("</") );
}

bool PluginKateXMLToolsCompletionModel::isEmptyTag( QString tag )
{
  return ( tag.right(2) == "/>" );
}

/**
 * Return true if ch is a single or double quote. Expects ch to be of length 1.
 */
bool PluginKateXMLToolsCompletionModel::isQuote( QString ch )
{
  return ( ch == "\"" || ch == "'" );
}


// ========================================================================
// Tools:

/** Sort a QStringList case-insensitively. Static. TODO: make it more simple. */
QStringList PluginKateXMLToolsCompletionModel::sortQStringList( QStringList list ) {
  // Sort list case-insensitive. This looks complicated but using a QMap
  // is even suggested by the Qt documentation.
  QMap<QString,QString> mapList;
  for ( QStringList::Iterator it = list.begin(); it != list.end(); ++it )
  {
    QString str = *it;
    if( mapList.contains(str.toLower()) )
    {
      // do not override a previous value, e.g. "Auml" and "auml" are two different
      // entities, but they should be sorted next to each other.
      // TODO: currently it's undefined if e.g. "A" or "a" comes first, it depends on
      // the meta DTD ( really? it seems to work okay?!? )
      mapList[str.toLower()+'_'] = str;
    }
    else
      mapList[str.toLower()] = str;
  }

  list.clear();
  QMap<QString,QString>::Iterator it;

  // Qt doc: "the items are alphabetically sorted [by key] when iterating over the map":
  for( it = mapList.begin(); it != mapList.end(); ++it )
    list.append( it.value() );

  return list;
}

//BEGIN InsertElement dialog
InsertElement::InsertElement( QWidget *parent, const char *name )
  :KDialog( parent)
{
  setCaption(i18n("Insert XML Element" ));
  setButtons(KDialog::Ok|KDialog::Cancel);
  setDefaultButton(KDialog::Cancel);
  setModal(true);
}

InsertElement::~InsertElement()
{
}

void InsertElement::slotHistoryTextChanged( const QString& text )
{
  enableButtonOk( !text.isEmpty() );
}

QString InsertElement::showDialog( QStringList &completions )
{
  QWidget *page = new QWidget( this );
  setMainWidget( page );
  QVBoxLayout *topLayout = new QVBoxLayout( page );

  KHistoryComboBox *combo = new KHistoryComboBox( page );
  combo->setHistoryItems( completions, true );
  connect( combo->lineEdit(), SIGNAL(textChanged ( const QString & )),
           this, SLOT(slotHistoryTextChanged(const QString &)) );
  QString text = i18n( "Enter XML tag name and attributes (\"<\", \">\" and closing tag will be supplied):" );
  QLabel *label = new QLabel( text, page );

  topLayout->addWidget( label );
  topLayout->addWidget( combo );

  combo->setFocus();
  slotHistoryTextChanged( combo->lineEdit()->text() );
  if( exec() )
    return combo->currentText();

  return QString();
}
//END InsertElement dialog
// kate: space-indent on; indent-width 2; replace-tabs on; mixed-indent off;
