/* This file is part of the KDE project
Copyright (C) 2010 Dominik Haumann <dhaumann kde org>

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

#include "kwriteapp.h"

#include <ktexteditor/editor.h>
#include <ktexteditor/editorchooser.h>
#include <kcmdlineargs.h>
#include <kmessagebox.h>

#include <QDir>
#include <QTextCodec>

KWriteApp::KWriteApp(KCmdLineArgs *m_args)
  : KApplication ()
  , m_args(m_args)
{
  KTextEditor::ContainerInterface *iface =
    qobject_cast<KTextEditor::ContainerInterface*>(KTextEditor::EditorChooser::editor());
  if (iface) {
    iface->setContainer(this);
  }

  init();
}

KWriteApp::~KWriteApp()
{
}

KWriteApp *KWriteApp::self ()
{
  return (KWriteApp *) kapp;
}

void KWriteApp::init()
{
  if (isSessionRestored())
  {
    KWrite::restore();
  }
  else
  {
    bool nav = false;
    int line = 0, column = 0;

    QTextCodec *codec = m_args->isSet("encoding") ? QTextCodec::codecForName(m_args->getOption("encoding").toLocal8Bit()) : 0;

    if (m_args->isSet ("line"))
    {
      line = m_args->getOption ("line").toInt() - 1;
      nav = true;
    }

    if (m_args->isSet ("column"))
    {
      column = m_args->getOption ("column").toInt() - 1;
      nav = true;
    }

    if ( m_args->count() == 0 )
    {
        KWrite *t = new KWrite;

        if( m_args->isSet( "stdin" ) )
        {
          QTextStream input(stdin, QIODevice::ReadOnly);

          // set chosen codec
          if (codec)
            input.setCodec (codec);

          QString line;
          QString text;

          do
          {
            line = input.readLine();
            text.append( line + '\n' );
          } while( !line.isNull() );


          KTextEditor::Document *doc = t->view()->document();
          if( doc )
              doc->setText( text );
        }

        if (nav && t->view())
          t->view()->setCursorPosition (KTextEditor::Cursor (line, column));
    }
    else
    {
      int docs_opened = 0;
      for ( int z = 0; z < m_args->count(); z++ )
      {
        // this file is no local dir, open it, else warn
        bool noDir = !m_args->url(z).isLocalFile() || !QDir (m_args->url(z).toLocalFile()).exists();

        if (noDir)
        {
          ++docs_opened;
          KWrite *t = new KWrite();

          t->view()->document()->setSuppressOpeningErrorDialogs (true);

          if (codec)
            t->view()->document()->setEncoding(codec->name());

          t->loadURL( m_args->url( z ) );

          t->view()->document()->setSuppressOpeningErrorDialogs (false);

          if (nav)
            t->view()->setCursorPosition (KTextEditor::Cursor (line, column));
        }
        else
        {
          KMessageBox::sorry(0, i18n("The file '%1' could not be opened: it is not a normal file, it is a folder.", m_args->url(z).url()));
        }
      }
      if (!docs_opened) kapp->exit(1); // see http://bugs.kde.org/show_bug.cgi?id=124708
    }
  }

  // no window there, uh, ohh, for example borked session config !!!
  // create at least one !!
  if (KWrite::noWindows())
    new KWrite();
}

//BEGIN KTextEditor::MdiContainer
void KWriteApp::setActiveView(KTextEditor::View *view)
{
  Q_UNUSED(view)
  // NOTE: not implemented
}

KTextEditor::View *KWriteApp::activeView()
{
  KWrite *window = qobject_cast<KWrite*>(activeWindow());
  if (window) {
    return window->view();
  }
  return 0;
//   return KWriteApp::self()->activeMainWindow()->viewManager()->activeView();
}

KTextEditor::Document *KWriteApp::createDocument()
{
  // NOTE: not implemented
  kWarning() << "WARNING: interface call not implemented";
  return 0;
}

bool KWriteApp::closeDocument(KTextEditor::Document *doc)
{
  Q_UNUSED(doc)
  // NOTE: not implemented
  kWarning() << "WARNING: interface call not implemented";
  return false;
}

KTextEditor::View *KWriteApp::createView(KTextEditor::Document *doc)
{
  Q_UNUSED(doc)
  // NOTE: not implemented
  kWarning() << "WARNING: interface call not implemented";
  return 0;
}

bool KWriteApp::closeView(KTextEditor::View *view)
{
  Q_UNUSED(view)
  // NOTE: not implemented
  kWarning() << "WARNING: interface call not implemented";
  return false;
}
//END KTextEditor::MdiContainer

// kate: space-indent on; indent-width 2; replace-tabs on;