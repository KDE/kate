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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <kdebug.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kconfig.h>

#include <kjs/function_object.h>
#include <kjs/interpreter.h>
#include <kjs/lookup.h>

#include <qfile.h>
#include <qfileinfo.h>
#include <qregexp.h>

namespace KJS {

// taken from khtml
// therefor thx to:
// Copyright (C) 1999-2003 Harri Porten (porten@kde.org)
// Copyright (C) 2001-2003 David Faure (faure@kde.org)
// Copyright (C) 2003 Apple Computer, Inc.

UString::UString(const QString &d)
{
  unsigned int len = d.length();
  UChar *dat = new UChar[len];
  memcpy(dat, d.unicode(), len * sizeof(UChar));
  rep = UString::Rep::create(dat, len);
}

QString UString::qstring() const
{
  return QString((QChar*) data(), size());
}

QConstString UString::qconststring() const
{
  return QConstString((QChar*) data(), size());
}

}

//BEGIN JS API STUFF

class KateJSGlobal : public KJS::ObjectImp {
public:
  virtual KJS::UString className() const { return "global"; }
};

class KateJSDocument : public KJS::ObjectImp
{
  public:
    KateJSDocument (KJS::ExecState *exec, KateDocument *_doc);

    const KJS::ClassInfo* classInfo() const { return &info; }

    enum { FullText,
          Text,
          TextLine,
          NumLines,
          Length,
          LineLength,
          SetText,
          Clear,
          InsertText,
          RemoveText,
          InsertLine,
          RemoveLine,
          EditBegin,
          EditEnd
    };

  public:
    KateDocument *doc;

    static const KJS::ClassInfo info;
};

class KateJSView : public KJS::ObjectImp
{
  public:
    KateJSView (KJS::ExecState *exec, KateView *_view);

    KJS::Value get( KJS::ExecState *exec, const  KJS::Identifier &propertyName) const;

    KJS::Value getValueProperty(KJS::ExecState *exec, int token) const;

    void put(KJS::ExecState *exec, const KJS::Identifier &propertyName, const KJS::Value& value, int attr = KJS::None);

    void putValueProperty(KJS::ExecState *exec, int token, const KJS::Value& value, int attr);

    const KJS::ClassInfo* classInfo() const { return &info; }

    enum { CursorLine,
          CursorColumn,
          CursorColumnReal,
          Selection,
          HasSelection,
          SetSelection,
          RemoveSelection,
          SelectAll,
          ClearSelection,
          SelStartLine,
          SelStartCol,
          SelEndLine,
          SelEndCol
    };

  public:
    KateView *view;

    static const KJS::ClassInfo info;
};

#include "katejscript.lut.h"

//END

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

KJS::ObjectImp *KateJScript::wrapDocument (KJS::ExecState *exec, KateDocument *doc)
{
  return new KateJSDocument(exec, doc);
}

KJS::ObjectImp *KateJScript::wrapView (KJS::ExecState *exec, KateView *view)
{
  return new KateJSView(exec, view);
}

