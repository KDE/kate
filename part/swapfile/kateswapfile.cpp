/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2010 Dominik Haumann <dhaumann kde org>
 *  Copyright (C) 2010 Diana-Victoria Tiriplica <diana.tiriplica@gmail.com>
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

#include "kateswapfile.h"
#include "kateview.h"

#include <kde_file.h>

#include <QFileInfo>
#include <QDir>

// swap file version header
const static char * const swapFileVersionString = "Kate Swap File - Version 1.0";

// tokens for swap files
const static qint8 EA_StartEditing  = 'S';
const static qint8 EA_FinishEditing = 'E';
const static qint8 EA_WrapLine      = 'W';
const static qint8 EA_UnwrapLine    = 'U';
const static qint8 EA_InsertText    = 'I';
const static qint8 EA_RemoveText    = 'R';

namespace Kate {

SwapFile::SwapFile(KateDocument *document)
  : QObject(document)
  , m_document(document)
  , m_trackingEnabled(false)
{
  // fixed version of serialisation
  m_stream.setVersion (QDataStream::Qt_4_6);
  
  // connecting the signals
  connect(&m_document->buffer(), SIGNAL(saved(const QString &)), this, SLOT(fileSaved(const QString&)));
  connect(&m_document->buffer(), SIGNAL(loaded(const QString &, bool)), this, SLOT(fileLoaded(const QString&)));
  
  setTrackingEnabled(true);
}

SwapFile::~SwapFile()
{
  removeSwapFile();
}

void SwapFile::setTrackingEnabled(bool enable)
{
  if (m_trackingEnabled == enable) {
      return;
  }

  m_trackingEnabled = enable;

  TextBuffer &buffer = m_document->buffer();
 
  if (m_trackingEnabled) {
    connect(&buffer, SIGNAL(editingStarted()), this, SLOT(startEditing()));
    connect(&buffer, SIGNAL(editingFinished()), this, SLOT(finishEditing()));

    connect(&buffer, SIGNAL(lineWrapped(const KTextEditor::Cursor&)), this, SLOT(wrapLine(const KTextEditor::Cursor&)));
    connect(&buffer, SIGNAL(lineUnwrapped(int)), this, SLOT(unwrapLine(int)));
    connect(&buffer, SIGNAL(textInserted(const KTextEditor::Cursor &, const QString &)), this, SLOT(insertText(const KTextEditor::Cursor &, const QString &)));
    connect(&buffer, SIGNAL(textRemoved(const KTextEditor::Range &, const QString &)), this, SLOT(removeText(const KTextEditor::Range &)));
  } else {
    disconnect(&buffer, SIGNAL(editingStarted()), this, SLOT(startEditing()));
    disconnect(&buffer, SIGNAL(editingFinished()), this, SLOT(finishEditing()));

    disconnect(&buffer, SIGNAL(lineWrapped(const KTextEditor::Cursor&)), this, SLOT(wrapLine(const KTextEditor::Cursor&)));
    disconnect(&buffer, SIGNAL(lineUnwrapped(int)), this, SLOT(unwrapLine(int)));
    disconnect(&buffer, SIGNAL(textInserted(const KTextEditor::Cursor &, const QString &)), this, SLOT(insertText(const KTextEditor::Cursor &, const QString &)));
    disconnect(&buffer, SIGNAL(textRemoved(const KTextEditor::Range &, const QString &)), this, SLOT(removeText(const KTextEditor::Range &)));
  }
}

void SwapFile::fileClosed ()
{
  // remove old swap file, file is now closed
  removeSwapFile();
  
  // purge filename
  updateFileName();
}

void SwapFile::fileLoaded(const QString&)
{
  // look for swap file
  if (!updateFileName())
    return;

  if (!m_swapfile.exists())
  {
    kDebug (13020) << "No swap file";
    return;
  }

  if (!QFileInfo(m_swapfile).isReadable())
  {
    kWarning( 13020 ) << "Can't open swap file (missing permissions)";
    return;
  }

  // emit signal in case the document has more views
  emit swapFileFound();
}

void SwapFile::recover()
{
  // disconnect current signals
  setTrackingEnabled(false);

  // replay the swap file
  if (!m_swapfile.open(QIODevice::ReadOnly))
  {
    kWarning( 13020 ) << "Can't open swap file";
    return;
  }
    
  // open data stream
  m_stream.setDevice(&m_swapfile);
  
  // read and check header
  QByteArray header;
  m_stream >> header;
  if (header != swapFileVersionString)
  {
    m_stream.setDevice (0);
    m_swapfile.close ();
    kWarning( 13020 ) << "Can't open swap file, wrong version";
    return;
  }
  
  // replay swapfile
  bool editStarted = false;
  while (!m_stream.atEnd()) {
    qint8 type;
    m_stream >> type;
    switch (type) {
      case EA_StartEditing: {
        m_document->editStart();
        editStarted = true;
        break;
      }
      case EA_FinishEditing: {
        m_document->editEnd();
        editStarted = false;
        break;
      }
      case EA_WrapLine: {
        int line = 0, column = 0;
        m_stream >> line >> column;
        
        // emulate buffer unwrapLine with document
        m_document->editWrapLine(line, column, true);
        break;
      }
      case EA_UnwrapLine: {
        int line = 0;
        m_stream >> line;
        
        // assert valid line
        Q_ASSERT (line > 0);
        
        // emulate buffer unwrapLine with document
        m_document->editUnWrapLine(line - 1, true, 0);
        break;
      }
      case EA_InsertText: {
        int line, column;
        QByteArray text;
        m_stream >> line >> column >> text;
        m_document->insertText(KTextEditor::Cursor(line, column), QString::fromUtf8 (text.data (), text.size()));
        break;
      }
      case EA_RemoveText: {
        int line, startColumn, endColumn;
        m_stream >> line >> startColumn >> endColumn;
        m_document->removeText (KTextEditor::Range(KTextEditor::Cursor(line, startColumn), KTextEditor::Cursor(line, endColumn)));
        break;
      }
      default: {
        kWarning( 13020 ) << "Unknown type:" << type;
      }
    }
  }

  // balance editStart and editEnd
  if (editStarted) {
    kWarning ( 13020 ) << "Some data might be lost";
    m_document->editEnd();
  }
  
  // close swap file
  m_stream.setDevice(0);
  m_swapfile.close();

  // reconnect the signals
  setTrackingEnabled(true);

  // emit signal in case the document has more views
  emit swapFileHandled();
}

void SwapFile::fileSaved(const QString&)
{
  // remove old swap file (e.g. if a file A was "saved as" B)
  removeSwapFile();
  
  // set the name for the new swap file
  updateFileName();
}

void SwapFile::startEditing ()
{
  // no swap file, no work
  if (m_swapfile.fileName().isEmpty())
    return;
  
  //  if swap file doesn't exists, open it in WriteOnly mode
  // if it does, append the data to the existing swap file,
  // in case you recover and start edititng again
  if (!m_swapfile.exists()) {
    // TODO set file as read-only
    m_swapfile.open(QIODevice::WriteOnly);
    m_stream.setDevice(&m_swapfile);

    // write file header
    m_stream << QByteArray (swapFileVersionString);
  } else if (m_stream.device() == 0) {
    m_swapfile.open(QIODevice::Append);
    m_stream.setDevice(&m_swapfile);
  }
  
  // format: qint8  
  m_stream << EA_StartEditing;
}

void SwapFile::finishEditing ()
{
  // skip if not open
  if (!m_swapfile.isOpen ())
    return;
  
  // format: qint8
  m_stream << EA_FinishEditing;
  m_swapfile.flush();

#ifndef Q_OS_WIN
  // ensure that the file is written to disk
  fdatasync (m_swapfile.handle());
#endif
}

void SwapFile::wrapLine (const KTextEditor::Cursor &position)
{
  // skip if not open
  if (!m_swapfile.isOpen ())
    return;
  
  // format: qint8, int, int
  m_stream << EA_WrapLine << position.line() << position.column();
}

void SwapFile::unwrapLine (int line)
{
  // skip if not open
  if (!m_swapfile.isOpen ())
    return;
  
  // format: qint8, int
  m_stream << EA_UnwrapLine << line;
}

void SwapFile::insertText (const KTextEditor::Cursor &position, const QString &text)
{
  // skip if not open
  if (!m_swapfile.isOpen ())
    return;
  
  // format: qint8, int, int, bytearray
  m_stream << EA_InsertText << position.line() << position.column() << text.toUtf8 ();
}

void SwapFile::removeText (const KTextEditor::Range &range)
{
  // skip if not open
  if (!m_swapfile.isOpen ())
    return;
  
  // format: qint8, int, int, int
  Q_ASSERT (range.start().line() == range.end().line());
  m_stream << EA_RemoveText
            << range.start().line() << range.start().column()
            << range.end().column();
}

bool SwapFile::shouldRecover() const
{
  return !m_swapfile.fileName().isEmpty() && m_swapfile.exists() && m_stream.device() == 0;
}

void SwapFile::discard()
{
  removeSwapFile();
  emit swapFileHandled();
}

void SwapFile::removeSwapFile()
{
  if (!m_swapfile.fileName().isEmpty() && m_swapfile.exists()) {
    m_stream.setDevice(0);
    m_swapfile.close();
    m_swapfile.remove();
  }
}

bool SwapFile::updateFileName()
{
  // first clear filename
  m_swapfile.setFileName ("");
  
  const KUrl &url = m_document->url();
  if (url.isEmpty() || !url.isLocalFile())
    return false;

  QString path = url.toLocalFile();
  int poz = path.lastIndexOf(QDir::separator());
  path.insert(poz+1, ".swp.");

  m_swapfile.setFileName(path);
  return true;
}

}

// kate: space-indent on; indent-width 2; replace-tabs on;
