/* This file is part of the KDE libraries
   Copyright (C) 2002 Anders Lund <anders@alweb.dk>
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   $Id$
*/

#include "insertfileplugin.h"
#include "insertfileplugin.moc"

#include <ktexteditor/document.h>
#include <ktexteditor/viewcursorinterface.h>
#include <ktexteditor/editinterface.h>

#include <assert.h>
#include <kio/job.h>
#include <kaction.h>
#include <kfiledialog.h>
#include <kgenericfactory.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <ktempfile.h>
#include <kurl.h>

#include <qfile.h>
#include <qtextstream.h>

K_EXPORT_COMPONENT_FACTORY( ktexteditor_insertfile, KGenericFactory<InsertFilePlugin>( "ktexteditor_insertfile" ) );


//BEGIN InsertFilePlugin
InsertFilePlugin::InsertFilePlugin( QObject *parent, const char* name, const QStringList& )
	: KTextEditor::Plugin ( (KTextEditor::Document*) parent, name )
{
}

InsertFilePlugin::~InsertFilePlugin()
{
}

void InsertFilePlugin::addView(KTextEditor::View *view)
{
  InsertFilePluginView *nview = new InsertFilePluginView (view, "Insert File Plugin");
  m_views.append (nview);
}

void InsertFilePlugin::removeView(KTextEditor::View *view)
{
  for (uint z=0; z < m_views.count(); z++)
    if (m_views.at(z)->parentClient() == view)
    {
       InsertFilePluginView *nview = m_views.at(z);
       m_views.remove (nview);
       delete nview;
    }
}
//END InsertFilePlugin

//BEGIN InsertFilePluginView
InsertFilePluginView::InsertFilePluginView( KTextEditor::View *view, const char *name )
  : QObject( view, name ),
    KXMLGUIClient( view )
{
  view->insertChildClient( this );
  setInstance( KGenericFactory<InsertFilePlugin>::instance() );
  _job = 0;
  (void) new KAction( i18n("Insert File"), 0, this, SLOT(slotInsertFile()), actionCollection(), "tools_insert_file" );
  setXMLFile( "ktexteditor_insertfileui.rc" );
}

void InsertFilePluginView::slotInsertFile()
{
  _file = KFileDialog::getOpenURL( "::insertfile", "",
                                             (QWidget*)parent(),
                                             i18n("Choose File to Insert") ).url();
  if ( _file.isEmpty() ) return;

  if ( KURL( _file ).isLocalFile() ) {
    _tmpfile = _file;
    insertFile();
  }
  else {
    KURL url( _file );
    KTempFile tempFile( QString::null );
    _tmpfile = tempFile.name();

    KURL destURL;
    destURL.setPath( _tmpfile );
    _job = KIO::file_copy( url, destURL, 0600, true, false, true );
    connect( _job, SIGNAL( result( KIO::Job * ) ), this, SLOT( slotFinished ( KIO::Job * ) ) );
  }
}

void InsertFilePluginView::slotFinished( KIO::Job *job )
{
  assert( job == _job );
  _job = 0;
  if ( job->error() )
    KMessageBox::error( (QWidget*)parent(), i18n("Failed to load file:\n\n") + job->errorString(), i18n("Insert File error") );
  else
    insertFile();
}

void InsertFilePluginView::insertFile()
{
  QString error;
  if ( _tmpfile.isEmpty() )
    error = i18n("<p>The file <strong>%1</strong> is empty, aborting.").arg(_file);

  QFileInfo fi;
  fi.setFile( _tmpfile );
  if (!fi.exists() || !fi.isReadable())
    error = i18n("<p>The file <strong>%1</strong> does not exist or is not readable, aborting.").arg(_file);

  QFile f( _tmpfile );
  if ( ! f.open(IO_ReadOnly) )
    error = i18n("<p>Unable to open file <strong>%1</strong>, aborting.").arg(_file);

  if ( ! error.isEmpty() ) {
    KMessageBox::sorry( (QWidget*)parent(), error, i18n("Insert file error") );
    return;
  }

  // now grab file contents
  QTextStream stream(&f);
  QString str, tmp;
  uint numlines = 0;
  uint len = 0;
  while (!stream.eof()) {
    if ( numlines )
      str += "\n";
    tmp = stream.readLine();
    str += tmp;
    len = tmp.length();
    numlines++;
  }
  f.close();

  if ( str.isEmpty() )
    error = i18n("<p>File <strong>%1</strong> had no contents.").arg(_file);
  if ( ! error.isEmpty() ) {
    KMessageBox::sorry( (QWidget*)parent(), error, i18n("Insert file error") );
    return;
  }

  // insert !!
  KTextEditor::EditInterface *ei;
  KTextEditor::ViewCursorInterface *ci;
  KTextEditor::View *v = (KTextEditor::View*)parent();
  ei = KTextEditor::editInterface( v->document() );
  ci = KTextEditor::viewCursorInterface( v );
  uint line, col;
  ci->cursorPositionReal( &line, &col );
  ei->insertText( line, col, str );

  // move the cursor
  ci->setCursorPositionReal( line + numlines - 1, numlines > 1 ? len : col + len  );

  // clean up
  _file.truncate( 0 );
  _tmpfile.truncate( 0 );
  v = 0;
  ei = 0;
  ci = 0;
}

//END InsertFilePluginView

