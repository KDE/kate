/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2010 Christoph Cullmann <cullmann@kde.org>
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

#ifndef KATE_TEXTLOADER_H
#define KATE_TEXTLOADER_H

#include <QtCore/QString>
#include <QtCore/QFile>

// on the fly compression
#include <kfilterdev.h>
#include <kmimetype.h>

namespace Kate {

/**
 * loader block size, load 256 kb at once per default
 * if file size is smaller, fall back to file size
 * must be a multiple of 2
 */
static const qint64 KATE_FILE_LOADER_BS  = 256 * 1024;

/**
 * File Loader, will handle reading of files + detecting encoding
 */
class TextLoader
{
  public:
    /**
     * Construct file loader for given file.
     * @param filename file to open
     * @param proberType prober type
     */
    TextLoader (const QString &filename, KEncodingProber::ProberType proberType)
      : m_codec (0)
      , m_eof (false) // default to not eof
      , m_lastWasEndOfLine (true) // at start of file, we had a virtual newline
      , m_lastWasR (false) // we have not found a \r as last char
      , m_position (0)
      , m_lastLineStart (0)
      , m_eol (TextBuffer::eolUnknown) // no eol type detected atm
      , m_buffer (KATE_FILE_LOADER_BS, 0)
      , m_converterState (0)
      , m_bomFound (false)
      , m_firstRead (true)
      , m_proberType (proberType)
    {
      // try to get mimetype for on the fly decompression, don't rely on filename!
      QFile testMime (filename);
      if (testMime.open (QIODevice::ReadOnly))
        m_mimeType = KMimeType::findByContent (&testMime)->name ();
      else
        m_mimeType = KMimeType::findByPath (filename, 0, false)->name ();

      // construct filter device
      m_file = KFilterDev::deviceForFile (filename, m_mimeType, false);
    }

    /**
     * Destructor
     */
    ~TextLoader ()
    {
      delete m_file;
      delete m_converterState;
    }

    /**
     * open file with given codec
     * @param codec codec to use, if 0, will do some auto-dectect or fallback
     * @return success
     */
    bool open (QTextCodec *codec)
    {
      m_codec = codec;
      m_eof = false;
      m_lastWasEndOfLine = true;
      m_lastWasR = false;
      m_position = 0;
      m_lastLineStart = 0;
      m_eol = TextBuffer::eolUnknown;
      m_text.clear ();
      delete m_converterState;
      m_converterState = new QTextCodec::ConverterState (QTextCodec::ConvertInvalidToNull);
      m_bomFound = false;
      m_firstRead = true;

      // if already opened, close the file...
      if (m_file->isOpen())
        m_file->close ();

      return m_file->open (QIODevice::ReadOnly);
    }

    /**
     * end of file reached?
     * @return end of file reached
     */
    bool eof () const { return m_eof && !m_lastWasEndOfLine && (m_lastLineStart == m_text.length()); }

    /**
     * Detected end of line mode for this file.
     * Detected during reading, is valid after complete file is read.
     * @return eol mode of this file
     */
    TextBuffer::EndOfLineMode eol () const { return m_eol; }

    /**
     * BOM found?
     * @return byte order mark found?
     */
    bool byteOrderMarkFound () const { return m_bomFound; }

    /**
     * mime type used to create filter dev
     * @return mime-type of filter device
     */
    const QString &mimeTypeForFilterDev () const { return m_mimeType; }

    /**
     * internal unicode data array
     * @return internal unicode data
     */
    const QChar *unicode () const { return m_text.unicode(); }

    /**
     * Get codec for this loader
     * @return currently in use codec of this loader
     */
    QTextCodec *textCodec () const { return m_codec; }

