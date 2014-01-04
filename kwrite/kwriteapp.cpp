/* This file is part of the KDE project
 *
 * Copyright (C) 2010 Dominik Haumann <dhaumann kde org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "kwriteapp.h"

#include <ktexteditor/editor.h>
#include <KMessageBox>
#include <KSharedConfig>
#include <KLocalizedString>

#include <QCommandLineParser>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QTextCodec>
#include <QApplication>

Q_DECLARE_LOGGING_CATEGORY(LOG_KWRITE)
Q_LOGGING_CATEGORY(LOG_KWRITE, "kwrite")

KWriteApp *KWriteApp::s_self = 0;

KWriteApp::KWriteApp(const QCommandLineParser &args)
  : m_args(args)
{
  s_self = this;
  
  m_editor = KTextEditor::Editor::instance();
  Q_ASSERT (m_editor);

  // read from global config once
  m_editor->readConfig(KSharedConfig::openConfig().data());

  init();
}

KWriteApp::~KWriteApp()
{
}

KWriteApp *KWriteApp::self ()
{
  return s_self;
}

void KWriteApp::init()
{
  if ( qApp->isSessionRestored() )
  {
    KWrite::restore();
  }
  else
  {
    bool nav = false;
    int line = 0, column = 0;

    QTextCodec *codec = m_args.isSet("encoding") ? QTextCodec::codecForName(m_args.value("encoding").toLocal8Bit()) : 0;

    if (m_args.isSet ("line"))
    {
      line = m_args.value ("line").toInt() - 1;
      nav = true;
    }

    if (m_args.isSet ("column"))
    {
      column = m_args.value ("column").toInt() - 1;
      nav = true;
    }

    if ( m_args.positionalArguments().count() == 0 )
    {
        KWrite *t = new KWrite;

        if( m_args.isSet( "stdin" ) )
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
      Q_FOREACH (const QString positionalArgument, m_args.positionalArguments())
      {
        QUrl url;

        // convert to an url
        QRegExp withProtocol("^[a-zA-Z]+:"); // TODO: remove after Qt supports this on its own
        if (withProtocol.indexIn(positionalArgument) == 0) {
          url = QUrl::fromUserInput(positionalArgument);
        } else {
          url = QUrl::fromLocalFile(positionalArgument);
        }

        // this file is no local dir, open it, else warn
        bool noDir = !url.isLocalFile() || !QFileInfo (url.toLocalFile()).isDir();

        if (noDir)
        {
          ++docs_opened;
          KWrite *t = new KWrite();

          t->view()->document()->setSuppressOpeningErrorDialogs (true);

          if (codec)
            t->view()->document()->setEncoding(codec->name());

          t->loadURL( url );

          t->view()->document()->setSuppressOpeningErrorDialogs (false);

          if (nav)
            t->view()->setCursorPosition (KTextEditor::Cursor (line, column));
        }
        else
        {
          KMessageBox::sorry(0, i18n("The file '%1' could not be opened: it is not a normal file, it is a folder.", url.toString()));
        }
      }
      if (!docs_opened) ::exit(1); // see http://bugs.kde.org/show_bug.cgi?id=124708
    }
  }
  
  // no window there, uh, ohh, for example borked session config !!!
  // create at least one !!
  if (KWrite::noWindows())
    new KWrite();
}

// kate: space-indent on; indent-width 2; replace-tabs on;

