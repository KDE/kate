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

#include "katejscript.h"

#include "katedocument.h"
#include "kateview.h"

#include <kdebug.h>

#include <kjs/interpreter.h>

namespace KJS {

// taken from khtml, just to play
UString::UString(const QString &d)
{
  unsigned int len = d.length();
  UChar *dat = new UChar[len];
  memcpy(dat, d.unicode(), len * sizeof(UChar));
  rep = UString::Rep::create(dat, len);
}

}

class GlobalImp : public KJS::ObjectImp {
public:
  virtual KJS::UString className() const { return "global"; }
};

KateJScript::KateJScript ()
 : m_global (new KJS::Object (new GlobalImp ()))
 , m_interpreter (new KJS::Interpreter (*m_global))
{
}

KateJScript::~KateJScript ()
{
  delete m_interpreter;
  delete m_global;
}

bool KateJScript::execute (KateDocument *doc, KateView *view, const QString &script)
{
  // run
  KJS::Completion comp (m_interpreter->evaluate(script));

  if (comp.complType() == KJS::Throw)
  {
    KJS::ExecState *exec = m_interpreter->globalExec();

    KJS::Value exVal = comp.value();

    char *msg = exVal.toString(exec).ascii();

    int lineno = -1;

    if (exVal.type() == KJS::ObjectType)
    {
      KJS::Value lineVal = KJS::Object::dynamicCast(exVal).get(exec,"line");

      if (lineVal.type() == KJS::NumberType)
        lineno = int(lineVal.toNumber(exec));
    }

    kdDebug () << "Exception, line " << lineno << ": " << msg << endl;

    return false;
  }

  kdDebug () << "script executed" << endl;
  return true;
}

// kate: space-indent on; indent-width 2; replace-tabs on;