    /**
     * read a line, return length + offset in unicode data
     * @param offset offset into internal unicode data for read line
     * @param length lenght of read line
     * @return true if no encoding errors occured
     */
    bool readLine (int &offset, int &length)
    {
      length = 0;
      offset = 0;
      bool encodingError = false;

      static const QLatin1Char cr(QLatin1Char('\r'));
      static const QLatin1Char lf(QLatin1Char('\n'));

      while (m_position <= m_text.length())
      {
        if (m_position == m_text.length())
        {
          // try to load more text if something is around
          if (!m_eof)
          {
            int c = m_file->read (m_buffer.data(), m_buffer.size());

            // kill the old lines...
            m_text.remove (0, m_lastLineStart);

            // if any text is there, append it....
            if (c > 0)
            {
              // detect byte order marks & codec for byte order markers on first read
              int bomBytes = 0;
              if (m_firstRead) {
                // use first 16 bytes max to allow BOM detection of codec
                QByteArray bom (m_buffer.data(), qMin (16, c));
                QTextCodec *codecForByteOrderMark = QTextCodec::codecForUtfText (bom, 0);

                // if codec != null, we found a BOM!
                if (codecForByteOrderMark) {
                  m_bomFound = true;

                  // eat away the different boms!
                  int mib = codecForByteOrderMark->mibEnum ();
                  if (mib == 106) // utf8
                    bomBytes = 3;
                  if (mib == 1013 || mib == 1014) // utf16
                    bomBytes = 2;
                  if (mib == 1018 || mib == 1019) // utf32
                    bomBytes = 4;
                }

                /**
                 * if no codec given, do autodetection
                 */
                if (!m_codec) {
                  /**
                   * byte order said something about encoding?
                   */
                  if (codecForByteOrderMark)
                    m_codec = codecForByteOrderMark;
                  else {
                    /**
                     * no unicode BOM found, trigger prober
                     */
                    KEncodingProber prober (m_proberType);
                    prober.feed (m_buffer.constData(), c);

                    // we found codec with some confidence?
                    if (prober.confidence() > 0.5)
                      m_codec = QTextCodec::codecForName(prober.encoding());

                    // no codec, no chance, encoding error
                    if (!m_codec)
                      return false;
                  }
                }

                m_firstRead = false;
              }

              Q_ASSERT (m_codec);
              QString unicode = m_codec->toUnicode (m_buffer.constData() + bomBytes, c - bomBytes, m_converterState);

              // detect broken encoding
              for (int i = 0; i < unicode.size(); ++i) {
                  if (unicode[i] == 0) {
                    encodingError = true;
                    break;
                  }
              }

              m_text.append (unicode);
            }

            // is file completely read ?
            m_eof = (c == -1) || (c == 0);

            // recalc current pos and last pos
            m_position -= m_lastLineStart;
            m_lastLineStart = 0;
          }

          // oh oh, end of file, escape !
          if (m_eof && (m_position == m_text.length()))
          {
            m_lastWasEndOfLine = false;

            // line data
            offset = m_lastLineStart;
            length = m_position-m_lastLineStart;

            m_lastLineStart = m_position;

            return !encodingError;
          }
        }

        if (m_text.at(m_position) == lf)
        {
          m_lastWasEndOfLine = true;

          if (m_lastWasR)
          {
            m_lastLineStart++;
            m_lastWasR = false;
            m_eol = TextBuffer::eolDos;
          }
          else
          {
            // line data
            offset = m_lastLineStart;
            length = m_position-m_lastLineStart;

            m_lastLineStart = m_position+1;
            m_position++;

            // only win, if not dos!
            if (m_eol != TextBuffer::eolDos)
              m_eol = TextBuffer::eolUnix;

            return !encodingError;
          }
        }
        else if (m_text.at(m_position) == cr)
        {
          m_lastWasEndOfLine = true;
          m_lastWasR = true;

          // line data
          offset = m_lastLineStart;
          length = m_position-m_lastLineStart;

          m_lastLineStart = m_position+1;
          m_position++;

          // should only win of first time!
          if (m_eol == TextBuffer::eolUnknown)
            m_eol = TextBuffer::eolMac;

          return !encodingError;
        }
        else
        {
          m_lastWasEndOfLine = false;
          m_lastWasR = false;
        }

        m_position++;
      }

      return !encodingError;
    }

  private:
    QTextCodec *m_codec;
    bool m_eof;
    bool m_lastWasEndOfLine;
    bool m_lastWasR;
    int m_position;
    int m_lastLineStart;
    TextBuffer::EndOfLineMode m_eol;
    QString m_mimeType;
    QIODevice *m_file;
    QByteArray m_buffer;
    QString m_text;
    QTextCodec::ConverterState *m_converterState;
    bool m_bomFound;
    bool m_firstRead;
    KEncodingProber::ProberType m_proberType;
};

}

#endif
