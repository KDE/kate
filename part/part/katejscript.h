/* This file is part of the KDE libraries
   Copyright (C) 2005 Christoph Cullmann <cullmann@kde.org>

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

#ifndef __kate_jscript_h__
#define __kate_jscript_h__

class KateDocument;
class KateView;
class QString;

namespace KJS {
  class Object;
  class Interpreter;
}

class KateJScript
{
  public:
    KateJScript ();
    ~KateJScript ();

    bool execute (KateDocument *doc, KateView *view, const QString &script);

  private:
    /**
     * global object of interpreter
     */
    KJS::Object *m_global;

    /**
     * js interpreter
     */
    KJS::Interpreter *m_interpreter;
};

#endif
