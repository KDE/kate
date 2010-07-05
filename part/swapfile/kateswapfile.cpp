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

#include <QFileInfo>
#include <QDir>

namespace Kate {

const static qint8 EA_StartEditing  = 0x02; // 0000 0010
const static qint8 EA_FinishEditing = 0x03; // 0000 0011
const static qint8 EA_WrapLine      = 0x04; // 0000 0100
const static qint8 EA_UnwrapLine    = 0x05; // 0000 0101
const static qint8 EA_InsertText    = 0x08; // 0000 1000
const static qint8 EA_RemoveText    = 0x09; // 0000 1001



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
  m_stream.setDevice(0);
  if (m_swapfile.exists())
    m_swapfile.remove();
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

void SwapFile::fileLoaded(const QString&)
{
  // TODO FIXME: remove old swap file if there exists one
  
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
  
  // TODO set file as read-only
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
  QString header;
  m_stream >> header;
  if (header != QString ("Kate Swap File Version 1.0"))
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
        int line, column;
        m_stream >> line >> column;
        m_document->editWrapLine(line, column);
        break;
      }
      case EA_UnwrapLine: {
        int line;
        m_stream >> line;
        m_document->editUnWrapLine(line);
        break;
      }
      case EA_InsertText: {
        int line, column;
        QString text;
        m_stream >> line >> column >> text;
        m_document->insertText(KTextEditor::Cursor(line, column), text);
        break;
      }
      case EA_RemoveText: {
        int startLine, startColumn, endLine, endColumn;
        m_stream >> startLine >> startColumn >> endLine >> endColumn;
        m_document->removeText(KTextEditor::Range(KTextEditor::Cursor(startLine, startColumn),
                                              KTextEditor::Cursor(endLine, endColumn)));
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
  if (!updateFileName())
    return;
}

void SwapFile::startEditing ()
{
  //  if swap file doesn't exists, open it in WriteOnly mode
  // if it does, append the data to the existing swap file,
  // in case you recover and start edititng again
  if (!m_swapfile.exists()) { 
    m_swapfile.open(QIODevice::WriteOnly);
    m_stream.setDevice(&m_swapfile);
    
    // write file header
    m_stream << QString ("Kate Swap File Version 1.0");
  } else if (m_stream.device() == 0) {
    m_swapfile.open(QIODevice::Append);
    m_stream.setDevice(&m_swapfile);
  }
  
  // format: qint8  
  m_stream << EA_StartEditing;
}

void SwapFile::finishEditing ()
{
  // format: qint8
  m_stream << EA_FinishEditing;
  m_swapfile.flush();
}

void SwapFile::wrapLine (const KTextEditor::Cursor &position)
{
  // format: qint8, int, int
  m_stream << EA_WrapLine << position.line() << position.column();
}

void SwapFile::unwrapLine (int line)
{
  // format: qint8, int
  m_stream << EA_UnwrapLine << line;
}

void SwapFile::insertText (const KTextEditor::Cursor &position, const QString &text)
{
  // format: qint8, int, int, string
  m_stream << EA_InsertText << position.line() << position.column() << text;
}

void SwapFile::removeText (const KTextEditor::Range &range)
{
  // format: qint8, int, int, int, int
  m_stream << EA_RemoveText
            << range.start().line() << range.start().column()
            << range.end().line() << range.end().column();
}

bool SwapFile::shouldRecover() const
{
  return m_swapfile.exists() && m_stream.device() == 0;
}

void SwapFile::discard()
{
  removeSwapFile();
  emit swapFileHandled();
}

void SwapFile::removeSwapFile()
{
  if (m_swapfile.exists()) {
    m_stream.setDevice(0);
    m_swapfile.close();
    m_swapfile.remove();
  }
}

bool SwapFile::updateFileName()
{
  KUrl url = m_document->url();
  if (!url.isLocalFile())
    return false;

  QString path = url.toLocalFile();
  int poz = path.lastIndexOf(QDir::separator());
  path.insert(poz+1, ".swp.");

  m_swapfile.setFileName(path);

  return true;
}

}

// kate: space-indent on; indent-width 2; replace-tabs on;