bool KateJScript::execute (KateView *view, const QString &script)
{
  if (!view)
    return false;

  // put some stuff into env.
  m_interpreter->globalObject().put(m_interpreter->globalExec(), "document", KJS::Object(wrapDocument(m_interpreter->globalExec(), view->doc())));
  m_interpreter->globalObject().put(m_interpreter->globalExec(), "view", KJS::Object(wrapView(m_interpreter->globalExec(), view)));

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

// -------------------------------------------------------------------------
/* Source for KateJSDocumentProtoTable.
@begin KateJSDocumentProtoTable 15
#
# edit interface stuff + editBegin/End, this is nice start
#
  textFull       KateJSDocument::FullText      DontDelete|Function 0
  textPart       KateJSDocument::Text          DontDelete|Function 4
  textLine       KateJSDocument::TextLine      DontDelete|Function 1
  numLines       KateJSDocument::NumLines      DontDelete|Function 0
  length         KateJSDocument::Length        DontDelete|Function 0
  lineLength     KateJSDocument::LineLength    DontDelete|Function 1
  setText        KateJSDocument::SetText       DontDelete|Function 1
  clear          KateJSDocument::Clear         DontDelete|Function 0
  insertText     KateJSDocument::InsertText    DontDelete|Function 3
  removeText     KateJSDocument::RemoveText    DontDelete|Function 4
  insertLine     KateJSDocument::InsertLine    DontDelete|Function 2
  removeLine     KateJSDocument::RemoveLine    DontDelete|Function 1
  editBegin      KateJSDocument::EditBegin     DontDelete|Function 0
  editEnd        KateJSDocument::EditEnd       DontDelete|Function 0
@end
*/

DEFINE_PROTOTYPE("KateJSDocument",KateJSDocumentProto)
IMPLEMENT_PROTOFUNC(KateJSDocumentProtoFunc)
IMPLEMENT_PROTOTYPE(KateJSDocumentProto,KateJSDocumentProtoFunc)

const KJS::ClassInfo KateJSDocument::info = { "KateJSDocument", 0, 0, 0 };

Value KateJSDocumentProtoFunc::call(KJS::ExecState *exec, KJS::Object &thisObj, const KJS::List &args)
{
  KJS_CHECK_THIS( KateJSDocument, thisObj );

  KateDocument *doc = static_cast<KateJSDocument *>( thisObj.imp() )->doc;

  if (!doc)
    return KJS::Undefined();

  switch (id)
  {
    case KateJSDocument::FullText:
      return KJS::String (doc->text());

    case KateJSDocument::Text:
      return KJS::String (doc->text(args[0].toUInt32(exec), args[1].toUInt32(exec), args[2].toUInt32(exec), args[3].toUInt32(exec)));

    case KateJSDocument::TextLine:
      return KJS::String (doc->textLine (args[0].toUInt32(exec)));

    case KateJSDocument::NumLines:
      return KJS::Number (doc->numLines());

    case KateJSDocument::Length:
      return KJS::Number (doc->length());

    case KateJSDocument::LineLength:
      return KJS::Number (doc->lineLength(args[0].toUInt32(exec)));

    case KateJSDocument::SetText:
      return KJS::Boolean (doc->setText(args[0].toString(exec).qstring()));

    case KateJSDocument::Clear:
      return KJS::Boolean (doc->clear());

    case KateJSDocument::InsertText:
      return KJS::Boolean (doc->insertText (args[0].toUInt32(exec), args[1].toUInt32(exec), args[2].toString(exec).qstring()));

    case KateJSDocument::RemoveText:
      return KJS::Boolean (doc->removeText(args[0].toUInt32(exec), args[1].toUInt32(exec), args[2].toUInt32(exec), args[3].toUInt32(exec)));

    case KateJSDocument::InsertLine:
      return KJS::Boolean (doc->insertLine (args[0].toUInt32(exec), args[1].toString(exec).qstring()));

    case KateJSDocument::RemoveLine:
      return KJS::Boolean (doc->removeLine (args[0].toUInt32(exec)));

    case KateJSDocument::EditBegin:
      doc->editBegin();
      return KJS::Null ();

    case KateJSDocument::EditEnd:
      doc->editEnd ();
      return KJS::Null ();
  }

  return KJS::Undefined();
}

KateJSDocument::KateJSDocument (KJS::ExecState *exec, KateDocument *_doc)
    : KJS::ObjectImp (KateJSDocumentProto::self(exec))
    , doc (_doc)
{
}

//END

//BEGIN KateJSView

// -------------------------------------------------------------------------
/* Source for KateJSViewProtoTable.
@begin KateJSViewProtoTable 14
  cursorLine          KateJSView::CursorLine            DontDelete|Function 0
  cursorColumn        KateJSView::CursorColumn          DontDelete|Function 0
  cursorColumnReal    KateJSView::CursorColumnReal      DontDelete|Function 0
  selection           KateJSView::Selection         DontDelete|Function 0
  hasSelection        KateJSView::HasSelection      DontDelete|Function 0
  setSelection        KateJSView::SetSelection      DontDelete|Function 4
  removeSelection     KateJSView::RemoveSelection     DontDelete|Function 0
  selectAll           KateJSView::SelectAll           DontDelete|Function 0
  clearSelection      KateJSView::ClearSelection      DontDelete|Function 0
@end
*/

/* Source for KateJSViewTable.
@begin KateJSViewTable 5
  selStartLine        KateJSView::SelStartLine        DontDelete|ReadOnly
  selStartCol         KateJSView::SelStartCol         DontDelete|ReadOnly
  selEndLine          KateJSView::SelEndLine          DontDelete|ReadOnly
  selEndCol           KateJSView::SelEndCol           DontDelete|ReadOnly
@end
*/

DEFINE_PROTOTYPE("KateJSView",KateJSViewProto)
IMPLEMENT_PROTOFUNC(KateJSViewProtoFunc)
IMPLEMENT_PROTOTYPE(KateJSViewProto,KateJSViewProtoFunc)

const KJS::ClassInfo KateJSView::info = { "KateJSView", 0, &KateJSViewTable, 0 };

Value KateJSViewProtoFunc::call(KJS::ExecState *exec, KJS::Object &thisObj, const KJS::List &args)
{
  KJS_CHECK_THIS( KateJSView, thisObj );

  KateView *view = static_cast<KateJSView *>( thisObj.imp() )->view;

  if (!view)
    return KJS::Undefined();

  switch (id)
  {
    case KateJSView::CursorLine:
      return KJS::Number (view->cursorLine());

    case KateJSView::CursorColumn:
      return KJS::Number (view->cursorColumn());

    case KateJSView::CursorColumnReal:
      return KJS::Number (view->cursorColumnReal());

    // SelectionInterface goes in the view, in anticipation of the future
    case KateJSView::Selection:
      return KJS::String( view->getDoc()->selection() );

    case KateJSView::HasSelection:
      return KJS::Boolean( view->getDoc()->hasSelection() );

    case KateJSView::SetSelection:
      return KJS::Boolean( view->getDoc()->setSelection(args[0].toUInt32(exec),
                                                     args[1].toUInt32(exec),
                                                     args[2].toUInt32(exec),
                                                     args[3].toUInt32(exec)) );

    case KateJSView::RemoveSelection:
      return KJS::Boolean( view->getDoc()->removeSelectedText() );

    case KateJSView::SelectAll:
      return KJS::Boolean( view->getDoc()->selectAll() );

    case KateJSView::ClearSelection:
      return KJS::Boolean( view->getDoc()->clearSelection() );
  }

  return KJS::Undefined();
}

KateJSView::KateJSView (KJS::ExecState *exec, KateView *_view)
    : KJS::ObjectImp (KateJSViewProto::self(exec))
    , view (_view)
{
}

KJS::Value KateJSView::get( KJS::ExecState *exec, const  KJS::Identifier &propertyName) const
{
  return KJS::lookupGetValue<KateJSView,KJS::ObjectImp>(exec, propertyName, &KateJSViewTable, this );
}

KJS::Value KateJSView::getValueProperty(KJS::ExecState *exec, int token) const
{
  if (!view)
    return KJS::Undefined ();

  switch (token) {
    case KateJSView::SelStartLine:
      return KJS::Number( view->getDoc()->selStartLine() );

    case KateJSView::SelStartCol:
      return KJS::Number( view->getDoc()->selStartCol() );

    case KateJSView::SelEndLine:
      return KJS::Number( view->getDoc()->selEndLine() );

    case KateJSView::SelEndCol:
      return KJS::Number( view->getDoc()->selEndCol() );
    }

  return KJS::Undefined ();
}

void KateJSView::put(KJS::ExecState *exec, const KJS::Identifier &propertyName, const KJS::Value& value, int attr)
{
   KJS::lookupPut<KateJSView,KJS::ObjectImp>(exec, propertyName, value, attr, &KateJSViewTable, this );
}

void KateJSView::putValueProperty(KJS::ExecState *exec, int token, const KJS::Value& value, int attr)
{
  if (!view)
    return;


}

//END

//BEGIN KateJScriptManager

KateJScriptManager::KateJScriptManager ()
{
  m_scripts.setAutoDelete (true);
  collectScripts ();
}

KateJScriptManager::~KateJScriptManager ()
{
}

void KateJScriptManager::collectScripts (bool force)
{
// If there's something in myModeList the Mode List was already built so, don't do it again
  if (!m_scripts.isEmpty())
    return;

  // We'll store the scripts list in this config
  KConfig config("katepartjscriptrc", false, false);

  // figure our if the kate install is too new
  config.setGroup ("General");
  if (config.readNumEntry ("Version") > config.readNumEntry ("CachedVersion"))
  {
    config.writeEntry ("CachedVersion", config.readNumEntry ("Version"));
    force = true;
  }

  // Let's get a list of all the xml files for hl
  QStringList list = KGlobal::dirs()->findAllResources("data","katepart/scripts/*.js",false,true);

  // Let's iterate through the list and build the Mode List
  for ( QStringList::Iterator it = list.begin(); it != list.end(); ++it )
  {
    // Each file has a group called:
    QString Group="Cache "+ *it;

    // Let's go to this group
    config.setGroup(Group);

    // stat the file
    struct stat sbuf;
    memset (&sbuf, 0, sizeof(sbuf));
    stat(QFile::encodeName(*it), &sbuf);

    // If the group exist and we're not forced to read the xml file, let's build myModeList for katesyntax..rc
    if (!force && config.hasGroup(Group) && (sbuf.st_mtime == config.readNumEntry("lastModified")))
    {
    }
    else
    {
      kdDebug (13050) << "add script: " << *it << endl;

      QString desktopFile =  (*it).left((*it).length()-2).append ("desktop");

      kdDebug (13050) << "add script (desktop file): " << desktopFile << endl;

      QFileInfo dfi (desktopFile);

      if (dfi.exists())
      {

      }
      else // no desktop file around, fall back to scriptfilename == commandname
      {
        kdDebug (13050) << "add script: fallback, no desktop file around!" << endl;

        QFileInfo fi (*it);

        if (m_scripts[fi.baseName()])
          continue;

        KateJScriptManager::Script *s = new KateJScriptManager::Script ();

        s->name = fi.baseName();
        s->filename = *it;

        m_scripts.insert (s->name, s);
      }
    }
  }

  // Syncronize with the file katesyntax...rc
  config.sync();
}

bool KateJScriptManager::exec( class Kate::View *view, const QString &_cmd, QString &errorMsg )
{
   //create a list of args
  QStringList args( QStringList::split( QRegExp("\\s+"), _cmd ) );
  QString cmd ( args.first() );
  args.remove( args.first() );

  kdDebug(13050) << cmd << endl;

  return false;
}

bool KateJScriptManager::help( class Kate::View *, const QString &, QString & )
{
  return false;
}

QStringList KateJScriptManager::cmds()
{
   QStringList l;

   QDictIterator<KateJScriptManager::Script> it( m_scripts );
   for( ; it.current(); ++it )
     l << it.current()->name;

   return l;
}

//END

// kate: space-indent on; indent-width 2; replace-tabs on;
