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

#include "katepartprivate_export.h"

namespace Kate {

class TextBuffer;

/**
 * Class representing the editing history of a TextBuffer
 */
class KATEPART_TESTS_EXPORT TextHistory {
  public:
    /**
     * Construct an empty text history.
     * @param buffer buffer this text history belongs to
     */
    TextHistory (TextBuffer &buffer);

    /**
     * Destruct the text history
     */
    ~TextHistory ();

  private:
    /**
     * TextBuffer this history belongs to
     */
    TextBuffer &m_buffer;
};

}

#endif
