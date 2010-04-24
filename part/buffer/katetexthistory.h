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

#ifndef KATE_TEXTHISTORY_H
#define KATE_TEXTHISTORY_H

#include <QtCore/QList>

#include "katepartprivate_export.h"

namespace Kate {

class TextBuffer;

/**
 * Class representing the editing history of a TextBuffer
 */
class KATEPART_TESTS_EXPORT TextHistory {
  friend class TextBuffer;

  public:
    /**
     * Current revision, just relay the revision of the buffer
     * @return current revision
     */
    qint64 currentRevision () const;

    /**
     * Last revision the buffer got successful saved
     * @return last revision buffer got saved, -1 if none
     */
    qint64 lastSavedRevision () const { return m_lastSavedRevision; }

    /**
     * Class representing one entry in the editing history.
     */
    class Entry {
      public:
    };

  private:
    /**
     * Construct an empty text history.
     * @param buffer buffer this text history belongs to
     */
    TextHistory (TextBuffer &buffer);

    /**
     * Destruct the text history
     */
    ~TextHistory ();

    /**
     * Clear the edit history, this is done on clear() in buffer.
     */
    void clear ();

    /**
     * Set current revision as last saved revision
     */
    void setLastSavedRevision ();

  private:
    /**
     * TextBuffer this history belongs to
     */
    TextBuffer &m_buffer;

    /**
     * Last revision the buffer got saved
     */
    qint64 m_lastSavedRevision;

    /**
     * history of edits
     */
    QList<Entry> m_historyEntries;

    /**
     * offset for the first entry in m_history, to which revision it really belongs?
     */
    qint64 m_firstHistoryEntryRevision;
};

}

#endif
