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

#include <kjs/function_object.h>
#include <kjs/interpreter.h>
#include <kjs/lookup.h>

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

class KateJSGlobal : public KJS::ObjectImp {
public:
  virtual KJS::UString className() const { return "global"; }
};

class KateJSDocument : public KJS::ObjectImp {
public:
  KateJSDocument (KJS::ExecState *exec, KateDocument *_doc);

  virtual const KJS::ClassInfo* classInfo() const { return &info; }

  static const KJS::ClassInfo info;

  enum { InsertLine,
         RemoveLine,
         Name
  };

public:
  KateDocument *doc;
};

KateJScript::KateJScript ()
 : m_global (new KJS::Object (new KateJSGlobal ()))
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
  // put some stuff into env.
  m_global->put(m_interpreter->globalExec(), "document", KJS::Object(new KateJSDocument(m_interpreter->globalExec(), doc)));

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

//BEGIN KateJSDocument

#include "katejscript.lut.h"

// -------------------------------------------------------------------------
/* Source for KateJSDocumentProtoTable.
@begin KateJSDocumentProtoTable 3
  insertLine KateJSDocument::InsertLine  DontDelete|Function 2
  removeLine KateJSDocument::RemoveLine  DontDelete|Function 1
@end
*/

/* Source for KateJSDocumentTable.
@begin KateJSDocumentTable 2
  nodeName  KateJSDocument::Name DontDelete|ReadOnly
@end
*/

DEFINE_PROTOTYPE("KateJSDocument",KateJSDocumentProto)
IMPLEMENT_PROTOFUNC(KateJSDocumentProtoFunc)
IMPLEMENT_PROTOTYPE(KateJSDocumentProto,KateJSDocumentProtoFunc)

const KJS::ClassInfo KateJSDocument::info = { "KateJSDocument", 0, &KateJSDocumentTable, 0 };

Value KateJSDocumentProtoFunc::call(KJS::ExecState *exec, KJS::Object &thisObj, const KJS::List &args)
{
  KJS_CHECK_THIS( KateJSDocument, thisObj );

  KateDocument *doc = static_cast<KateJSDocument *>( thisObj.imp() )->doc;

  switch (id) {
    case KateJSDocument::InsertLine:
      doc->insertLine (0, "TEST LINE");
      break;
  }

  return KJS::Undefined();
}

KateJSDocument::KateJSDocument (KJS::ExecState *exec, KateDocument *_doc)
    : KJS::ObjectImp (KateJSDocumentProto::self(exec))
    , doc (_doc)
{
}

//END

// kate: space-indent on; indent-width 2; replace-tabs on;
