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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "insertfileplugin.h"
#include "insertfileplugin.moc"

#include <ktexteditor/document.h>

#include <assert.h>
#include <kio/job.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <kfiledialog.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpushbutton.h>
#include <ktemporaryfile.h>
#include <kurl.h>

#include <QtCore/QFile>
#include <QtCore/QTextStream>

K_PLUGIN_FACTORY( InsertFilePluginFactory, registerPlugin<InsertFilePlugin>(); )
K_EXPORT_PLUGIN( InsertFilePluginFactory( "ktexteditor_insertfile", "ktexteditor_plugins" ) )

//BEGIN InsertFilePlugin
InsertFilePlugin::InsertFilePlugin( QObject *parent, const QVariantList& )
	: KTextEditor::Plugin ( parent )
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
    int z=0;
    // Loop written for the unlikely case of a view being added more than once
    while (z < m_views.count())
    {
      InsertFilePluginView *nview = m_views.at(z);
      if (nview->parentClient() == view)
      {
         m_views.removeAll (nview);
         delete nview;
      }
      else
         ++z;
    }
}
//END InsertFilePlugin

//BEGIN InsertFilePluginView
InsertFilePluginView::InsertFilePluginView( KTextEditor::View *view, const char *name )
  : QObject( view ),
    KXMLGUIClient( view )
{
  setObjectName( name );

  view->insertChildClient( this );
  setComponentData( InsertFilePluginFactory::componentData() );
  _job = 0;

  KAction *action = new KAction( i18n("Insert File..."), this );
  actionCollection()->addAction( "tools_insert_file", action );
  connect( action, SIGNAL( triggered( bool ) ), this, SLOT(slotInsertFile()) );

  setXMLFile( "ktexteditor_insertfileui.rc" );
}

void InsertFilePluginView::slotInsertFile()
{
  KFileDialog dlg( KUrl( "kfiledialog:///insertfile?global" ), "", (QWidget*)parent());
  dlg.setOperationMode( KFileDialog::Opening );

  dlg.setCaption(i18n("Choose File to Insert"));
  dlg.okButton()->setText(i18n("&Insert"));
  dlg.setMode( KFile::File );
  dlg.exec();

  _file = dlg.selectedUrl().url();
  if ( _file.isEmpty() ) return;

  if ( _file.isLocalFile() ) {
    _tmpfile = _file.path();
    insertFile();
  }
  else {
    KTemporaryFile tempFile;
    tempFile.setAutoRemove(false);
    tempFile.open();
    _tmpfile = tempFile.fileName();

    KUrl destURL;
    destURL.setPath( _tmpfile );
    _job = KIO::file_copy( _file, destURL, 0600, KIO::Overwrite );
    connect( _job, SIGNAL( result( KJob * ) ), this, SLOT( slotFinished ( KJob * ) ) );
  }
}

void InsertFilePluginView::slotFinished( KJob *job )
{
  assert( job == _job );
  _job = 0;
  if ( job->error() )
    KMessageBox::error( (QWidget*)parent(), i18n("Failed to load file:\n\n") + job->errorString(), i18n("Insert File Error") );
  else
    insertFile();
}

void InsertFilePluginView::insertFile()
{
  QString error;
  if ( _tmpfile.isEmpty() )
    return;

  QFileInfo fi;
  fi.setFile( _tmpfile );
  if (!fi.exists() || !fi.isReadable())
    error = i18n("<p>The file <strong>%1</strong> does not exist or is not readable, aborting.</p>", _file.fileName());

  QFile f( _tmpfile );
  if ( !f.open(QIODevice::ReadOnly) )
    error = i18n("<p>Unable to open file <strong>%1</strong>, aborting.</p>", _file.fileName());

  if ( ! error.isEmpty() ) {
    KMessageBox::sorry( (QWidget*)parent(), error, i18n("Insert File Error") );
    return;
  }

  // now grab file contents
  QTextStream stream(&f);
  QString str, tmp;
  uint numlines = 0;
  uint len = 0;
  while (!stream.atEnd()) {
    if ( numlines )
      str += '\n';
    tmp = stream.readLine();
    str += tmp;
    len = tmp.length();
    numlines++;
  }
  f.close();

  if ( str.isEmpty() )
    error = i18n("<p>File <strong>%1</strong> had no contents.</p>", _file.fileName());
  if ( ! error.isEmpty() ) {
    KMessageBox::sorry( (QWidget*)parent(), error, i18n("Insert File Error") );
    return;
  }

  // insert !!
  KTextEditor::View *v = (KTextEditor::View*)parent();
  int line, col;
  line = v->cursorPosition().line();
  col = v->cursorPosition().column();
  v->document()->insertText( v->cursorPosition(), str );

  // move the cursor
  v->setCursorPosition ( KTextEditor::Cursor (line + numlines - 1, numlines > 1 ? len : col + len)  );

  // clean up
  _file = KUrl ();
  _tmpfile.truncate( 0 );
}

//END InsertFilePluginView

