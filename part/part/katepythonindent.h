/* This file is part of the KDE libraries
   Copyright (C) 2003 Jesse Yurkovich <yurkjes@iit.edu>

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
*/
#ifndef kate_pythonindent_h
#define kate_pythonindent_h

#include <qregexp.h>

#include "kateautoindent.h"
#include "katedocument.h"

class KatePythonIndent : public KateAutoIndent
{
  public:
    KatePythonIndent (KateDocument *doc);
    ~KatePythonIndent ();

    virtual void processNewline (KateDocCursor &begin, bool needContinue);

    virtual uint modeNumber () const { return KateDocumentConfig::imPythonStyle; };

  private:
    int calcExtra (int &prevBlock, int &pos, KateDocCursor &end);

    static QRegExp endWithColon;
    static QRegExp stopStmt;
    static QRegExp blockBegin;
};
#endif

// kate: space-indent on; indent-width 2; replace-tabs on; indent-mode cstyle;
