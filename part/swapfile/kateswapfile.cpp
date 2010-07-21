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
#include <QApplication>

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

QTimer* SwapFile::s_timer = 0;

SwapFile::SwapFile(KateDocument *document)
  : QObject(document)
  , m_document(document)
  , m_trackingEnabled(false)
  , m_recovered(false)
  , m_modified(false)
{
  // fixed version of serialisation
  m_stream.setVersion (QDataStream::Qt_4_6);

  // conect the timer
  connect(syncTimer(), SIGNAL(timeout()), this, SLOT(writeFileToDisk()), Qt::DirectConnection);
  
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
  // replay the swap file
  if (!m_swapfile.open(QIODevice::ReadOnly))
  {
    kWarning( 13020 ) << "Can't open swap file";
    return;
  }

  // remember that the file has recovered
  m_recovered = true;
  
  // open data stream
  m_stream.setDevice(&m_swapfile);
  
  recover(m_stream);
  
  // close swap file
  m_stream.setDevice(0);
  m_swapfile.close();

  // emit signal in case the document has more views
  emit swapFileHandled();
}

bool SwapFile::recover(QDataStream& stream)
{  
  // read and check header
  QByteArray header;
  stream >> header;
  if (header != swapFileVersionString)
  {
    stream.setDevice (0);
    m_swapfile.close ();
    kWarning( 13020 ) << "Can't open swap file, wrong version";
    return false;
  }

  // disconnect current signals
  setTrackingEnabled(false);

  // replay swapfile
  bool editStarted = false;
  while (!stream.atEnd()) {
    qint8 type;
    stream >> type;
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
        stream >> line >> column;
        
        // emulate buffer unwrapLine with document
        m_document->editWrapLine(line, column, true);
        break;
      }
      case EA_UnwrapLine: {
        int line = 0;
        stream >> line;
        
        // assert valid line
        Q_ASSERT (line > 0);
        
        // emulate buffer unwrapLine with document
        m_document->editUnWrapLine(line - 1, true, 0);
        break;
      }
      case EA_InsertText: {
        int line, column;
        QByteArray text;
        stream >> line >> column >> text;
        m_document->insertText(KTextEditor::Cursor(line, column), QString::fromUtf8 (text.data (), text.size()));
        break;
      }
      case EA_RemoveText: {
        int line, startColumn, endColumn;
        stream >> line >> startColumn >> endColumn;
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
  
  // reconnect the signals
  setTrackingEnabled(true);
  
  return true;
}

void SwapFile::fileSaved(const QString&)
{
  m_modified = false;
  
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

  // write the file to the disk every 15 seconds
  if (!syncTimer()->isActive())
    syncTimer()->start(15000);
  
  // format: qint8
  m_stream << EA_FinishEditing;
  m_swapfile.flush();
}

void SwapFile::wrapLine (const KTextEditor::Cursor &position)
{
  // skip if not open
  if (!m_swapfile.isOpen ())
    return;
  
  // format: qint8, int, int
  m_stream << EA_WrapLine << position.line() << position.column();

  m_modified = true;
}

void SwapFile::unwrapLine (int line)
{
  // skip if not open
  if (!m_swapfile.isOpen ())
    return;
  
  // format: qint8, int
  m_stream << EA_UnwrapLine << line;

  m_modified = true;
}

void SwapFile::insertText (const KTextEditor::Cursor &position, const QString &text)
{
  // skip if not open
  if (!m_swapfile.isOpen ())
    return;
  
  // format: qint8, int, int, bytearray
  m_stream << EA_InsertText << position.line() << position.column() << text.toUtf8 ();

  m_modified = true;
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

  m_modified = true;
}

bool SwapFile::shouldRecover() const
{
  // should not recover if the file has already recovered in another view
  if (m_recovered)
    return false;

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

  // get the new path
  QString path = fileName();
  if (path.isNull())
    return false;

  m_swapfile.setFileName(path);
  return true;
}

QString SwapFile::fileName()
{
  const KUrl &url = m_document->url();
  if (url.isEmpty() || !url.isLocalFile())
    return QString();

  QString path = url.toLocalFile();
  int poz = path.lastIndexOf(QDir::separator());
  path.insert(poz+1, ".swp.");

  return path;
}

QTimer* SwapFile::syncTimer()
{
  if (s_timer == 0) {
    s_timer = new QTimer(QApplication::instance());
    s_timer->setSingleShot(true);
  }

  return s_timer;
}

void SwapFile::writeFileToDisk()
{
  if (m_modified) {
    m_modified = false;

    #ifndef Q_OS_WIN
    // ensure that the file is written to disk
    fdatasync (m_swapfile.handle());
    #endif
  }
}

}

// kate: space-indent on; indent-width 2; replace-tabs on;
