/* This file is part of the KDE libraries
   Copyright (C) 2005 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2005 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2006 Dominik Haumann <dhaumann@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "katejscript.h"

#include "katedocument.h"
#include "kateview.h"
#include "kateglobal.h"
#include "kateconfig.h"
#include "kateautoindent.h"
#include "katehighlight.h"
#include "katetextline.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <kdebug.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kconfig.h>

#include <kjs/collector.h>
#include <kjs/function_object.h>
#include <kjs/interpreter.h>
#include <kjs/lookup.h>
#include <kjs/context.h>
#include <kjs/nodes.h>
#include <kjs/function.h>
#include <kjs/scriptfunction.h> // ### supposed to be private API
#include <kjs/PropertyNameArray.h>

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRegExp>
#include <QtCore/QTextStream>


namespace KJS {

#ifdef __GNUC__
#warning "REMOVE ME once KJS headers get fixed"
#endif

#define KJS_CHECK_THIS( ClassName, theObj ) \
  if (!theObj || !theObj->inherits(&ClassName::info)) { \
    KJS::UString errMsg = "Attempt at calling a function that expects a "; \
    errMsg += ClassName::info.className; \
    errMsg += " on a "; \
    errMsg += theObj->className(); \
    KJS::JSObject* err = KJS::Error::create(exec, KJS::TypeError, errMsg.ascii()); \
    exec->setException(err); \
    return err; \
  }

// taken from khtml
// therefor thx to:
// Copyright (C) 1999-2003 Harri Porten (porten@kde.org)
// Copyright (C) 2001-2003 David Faure (faure@kde.org)
// Copyright (C) 2003 Apple Computer, Inc.

UString::UString(const QString &d)
{
  unsigned int len = d.length();
  UChar *dat = static_cast<UChar*>(fastMalloc(sizeof(UChar)*len));
  memcpy(dat, d.unicode(), len * sizeof(UChar));
  m_rep = UString::Rep::create(dat, len);
}

QString UString::qstring() const
{
  return QString((QChar*) data(), size());
}

QString Identifier::qstring() const
{
  return QString((QChar*) data(), size());
}

}

//BEGIN js exception handling
QString mapIdToName(const KJS::HashTable& t, int id)
{
  for (int i = 0; i < t.size; ++i)
    if (id == t.entries[i].value)
      return QString(t.entries[i].s);
  return i18n("unknown");
}

KateJSExceptionTranslator::KateJSExceptionTranslator(KJS::ExecState *exec,
                                                     const KJS::HashTable& hashTable,
                                                     int id,
                                                     const KJS::List& args)
  : m_exec(exec), m_hashTable(hashTable), m_id(id), m_args(args.size()), m_trigger(false)
{}

bool KateJSExceptionTranslator::invalidArgs(int min, int max)
{
  m_trigger = m_args < min || (max != -1 && m_args > max);
  return m_trigger;
}

KateJSExceptionTranslator::~KateJSExceptionTranslator()
{
  if (!m_trigger || m_exec->hadException())
    return;
  const int line = m_exec->context()->currentBody()->lineNo();
  const QString func = (m_exec->context()->function() == 0) ? i18n("unknown") : m_exec->context()->function()->functionName().qstring();
  QString error = i18n("Context '%1': Incorrect number of arguments in '%2'", func, mapIdToName(m_hashTable, m_id));

  throwError(m_exec, KJS::GeneralError, error, line, m_exec->context()->currentBody()->sourceId(), m_exec->context()->currentBody()->sourceURL());
}
//END js exception handling

//BEGIN JS API STUFF

using namespace KJS;

//BEGIN global methods
class KateJSGlobalFunctions : public KJS::JSObject
{
  public:
    KateJSGlobalFunctions(int i, int length);

    virtual bool implementsCall() const { return true; }

    virtual KJS::JSValue* callAsFunction (KJS::ExecState *exec, KJS::JSObject *thisObj, const KJS::List &args);

    enum {
      Debug
    };

  private:
    int id;
};

KateJSGlobalFunctions::KateJSGlobalFunctions(int i, int length) : KJS::JSObject(), id(i)
{
  putDirect(Identifier("length"),length,KJS::DontDelete|KJS::ReadOnly|KJS::DontEnum);
}

JSValue* KateJSGlobalFunctions::callAsFunction (KJS::ExecState *exec, KJS::JSObject *thisObj, const KJS::List &args)
{
  switch (id) {
    case Debug:
      kDebug(13051) << args[0]->toString(exec).qstring() << endl;
      return KJS::Undefined();
    default:
      break;
  }

  return KJS::Undefined();
}
//END global methods

class KateJSGlobal : public KJS::JSObject {
public:
  virtual KJS::UString className() const { return "global"; }
};

class KateJSDocument : public KJS::JSObject
{
  public:
    KateJSDocument (KJS::ExecState *exec, KateDocument *_doc);

    bool getOwnPropertySlot( KJS::ExecState *exec, const KJS::Identifier &propertyName, KJS::PropertySlot &slot);

    KJS::JSValue* getValueProperty(KJS::ExecState *exec, int token) const;

    void put(KJS::ExecState *exec, const KJS::Identifier &propertyName, KJS::JSValue *value, int attr = KJS::None);

    void putValueProperty(KJS::ExecState *exec, int token, KJS::JSValue* value, int attr);

    const KJS::ClassInfo* classInfo() const { return &info; }

    enum {
      // common stuff
      FileName,
      Url,
      MimeType,
      Encoding,
      IsModified,

      // document manipulation
      Text,                 //
      TextRange,            // (line, column, line, column)
      TextLine,             // (line)
      WordAt,               // (line, column)
      CharAt,               // (line, column)
      FirstChar,            // (line)
      LastChar,             // (line)
      IndexOf,              // (line, string, [from=0], [bool=casesensitive])
      LastIndexOf,          // (line, string, [from=-1], [bool=casesensitive])
      IsSpace,              // (line, column)
      StartsWith,           // (line, string, [bool=do not ignore leading spaces])
      EndsWith,             // (line, string, [bool=do not ignore trailing spaces])
      MatchesAt,            // (line, column, string)

      SetText,              // (string)
      Clear,                //
      Truncate,             // (line, new-length)
      InsertText,           // (line, column, string)
      RemoveText,           // (line, column, line, column)
      InsertLine,           // (line, string)
      RemoveLine,           // (line)
      JoinLines,            // (start-line, end-line)

      Lines,                //
      Length,               //
      LineLength,           // (line)
      VirtualLineLength,    // (line, [tab-width=doc->config()->tabWidth()])

      EditBegin,
      EditEnd,

      // helper functions to speed up kjs
      FirstColumn,          // (line)
      FirstVirtualColumn,   // (line, [tab-width=doc->config()->tabWidth()])
      LastColumn,           // (line)
      LastVirtualColumn,    // (line, [tab-width=doc->config()->tabWidth()])
      ToVirtualColumn,      // (line, column, [tab-width=doc->config()->tabWidth()])
      FromVirtualColumn,    // (line, virtual-column, [tab-width=doc->config()->tabWidth()])
      PrevNonSpaceColumn,   // (line, column)
      NextNonSpaceColumn,   // (line, column)

      PrevNonEmptyLine,     // (line)
      NextNonEmptyLine,     // (line)

      FindLeftBrace,        // (line, column)
      FindLeftParenthesis,  // (line, column)
      FindStartOfCComment,  // (line, column)

      // highlighting
      IsInWord,             // (char, attribute)
      CanBreakAt,           // (char, attribute)
      CanComment,           // (start-attribute, end-attribute)
      CommentMarker,        // (attribute)
      CommentStart,         // (attribute)
      CommentEnd,           // (attribute)
      Attribute,            // (line, column)

      // variable interface
      Variable,             // (string)

      // config settings (properties)
      IndentWidth,
      TabWidth,
      IndentMode,
      HighlightMode,
      ExpandTabs
    };

  public:
    KateDocument *doc;

    static const KJS::ClassInfo info;
};

class KateJSView : public KJS::JSObject
{
  public:
    KateJSView (KJS::ExecState *exec, KateView *_view);

    bool getOwnPropertySlot( KJS::ExecState *exec, const KJS::Identifier &propertyName, KJS::PropertySlot &slot);

    KJS::JSValue* getValueProperty(KJS::ExecState *exec, int token) const;

    void put(KJS::ExecState *exec, const KJS::Identifier &propertyName, KJS::JSValue *value, int attr = KJS::None);

    void putValueProperty(KJS::ExecState *exec, int token, KJS::JSValue* value, int attr);

    const KJS::ClassInfo* classInfo() const { return &info; }

    enum {
      // cursor interface
      CursorPosition,            //
      SetCursorPosition,         // (line, column)
      VirtualCursorPosition,     //
      SetVirtualCursorPosition,  // (line, column)

      // selection interface
      Selection,
      HasSelection,
      SetSelection,
      RemoveSelection,
      SelectAll,
      ClearSelection,
      StartOfSelection,
      EndOfSelection,

      // indentation
      Indent
    };

  public:
    KateView *view;

    static const KJS::ClassInfo info;
};

#include "katejscript.lut.h"
//END

//BEGIN KateJSInterpreterContext
KateJSInterpreterContext::KateJSInterpreterContext (const QString &filename)
 : m_global (new KateJSGlobal ())
 , m_interpreter (new KJS::Interpreter (m_global))
 , m_document (new KateJSDocument(m_interpreter->globalExec(), 0))
 , m_view (new KateJSView(m_interpreter->globalExec(), 0))
{
  m_interpreter->ref();

  // add stuff that is not bound to a doc/view
  m_interpreter->globalObject()->put(m_interpreter->globalExec(), "debug",
      new KateJSGlobalFunctions(KateJSGlobalFunctions::Debug,1));

  // eval file, if any
  if (!filename.isEmpty())
  {
    kDebug(13051) << "read script: " << filename << endl;

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
      kDebug(13051) << i18n("Unable to read file: '%1'", filename);
      return;
    }

    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    QString source = stream.readAll();
    file.close();

    // parse + eval script....
    KJS::Completion comp (m_interpreter->evaluate("", 0, source));

    if (comp.complType() == KJS::Throw)
    {
      KJS::JSValue *exVal = comp.value();
      KJS::ExecState *exec = m_interpreter->globalExec();
      QString msg = exVal->toString(exec).qstring();

      int lineno = -1;
      if (exVal->type() == KJS::ObjectType)
      {
        KJS::JSValue *lineVal = exVal->getObject()->get(exec,"line");

        if (lineVal->type() == KJS::NumberType)
          lineno = lineVal->toInt32(exec);
      }

      kDebug(13051) << i18n("Exception, line %1: %2", lineno, msg);
      return;
    }
  }

  // now also add document and view. this should stay for all executions.
  m_interpreter->globalObject()->put(m_interpreter->globalExec(), "document", m_document);
  m_interpreter->globalObject()->put(m_interpreter->globalExec(), "view", m_view);
}

KateJSInterpreterContext::~KateJSInterpreterContext ()
{
  m_interpreter->deref();
  // NOTE: do not delete objects, the KJS garbage collection does this
  // big no no: delete m_global;
}

KJS::JSObject *KateJSInterpreterContext::wrapDocument(KJS::ExecState *exec, KateDocument *doc)
{
  return new KateJSDocument(exec, doc);
}

KJS::JSObject *KateJSInterpreterContext::wrapView(KJS::ExecState *exec, KateView *view)
{
  return new KateJSView(exec, view);
}


bool KateJSInterpreterContext::evalSource(KateView *view, const QString &script, QString &errorMsg)
{
  // no view, no fun
  if (!view)
  {
    errorMsg = i18n("Could not access view");
    return false;
  }

  // init doc & view with new pointers!
  static_cast<KateJSDocument *>( m_document )->doc = view->doc();
  static_cast<KateJSView *>( m_view )->view = view;

  // run the script for real
  view->doc()->pushEditState();
  KJS::Completion comp (m_interpreter->evaluate("", 0, script));
  view->doc()->popEditState();

  if (comp.complType() == KJS::Throw)
  {
    KJS::JSValue *exVal = comp.value();
    KJS::ExecState *exec = m_interpreter->globalExec();
    QString msg = exVal->toString(exec).qstring();

    int lineno = -1;
    if (exVal->type() == KJS::ObjectType)
    {
      KJS::JSValue *lineVal = exVal->getObject()->get(exec,"line");

      if (lineVal->type() == KJS::NumberType)
        lineno = lineVal->toInt32(exec);
    }

    errorMsg = i18n("Exception, line %1: %2", lineno, msg);
    return false;
  }

  return true;
}

bool KateJSInterpreterContext::evalFile(KateView* view, const QString& url, QString& error)
{
  QFile file(url);

  if (!file.open(QIODevice::ReadOnly)) {
    error = i18n("Unable to read file: '%1'", url);
    return false;
  }

  QTextStream stream(&file);
  stream.setCodec("UTF-8");
  QString source = stream.readAll();
  file.close();

  return evalSource(view, source, error);
}

KJS::JSValue* KateJSInterpreterContext::callFunction(KateView* view,
                                                     KJS::JSObject* lookupObj,
                                                     const KJS::Identifier& function,
                                                     KJS::List parameter,
                                                     QString& error)
{
  // no view, no fun
  if (!view) {
    error = i18n("Could not access view.");
    return 0;
  }

  if (!lookupObj) {
    error = i18n("Could not access lookup object.");
    return 0;
  }

  KJS::ExecState *exec = m_interpreter->globalExec();

  // lookup function
  KJS::JSObject *o = lookupObj->get(exec, function)->toObject(exec);
  if (exec->hadException()) {
    KJS::JSObject *thrownObj = exec->exception()->toObject(exec);
    QString message = thrownObj->get(exec, "message")->toString(exec).qstring();
    error = i18n("Exception: Unable to find function '%1': %2", function.qstring(), message);
    exec->clearException();
    return 0;
  }

  // init doc & view with new pointers!
  static_cast<KateJSDocument *>( m_document )->doc = view->doc();
  static_cast<KateJSView *>( m_view )->view = view;

  KJS::JSValue *retval = o->call(exec, m_interpreter->globalObject(), parameter);
  if (exec->hadException()) {
    KJS::JSObject *thrownObj = exec->exception()->toObject(exec);
    QString message = thrownObj->get(exec, "message")->toString(exec).qstring();

    int lineno = -1;
    KJS::JSValue *lineVal = thrownObj->get(exec, "line");
    if (lineVal->type() == KJS::NumberType)
      lineno = lineVal->toInt32(exec);

    error = i18n("Exception in line %1: %2", lineno, message);
    exec->clearException();
    return 0;
  }

  return retval;
}
//END KateJSInterpreterContext

//BEGIN KateJSDocument

// -------------------------------------------------------------------------
/* Source for KateJSDocumentProtoTable.
@begin KateJSDocumentProtoTable 52
#
# common document stuff
#
  fileName             KateJSDocument::FileName          DontDelete|Function 0
  url                  KateJSDocument::Url               DontDelete|Function 0
  mimeType             KateJSDocument::MimeType          DontDelete|Function 0
  encoding             KateJSDocument::Encoding          DontDelete|Function 0
  isModified           KateJSDocument::IsModified        DontDelete|Function 0
#
# edit interface stuff + editBegin/End
#
  text                 KateJSDocument::Text              DontDelete|Function 0
  textRange            KateJSDocument::TextRange         DontDelete|Function 4
  line                 KateJSDocument::TextLine          DontDelete|Function 1
  wordAt               KateJSDocument::WordAt            DontDelete|Function 2
  charAt               KateJSDocument::CharAt            DontDelete|Function 2
  firstChar            KateJSDocument::FirstChar         DontDelete|Function 1
  lastChar             KateJSDocument::LastChar          DontDelete|Function 1
  indexOf              KateJSDocument::IndexOf           DontDelete|Function 4
  lastIndexOf          KateJSDocument::LastIndexOf       DontDelete|Function 4
  isSpace              KateJSDocument::IsSpace           DontDelete|Function 2
  startsWith           KateJSDocument::StartsWith        DontDelete|Function 3
  endsWith             KateJSDocument::EndsWith          DontDelete|Function 3
  matchesAt            KateJSDocument::MatchesAt         DontDelete|Function 3
# FIXME: add
#        search(start-line, start-col, text, bool case-sensitive)
#        searchBackwards(start-line, start-col, text, bool case-sensitive)
#        replaceText(line, column, line, column, string)
#        isBalanced(line, column, line, column, open, close)
#        add case-sensitiv parameter to StartsWith, EndsWith, MatchesAt

  setText              KateJSDocument::SetText           DontDelete|Function 1
  clear                KateJSDocument::Clear             DontDelete|Function 0
  truncate             KateJSDocument::Truncate          DontDelete|Function 2
  insertText           KateJSDocument::InsertText        DontDelete|Function 3
  removeText           KateJSDocument::RemoveText        DontDelete|Function 4
  insertLine           KateJSDocument::InsertLine        DontDelete|Function 2
  removeLine           KateJSDocument::RemoveLine        DontDelete|Function 1
  joinLines            KateJSDocument::JoinLines         DontDelete|Function 2

  lines                KateJSDocument::Lines             DontDelete|Function 0
  length               KateJSDocument::Length            DontDelete|Function 0
  lineLength           KateJSDocument::LineLength        DontDelete|Function 1
  virtualLineLength    KateJSDocument::VirtualLineLength DontDelete|Function 2

  editBegin            KateJSDocument::EditBegin         DontDelete|Function 0
  editEnd              KateJSDocument::EditEnd           DontDelete|Function 0
#
# common textline helpers to speed up kjs
#
  firstColumn          KateJSDocument::FirstColumn         DontDelete|Function 1
  firstVirtualColumn   KateJSDocument::FirstVirtualColumn  DontDelete|Function 2
  lastColumn           KateJSDocument::LastColumn          DontDelete|Function 1
  lastVirtualColumn    KateJSDocument::LastVirtualColumn   DontDelete|Function 2
  toVirtualColumn      KateJSDocument::ToVirtualColumn     DontDelete|Function 1
  fromVirtualColumn    KateJSDocument::FromVirtualColumn   DontDelete|Function 1
  prevNonSpaceColumn   KateJSDocument::PrevNonSpaceColumn  DontDelete|Function 2
  nextNonSpaceColumn   KateJSDocument::NextNonSpaceColumn  DontDelete|Function 2

  prevNonEmptyLine     KateJSDocument::PrevNonEmptyLine    DontDelete|Function 1
  nextNonEmptyLine     KateJSDocument::NextNonEmptyLine    DontDelete|Function 1

  findLeftBrace        KateJSDocument::FindLeftBrace       DontDelete|Function 2
  findLeftParenthesis  KateJSDocument::FindLeftParenthesis DontDelete|Function 2
  findStartOfCComment  KateJSDocument::FindStartOfCComment DontDelete|Function 2
#
# methods from highlight (and around)
#
  isInWord       KateJSDocument::IsInWord         DontDelete|Function 2
  canBreakAt     KateJSDocument::CanBreakAt       DontDelete|Function 2
  canComment     KateJSDocument::CanComment       DontDelete|Function 2
  commentMarker  KateJSDocument::CommentMarker    DontDelete|Function 1
  commentStart   KateJSDocument::CommentStart     DontDelete|Function 1
  commentEnd     KateJSDocument::CommentEnd       DontDelete|Function 1
  attribute      KateJSDocument::Attribute        DontDelete|Function 2
#
# variable/modeline interface
#
  variable       KateJSDocument::Variable        DontDelete|Function 1
@end

@begin KateJSDocumentTable 5
#
# Configuration properties
#
  indentWidth     KateJSDocument::IndentWidth   DontDelete|ReadOnly
  tabWidth        KateJSDocument::TabWidth      DontDelete|ReadOnly
  indentMode      KateJSDocument::IndentMode    DontDelete|ReadOnly
  highlightMode   KateJSDocument::HighlightMode DontDelete|ReadOnly
  expandTabs      KateJSDocument::ExpandTabs    DontDelete|ReadOnly
@end
*/

KJS_DEFINE_PROTOTYPE(KateJSDocumentProto)
KJS_IMPLEMENT_PROTOFUNC(KateJSDocumentProtoFunc)
KJS_IMPLEMENT_PROTOTYPE("KateJSDocument", KateJSDocumentProto, KateJSDocumentProtoFunc)

const KJS::ClassInfo KateJSDocument::info = { "KateJSDocument", 0, 0, 0 };

JSValue* KateJSDocumentProtoFunc::callAsFunction(KJS::ExecState *exec,
                                                 KJS::JSObject *thisObj,
                                                 const KJS::List &args)
{
  KJS_CHECK_THIS( KateJSDocument, thisObj );

  KateDocument *doc = static_cast<KateJSDocument *>( thisObj )->doc;

  if (!doc)
    return KJS::Undefined();

  KateJSExceptionTranslator exception(exec, KateJSDocumentProtoTable, id, args);

  switch (id)
  {
    case KateJSDocument::FileName:
      if (exception.invalidArgs(0)) break;
      return KJS::String (doc->documentName());

    case KateJSDocument::Url:
      if (exception.invalidArgs(0)) break;
      return KJS::String (doc->url().prettyUrl());

    case KateJSDocument::MimeType:
      if (exception.invalidArgs(0)) break;
      return KJS::String (doc->mimeType());

    case KateJSDocument::Encoding:
      if (exception.invalidArgs(0)) break;
      return KJS::String (doc->encoding());

    case KateJSDocument::IsModified:
      if (exception.invalidArgs(0)) break;
      return KJS::Boolean (doc->isModified());

    case KateJSDocument::Text:
      if (exception.invalidArgs(0)) break;
      return KJS::String (doc->text());

    case KateJSDocument::TextRange:
      if (exception.invalidArgs(4)) break;
      return KJS::String (doc->text(KTextEditor::Range(args[0]->toUInt32(exec),
                                                       args[1]->toUInt32(exec),
                                                       args[2]->toUInt32(exec),
                                                       args[3]->toUInt32(exec))));

    case KateJSDocument::TextLine:
      if (exception.invalidArgs(1)) break;
      return KJS::String (doc->line (args[0]->toUInt32(exec)));

    case KateJSDocument::WordAt:
      if (exception.invalidArgs(2)) break;
      return KJS::String (doc->getWord(KTextEditor::Cursor(args[0]->toUInt32(exec),
                                                           args[1]->toUInt32(exec))));

    case KateJSDocument::CharAt: {
      if (exception.invalidArgs(2)) break;
      const QChar c = doc->character (KTextEditor::Cursor(args[0]->toUInt32(exec),
                                                          args[1]->toUInt32(exec)));
      return KJS::String (c.isNull() ? "" : QString(c));
    }

    case KateJSDocument::FirstChar: {
      if (exception.invalidArgs(1)) break;
      KateTextLine::Ptr textLine = doc->plainKateTextLine(args[0]->toUInt32(exec));
      if (!textLine) return KJS::String("");
      // check for isNull(), as the returned character then would be "\0"
      const QChar c = textLine->at(textLine->firstChar());
      return KJS::String (c.isNull() ? "" : QString(c));
    }

    case KateJSDocument::LastChar: {
      if (exception.invalidArgs(1)) break;
      KateTextLine::Ptr textLine = doc->plainKateTextLine(args[0]->toUInt32(exec));
      if (!textLine) return KJS::String("");
      // check for isNull(), as the returned character then would be "\0"
      const QChar c = textLine->at(textLine->lastChar());
      return KJS::String (c.isNull() ? "" : QString(c));
    }

    case KateJSDocument::IndexOf: {
      if (exception.invalidArgs(2, 4)) break;
      const int from = args.size() >=3 ? args[2]->toUInt32(exec) : 0;
      Qt::CaseSensitivity cs = Qt::CaseSensitive;
      if (args.size() == 4 && !args[3]->toBoolean(exec))
        cs = Qt::CaseInsensitive;
      return KJS::Number (doc->line(args[0]->toUInt32(exec)).indexOf(args[1]->toString(exec).qstring(), from, cs));
    }

    case KateJSDocument::LastIndexOf: {
      if (exception.invalidArgs(2, 4)) break;
      const int from = args.size() >=3 ? (int)args[2]->toUInt32(exec) : -1;
      Qt::CaseSensitivity cs = Qt::CaseSensitive;
      if (args.size() == 4 && !args[3]->toBoolean(exec))
        cs = Qt::CaseInsensitive;
      return KJS::Number (doc->line(args[0]->toUInt32(exec)).lastIndexOf(args[1]->toString(exec).qstring(), from, cs));
    }

    case KateJSDocument::IsSpace:
      if (exception.invalidArgs(2)) break;
      return KJS::Boolean (doc->character (KTextEditor::Cursor(args[0]->toUInt32(exec),
                                                               args[1]->toUInt32(exec))).isSpace());

    case KateJSDocument::StartsWith: {
      if (exception.invalidArgs(2, 3)) break;
      if (args.size() == 3 && args[2]->toBoolean(exec)) {
        KateTextLine::Ptr textLine = doc->plainKateTextLine(args[0]->toUInt32(exec));
        return KJS::Boolean(textLine && textLine->matchesAt(textLine->firstChar(), args[1]->toString(exec).qstring()));
      } else {
        return KJS::Boolean(doc->line(args[0]->toUInt32(exec)).startsWith(args[1]->toString(exec).qstring()));
      }
    }

    case KateJSDocument::EndsWith: {
      if (exception.invalidArgs(2, 3)) break;
      if (args.size() == 3 && args[2]->toBoolean(exec)) {
        KateTextLine::Ptr textLine = doc->plainKateTextLine(args[0]->toUInt32(exec));
        const QString match = args[1]->toString(exec).qstring();
        return KJS::Boolean( textLine && textLine->matchesAt(textLine->lastChar() - match.length() + 1, match) );
      } else {
        return KJS::Boolean(doc->line(args[0]->toUInt32(exec)).endsWith(args[1]->toString(exec).qstring()));
      }
    }

    case KateJSDocument::MatchesAt: {
      if (exception.invalidArgs(3)) break;
      KateTextLine::Ptr textLine = doc->plainKateTextLine(args[0]->toUInt32(exec));
      return KJS::Boolean (textLine ? textLine->matchesAt(args[1]->toUInt32(exec),
                                                          args[2]->toString(exec).qstring()) : false);
    }

    case KateJSDocument::SetText:
      if (exception.invalidArgs(1)) break;
      return KJS::Boolean (doc->setText(args[0]->toString(exec).qstring()));

    case KateJSDocument::Clear:
      if (exception.invalidArgs(0)) break;
      return KJS::Boolean (doc->clear());

    case KateJSDocument::Truncate: {
      if (exception.invalidArgs(2)) break;
      KateTextLine::Ptr textLine = doc->plainKateTextLine(args[0]->toUInt32(exec));
      if (textLine) textLine->truncate(args[1]->toUInt32(exec));
      return KJS::Boolean (static_cast<bool>(textLine));
    }

    case KateJSDocument::InsertText:
      if (exception.invalidArgs(3)) break;
      return KJS::Boolean (doc->insertText (KTextEditor::Cursor(args[0]->toUInt32(exec),
                                                                args[1]->toUInt32(exec)),
                                                                args[2]->toString(exec).qstring()));

    case KateJSDocument::RemoveText:
      if (exception.invalidArgs(4)) break;
      return KJS::Boolean (doc->removeText(KTextEditor::Range(args[0]->toUInt32(exec),
                                                              args[1]->toUInt32(exec),
                                                              args[2]->toUInt32(exec),
                                                              args[3]->toUInt32(exec))));

    case KateJSDocument::InsertLine:
      if (exception.invalidArgs(2)) break;
      return KJS::Boolean (doc->insertLine (args[0]->toUInt32(exec),
                                            args[1]->toString(exec).qstring()));

    case KateJSDocument::RemoveLine:
      if (exception.invalidArgs(1)) break;
      return KJS::Boolean (doc->removeLine (args[0]->toUInt32(exec)));

    case KateJSDocument::JoinLines:
      if (exception.invalidArgs(2)) break;
      doc->joinLines (args[0]->toUInt32(exec), args[1]->toUInt32(exec));
      return KJS::Null();

    case KateJSDocument::Lines:
      if (exception.invalidArgs(0)) break;
      return KJS::Number (doc->lines());

    case KateJSDocument::Length:
      if (exception.invalidArgs(0)) break;
      return KJS::Number (doc->totalCharacters());

    case KateJSDocument::LineLength:
      if (exception.invalidArgs(1)) break;
      return KJS::Number (doc->lineLength(args[0]->toUInt32(exec)));

    case KateJSDocument::VirtualLineLength: {
      if (exception.invalidArgs(1, 2)) break;
      const int tabWidth = (args.size() == 2) ? args[1]->toUInt32(exec) : doc->config()->tabWidth();
      const uint line = args[0]->toUInt32(exec);
      KateTextLine::Ptr textLine = doc->plainKateTextLine(line);
      if (!textLine) return KJS::Number(-1);
      return KJS::Number(textLine->virtualLength(tabWidth));
    }

    case KateJSDocument::EditBegin:
      if (exception.invalidArgs(0)) break;
      doc->editBegin();
      return KJS::Null ();

    case KateJSDocument::EditEnd:
      if (exception.invalidArgs(0)) break;
      doc->editEnd ();
      return KJS::Null ();

    case KateJSDocument::FirstColumn: {
      if (exception.invalidArgs(1)) break;
      KateTextLine::Ptr textLine = doc->plainKateTextLine(args[0]->toUInt32(exec));
      return KJS::Number(textLine ? textLine->firstChar() : -1);
    }

    case KateJSDocument::FirstVirtualColumn: {
      if (exception.invalidArgs(1, 2)) break;
      const int tabWidth = (args.size() == 2) ? args[1]->toUInt32(exec) : doc->config()->tabWidth();
      KateTextLine::Ptr textLine = doc->plainKateTextLine(args[0]->toUInt32(exec));
      const int firstPos = textLine ? textLine->firstChar() : -1;
      if (!textLine || firstPos == -1) return KJS::Number(-1);
      return KJS::Number(textLine->indentDepth(tabWidth));
    }

    case KateJSDocument::LastColumn: {
      if (exception.invalidArgs(1)) break;
      KateTextLine::Ptr textLine = doc->plainKateTextLine(args[0]->toUInt32(exec));
      return KJS::Number(textLine ? textLine->lastChar() : -1);
    }

    case KateJSDocument::LastVirtualColumn: {
      if (exception.invalidArgs(1, 2)) break;
      const int tabWidth = (args.size() == 2) ? args[1]->toUInt32(exec) : doc->config()->tabWidth();
      KateTextLine::Ptr textLine = doc->plainKateTextLine(args[0]->toUInt32(exec));
      const int lastPos = textLine ? textLine->lastChar() : -1;
      if (!textLine || lastPos == -1) return KJS::Number(-1);
      return KJS::Number(textLine->toVirtualColumn(lastPos, tabWidth));
    }

    case KateJSDocument::ToVirtualColumn: {
      if (exception.invalidArgs(2, 3)) break;
      const int tabWidth = (args.size() == 3) ? args[2]->toUInt32(exec) : doc->config()->tabWidth();
      KateTextLine::Ptr textLine = doc->plainKateTextLine(args[0]->toUInt32(exec));
      const int column = args[1]->toUInt32(exec);
      if (!textLine || column < 0 || column > textLine->length()) return KJS::Number(-1);
      return KJS::Number(textLine->toVirtualColumn(column, tabWidth));
    }

    case KateJSDocument::FromVirtualColumn: {
      if (exception.invalidArgs(2, 3)) break;
      const int tabWidth = (args.size() == 3) ? args[2]->toUInt32(exec) : doc->config()->tabWidth();
      KateTextLine::Ptr textLine = doc->plainKateTextLine(args[0]->toUInt32(exec));
      const int vcolumn = args[1]->toUInt32(exec);
      if (!textLine || vcolumn < 0 || vcolumn > textLine->virtualLength(tabWidth)) return KJS::Number(-1);
      return KJS::Number(textLine->fromVirtualColumn(vcolumn, tabWidth));
    }

    case KateJSDocument::PrevNonSpaceColumn: {
      if (exception.invalidArgs(2)) break;
      KateTextLine::Ptr textLine = doc->plainKateTextLine(args[0]->toUInt32(exec));
      if (!textLine) return KJS::Number(-1);
      return KJS::Number(textLine->previousNonSpaceChar(args[1]->toUInt32(exec)));
    }

    case KateJSDocument::NextNonSpaceColumn: {
      if (exception.invalidArgs(2)) break;
      KateTextLine::Ptr textLine = doc->plainKateTextLine(args[0]->toUInt32(exec));
      if (!textLine) return KJS::Number(-1);
      return KJS::Number(textLine->nextNonSpaceChar(args[1]->toUInt32(exec)));
    }

    case KateJSDocument::PrevNonEmptyLine: {
      if (exception.invalidArgs(1)) break;
      const int startLine = args[0]->toUInt32(exec);
      for (int currentLine = startLine; currentLine >= 0; --currentLine) {
        KateTextLine::Ptr textLine = doc->plainKateTextLine(currentLine);
        if (!textLine)
          return KJS::Number(-1);
        if (textLine->firstChar() != -1)
          return KJS::Number(currentLine);
      }
      return KJS::Number(-1);
    }

    case KateJSDocument::NextNonEmptyLine: {
      if (exception.invalidArgs(1)) break;
      const int startLine = args[0]->toUInt32(exec);
      for (int currentLine = startLine; currentLine < doc->lines(); ++currentLine) {
        KateTextLine::Ptr textLine = doc->plainKateTextLine(currentLine);
        if (!textLine)
          return KJS::Number(-1);
        if (textLine->firstChar() != -1)
          return KJS::Number(currentLine);
      }
      return KJS::Number(-1);
    }

    case KateJSDocument::FindLeftBrace: {
      if (exception.invalidArgs(2)) break;
      KateDocCursor cursor(args[0]->toUInt32(exec), args[1]->toUInt32(exec), doc);
      int count = 1;

      // Move backwards char by char and find the opening brace
      while (cursor.moveBackward(1)) {
        QChar ch = cursor.currentChar();
        if (ch == '{') {
          --count;
        } else if (ch == '}') {
          ++count;
        }

        if (count == 0) {
          KJS::JSObject *object = exec->lexicalInterpreter()->builtinObject()->construct(exec, KJS::List::empty());
          object->put(exec, "line", KJS::Number(cursor.line()));
          object->put(exec, "column", KJS::Number(cursor.column()));
          return object;
        }
      }

      return KJS::Null();
    }

    case KateJSDocument::FindLeftParenthesis: {
      if (exception.invalidArgs(2)) break;
      KateDocCursor cursor(args[0]->toUInt32(exec), args[1]->toUInt32(exec), doc);
      int count = 1;

      // Move backwards char by char and find the opening parenthesis
      while (cursor.moveBackward(1)) {
        QChar ch = cursor.currentChar();
        if (ch == '(') {
          --count;
        } else if (ch == ')') {
          ++count;
        }

        if (count == 0) {
          KJS::JSObject *object = exec->lexicalInterpreter()->builtinObject()->construct(exec, KJS::List::empty());
          object->put(exec, "line", KJS::Number(cursor.line()));
          object->put(exec, "column", KJS::Number(cursor.column()));
          return object;
        }
      }

      return KJS::Null();
    }

    case KateJSDocument::FindStartOfCComment: {
      if (exception.invalidArgs(2)) break;
      KateDocCursor cursor(args[0]->toUInt32(exec), args[1]->toUInt32(exec), doc);

      // find the line with the opening "/*"
      do {
        KateTextLine::Ptr textLine = doc->plainKateTextLine(cursor.line());
        if (!textLine)
          break;

        int column = textLine->string().indexOf("/*");
        if (column >= 0) {
          KJS::JSObject *object = exec->lexicalInterpreter()->builtinObject()->construct(exec, KJS::List::empty());
          object->put(exec, "line", KJS::Number(cursor.line()));
          object->put(exec, "column", KJS::Number(column));
          return object;
        }
      } while (cursor.gotoPreviousLine());

      return KJS::Null();
    }

    case KateJSDocument::IsInWord:
      if (exception.invalidArgs(2)) break;
      return KJS::Boolean( doc->highlight()->isInWord( args[0]->toString(exec).qstring().at(0),
                                                       args[1]->toUInt32(exec) ) );

    case KateJSDocument::CanBreakAt:
      if (exception.invalidArgs(2)) break;
      return KJS::Boolean( doc->highlight()->canBreakAt( args[0]->toString(exec).qstring().at(0),
                                                         args[1]->toUInt32(exec) ) );

    case KateJSDocument::CanComment:
      if (exception.invalidArgs(2)) break;
      return KJS::Boolean( doc->highlight()->canComment( args[0]->toUInt32(exec),
                                                         args[1]->toUInt32(exec) ) );

    case KateJSDocument::CommentMarker:
      if (exception.invalidArgs(1)) break;
      return KJS::String( doc->highlight()->getCommentSingleLineStart( args[0]->toUInt32(exec) ) );

    case KateJSDocument::CommentStart:
      if (exception.invalidArgs(1)) break;
      return KJS::String( doc->highlight()->getCommentStart( args[0]->toUInt32(exec) ) );

    case KateJSDocument::CommentEnd:
      if (exception.invalidArgs(1)) break;
      return KJS::String( doc->highlight()->getCommentEnd( args[0]->toUInt32(exec) ) );

    case KateJSDocument::Attribute: {
      if (exception.invalidArgs(2)) break;
      KateTextLine::Ptr textLine = doc->kateTextLine(args[0]->toUInt32(exec));
      if (!textLine) return KJS::Number(0);
      return KJS::Number( textLine->attribute(args[1]->toUInt32(exec)) );
    }

    case KateJSDocument::Variable:
      if (exception.invalidArgs(1)) break;
      return KJS::String( doc->variable(args[0]->toString(exec).qstring()) );

    default:
      kDebug(13051) << "Document: Unknown function id: " << id << endl;
  }

  return KJS::Undefined();
}

bool KateJSDocument::getOwnPropertySlot( KJS::ExecState *exec,
                                         const  KJS::Identifier &propertyName,
                                         KJS::PropertySlot &slot)
{
  return KJS::getStaticValueSlot<KateJSDocument, KJS::JSObject>(exec, &KateJSDocumentTable, this, propertyName, slot);
}

KJS::JSValue* KateJSDocument::getValueProperty(KJS::ExecState *exec, int token) const
{
  if (!doc)
    return KJS::Undefined ();

  switch (token) {
    case KateJSDocument::IndentWidth:
      return KJS::Number( doc->config()->indentationWidth() );

    case KateJSDocument::TabWidth:
      return KJS::Number( doc->config()->tabWidth() );

    case KateJSDocument::IndentMode:
      return KJS::String( doc->config()->indentationMode() );

    case KateJSDocument::HighlightMode:
      return KJS::String( doc->hlModeName( doc->hlMode() ) );

    case KateJSDocument::ExpandTabs:
      return KJS::Boolean( doc->config()->configFlags() & KateDocumentConfig::cfReplaceTabsDyn );

    default:
      kDebug(13051) << "Document: Unknown property id: " << token << endl;
  }

  return KJS::Undefined ();
}

void KateJSDocument::put(KJS::ExecState *exec,
                         const KJS::Identifier &propertyName,
                         KJS::JSValue *value,
                         int attr)
{
  KJS::lookupPut<KateJSDocument,KJS::JSObject>(exec, propertyName, value, attr, &KateJSDocumentTable, this );
}

void KateJSDocument::putValueProperty(KJS::ExecState *exec,
                                      int token,
                                      KJS::JSValue* value,
                                      int attr)
{
}

KateJSDocument::KateJSDocument (KJS::ExecState *exec, KateDocument *_doc)
  : KJS::JSObject (KateJSDocumentProto::self(exec)),
    doc (_doc)
{
}

//END

//BEGIN KateJSView

// -------------------------------------------------------------------------
/* Source for KateJSViewProtoTable.
@begin KateJSViewProtoTable 10
#
# cursor interface
#
  cursorPosition           KateJSView::CursorPosition           DontDelete|Function 0
  setCursorPosition        KateJSView::SetCursorPosition        DontDelete|Function 2
  virtualCursorPosition    KateJSView::VirtualCursorPosition    DontDelete|Function 0
  setVirtualCursorPosition KateJSView::SetVirtualCursorPosition DontDelete|Function 2
#
# selection interface
#
  selection                KateJSView::Selection                DontDelete|Function 0
  hasSelection             KateJSView::HasSelection             DontDelete|Function 0
  setSelection             KateJSView::SetSelection             DontDelete|Function 4
  removeSelection          KateJSView::RemoveSelection          DontDelete|Function 0
  selectAll                KateJSView::SelectAll                DontDelete|Function 0
  clearSelection           KateJSView::ClearSelection           DontDelete|Function 0
  startOfSelection         KateJSView::StartOfSelection         DontDelete|Function 0
  endOfSelection           KateJSView::EndOfSelection           DontDelete|Function 0
#
# indentation
#
  indent                   KateJSView::Indent                   DontDelete|Function 3
@end
*/

/* Source for KateJSViewTable.
@begin KateJSViewTable 0
#  startOfSelection         KateJSView::StartOfSelection         DontDelete|ReadOnly
#  virtualStartOfSelection  KateJSView::VirtualStartOfSelection  DontDelete|ReadOnly
#  endOfSelection           KateJSView::EndOfSelection           DontDelete|ReadOnly
#  virtualEndOfSelection    KateJSView::VirtualEndOfSelection    DontDelete|ReadOnly
@end
*/


KJS_DEFINE_PROTOTYPE(KateJSViewProto)
KJS_IMPLEMENT_PROTOFUNC(KateJSViewProtoFunc)
KJS_IMPLEMENT_PROTOTYPE("KateJSView",KateJSViewProto,KateJSViewProtoFunc)

const KJS::ClassInfo KateJSView::info = { "KateJSView", 0, &KateJSViewTable, 0 };

JSValue* KateJSViewProtoFunc::callAsFunction(KJS::ExecState *exec,
                                             KJS::JSObject *thisObj,
                                             const KJS::List &args)
{
  KJS_CHECK_THIS( KateJSView, thisObj );

  KateView *view = static_cast<KateJSView *>( thisObj )->view;

  if (!view)
    return KJS::Undefined();

  KateJSExceptionTranslator exception(exec, KateJSViewProtoTable, id, args);

  switch (id)
  {
    case KateJSView::CursorPosition: {
      if (exception.invalidArgs(0)) break;
      KJS::JSObject *object = exec->lexicalInterpreter()->builtinObject()->construct(exec, KJS::List::empty());
      object->put(exec, "line", KJS::Number(view->cursorPosition().line()));
      object->put(exec, "column", KJS::Number(view->cursorPosition().column()));
      return object;
    }

    case KateJSView::SetCursorPosition:
      if (exception.invalidArgs(2)) break;
      return KJS::Boolean( view->setCursorPosition( KTextEditor::Cursor (args[0]->toUInt32(exec),
                                                                         args[1]->toUInt32(exec)) ) );

    case KateJSView::VirtualCursorPosition: {
      if (exception.invalidArgs(0)) break;
      KJS::JSObject *object = exec->lexicalInterpreter()->builtinObject()->construct(exec, KJS::List::empty());
      object->put(exec, "line", KJS::Number(view->cursorPosition().line()));
      object->put(exec, "column", KJS::Number(view->cursorPositionVirtual().column()));
      return object;
    }

    case KateJSView::SetVirtualCursorPosition:
      if (exception.invalidArgs(0)) break;
      return KJS::Boolean( view->setCursorPositionVisual( KTextEditor::Cursor (args[0]->toUInt32(exec),
                                                                               args[1]->toUInt32(exec)) ) );

    case KateJSView::Selection:
      if (exception.invalidArgs(0)) break;
      return KJS::String( view->selectionText() );

    case KateJSView::HasSelection:
      if (exception.invalidArgs(0)) break;
      return KJS::Boolean( view->selection() );

    case KateJSView::SetSelection:
      if (exception.invalidArgs(4)) break;
      return KJS::Boolean( view->setSelection(KTextEditor::Range(args[0]->toInt32(exec),
                                                                 args[1]->toInt32(exec),
                                                                 args[2]->toUInt32(exec),
                                                                 args[3]->toUInt32(exec))));

    case KateJSView::RemoveSelection:
      if (exception.invalidArgs(0)) break;
      return KJS::Boolean( view->removeSelectionText() );

    case KateJSView::SelectAll:
      if (exception.invalidArgs(0)) break;
      return KJS::Boolean( view->selectAll() );

    case KateJSView::ClearSelection:
      if (exception.invalidArgs(0)) break;
      return KJS::Boolean( view->clearSelection() );

    case KateJSView::StartOfSelection: {
      if (exception.invalidArgs(0)) break;
      if (!view->selection())
        return KJS::Null();

      KJS::JSObject *object = exec->lexicalInterpreter()->builtinObject()->construct(exec, KJS::List::empty());
      object->put(exec, "line", KJS::Number(view->selectionRange().start().line()));
      object->put(exec, "column", KJS::Number(view->selectionRange().start().column()));
      return object;
    }

    case KateJSView::EndOfSelection: {
      if (exception.invalidArgs(0)) break;
      if (!view->selection())
        return KJS::Null();

      KJS::JSObject *object = exec->lexicalInterpreter()->builtinObject()->construct(exec, KJS::List::empty());
      object->put(exec, "line", KJS::Number(view->selectionRange().end().line()));
      object->put(exec, "column", KJS::Number(view->selectionRange().end().column()));
      return object;
    }

    case KateJSView::Indent:
      if (exception.invalidArgs(3)) break;
      //return KJS::Boolean( view->indent (args[0]->toInt32(exec), args[1]->toInt32(exec), args[2]->toUInt32(exec) )) );

    default:
      kDebug(13051) << "View: Unknown function id: " << id << endl;
  }

  return KJS::Undefined();
}

KateJSView::KateJSView (KJS::ExecState *exec, KateView *_view)
    : KJS::JSObject (KateJSViewProto::self(exec))
    , view (_view)
{
}

bool KateJSView::getOwnPropertySlot( KJS::ExecState *exec, const  KJS::Identifier &propertyName, KJS::PropertySlot &slot)
{
  return KJS::getStaticValueSlot<KateJSView,KJS::JSObject>(exec, &KateJSViewTable, this, propertyName, slot );
}

KJS::JSValue* KateJSView::getValueProperty(KJS::ExecState *exec, int token) const
{
  if (!view)
    return KJS::Undefined ();

  switch (token) {
    default:
      kDebug(13051) << "View: Unknown property id: " << token << endl;
  }

  return KJS::Undefined ();
}

void KateJSView::put(KJS::ExecState *exec, const KJS::Identifier &propertyName, KJS::JSValue* value, int attr)
{
   KJS::lookupPut<KateJSView,KJS::JSObject>(exec, propertyName, value, attr, &KateJSViewTable, this );
}

void KateJSView::putValueProperty(KJS::ExecState *exec, int token, KJS::JSValue* value, int attr)
{
}
//END

//BEGIN KateJScriptManager

KateJScriptManager::KateJScriptManager ()
  : KTextEditor::Command()
{
  // collect all scripts available...
  collectScripts ();
}

KateJScriptManager::~KateJScriptManager ()
{
  qDeleteAll(m_scripts);
}

void KateJScriptManager::collectScripts (bool force)
{
  // clear old list...
  qDeleteAll(m_scripts);
  m_scripts.clear ();

  // get list of all scripts + given headers
  QStringList keys(QStringList() << "name" << "author" << "license"
                                 << "version" << "kate-version"
                                 << "type"
                                 << "functions");
  KateJScriptHeaderVector scripts;
  scripts = KateJScriptHelpers::findScripts(QString("katepartjscriptrc"), QString("katepart/jscript/*.js"), keys);

  // Let's iterate through the list and build the Mode List
  for (KateJScriptHeaderVector::iterator it = scripts.begin(); it != scripts.end(); ++it)
  {
    // get abs filename....
    QFileInfo fi(it->url);
    QString absPath = fi.absoluteFilePath();
    QString baseName = fi.baseName ();

    // skip multiple entries...
    if (m_scripts.contains(baseName))
      continue;

    kDebug (13050) << "add script: " << baseName << " - " << absPath << endl;

    KateJScriptManager::Script *s = new KateJScriptManager::Script ();
    s->basename = baseName;
    s->filename = absPath;

    // remember important pairs...
    s->name = it->pairs.value ("name");
    s->author = it->pairs.value ("author");
    s->license = it->pairs.value ("license");
    s->version = it->pairs.value ("version");
    s->kateVersion = it->pairs.value ("kate-version");
    s->type = it->pairs.value ("type");
    s->functions = it->pairs.value ("functions").split (' ', QString::SkipEmptyParts);

    kDebug (13050) << "type: " << s->type << endl;
    kDebug (13050) << "functions: " << s->functions << endl;

    foreach (QString fun, s->functions)
      m_function2Script.insert (fun, s);

    // remember this.....
    m_scripts.insert (s->basename, s);
    m_types[s->type] << s->basename;
  }

  // handle all indentation scripts...  // Let's iterate through the list and build the Mode List
  QStringList indents = m_types.value ("indentation");
  for (int i = 0; i < indents.size(); ++i)
  {
    kDebug (13050) << "add indent-script: " << indents[i] << endl;

    // construct and remember indentation script...
    KateIndentJScript *s = new KateIndentJScript(indents[i], m_scripts.value (indents[i]));
    m_indentationScripts.insert (indents[i], s);
    m_indentationScriptsList.append(s);
  }

}

KateJSInterpreterContext *KateJScriptManager::interpreter (const QString &name, bool persistent)
{
  // get the script
  KateJScriptManager::Script *s = m_scripts.value (name);

  // script not found...
  if (!s)
    return 0;

  // remember the flag...
  s->persistent = persistent;

  // get the interpreter...
  if (s->interpreter)
    return s->interpreter;

  // construct + return interpreter....
  return s->interpreter = new KateJSInterpreterContext (s->filename);
}

bool KateJScriptManager::exec( KTextEditor::View *view, const QString &_cmd, QString &errorMsg )
{
  if (!view) {
    errorMsg = i18n("Could not access view");
    return false;
  }

  QStringList args( _cmd.split( QRegExp("\\s+") ,QString::SkipEmptyParts) );
  QString cmd ( args.first() );
  args.removeFirst();

  KateView* kateView = qobject_cast<KateView*>(view);

  if (cmd == QLatin1String("js-run-myself"))
  {
    KateJSInterpreterContext script ("");
    return script.evalSource (kateView, kateView->doc()->text(), errorMsg);
  }

  KateJScriptManager::Script *script = m_function2Script.value (cmd);

  if (!script) {
    errorMsg = i18n("Command not found: %1", cmd);
    return false;
  }

  KateJSInterpreterContext *inter = interpreter (script->basename);

  if (!inter)
  {
    errorMsg = i18n("Failed to start interpreter for script %1, command %2", script->basename, cmd);
    return false;
  }

  KJS::List params;

  foreach (QString a, args)
    params.append(KJS::String(a));

  KJS::JSValue *val = inter->callFunction(kateView, inter->interpreter()->globalObject(), KJS::Identifier(cmd),
                                   params, errorMsg);

  return true;
}

bool KateJScriptManager::help( KTextEditor::View *, const QString &cmd, QString &msg )
{
 /* if (cmd == "js-run-myself") {
    msg = i18n("This executes the current document as a javascript within kate");
    return true;
  }

  if (!m_scripts.contains(cmd))
    return false;

  msg = m_scripts[cmd]->help;

  return !msg.isEmpty();*/
  return true;
}

const QStringList &KateJScriptManager::cmds()
{
  static QStringList l;

  l.clear();
  l << "js-run-myself";

  QHashIterator<QString, KateJScriptManager::Script*> i(m_function2Script);
  while (i.hasNext()) {
      i.next();
      l << i.key();
  }

  return l;
}
//END

//BEGIN KateIndentJScript
KateIndentJScript::KateIndentJScript(const QString& basename, KateJScriptManager::Script *info)
  : m_info (info), m_script(0),
    m_basename(basename),
    m_triggerCharactersSet(false)
{
}

KateIndentJScript::~KateIndentJScript()
{
  unloadInterpreter();
}

void KateIndentJScript::loadInterpreter(KateView* view)
{
  if (!m_script)
  {
    // get the interpreter...
    m_script = KateGlobal::self()->jscriptManager()->interpreter (m_basename, true);
  }
}

void KateIndentJScript::unloadInterpreter()
{
  if (m_script)
    delete m_script;
  m_script = 0;
}

const QString &KateIndentJScript::triggerCharacters(KateView* view)
{
  // already set, perfect, just return...
  if (m_triggerCharactersSet)
    return m_triggerCharacters;

  loadInterpreter(view);

  m_triggerCharactersSet = true;

  if (!m_script)
    return m_triggerCharacters;

  KJS::ExecState *exec = m_script->interpreter()->globalExec();

  m_triggerCharacters = m_script->interpreter()->globalObject()->get(exec, "triggerCharacters")->toString(exec).qstring();
  if (exec->hadException()) {
    kDebug(13050) << "triggerCharacters: Unable to lookup 'indenter.triggerCharacters'" << endl;
    exec->clearException();
    m_triggerCharacters = QString();
  }

  kDebug () << "trigger chars: '" << m_triggerCharacters << "'" << endl;

  return m_triggerCharacters;
}

int KateIndentJScript::indent(KateView* view,
                              const KTextEditor::Cursor& position,
                              QChar typedChar,
                              int indentWidth)
{
  loadInterpreter(view);

  if (!m_script)
    return -2;

  KJS::List params;
  params.append(KJS::Number(position.line()));
  params.append(KJS::Number(indentWidth));
  params.append(KJS::String(typedChar.isNull() ? QString("") : QString(typedChar)));

  QString errorMsg;
  KJS::JSValue *val = m_script->callFunction(view, m_script->interpreter()->globalObject(), KJS::Identifier("indent"),
                                   params, errorMsg);

  if (!val) {
    kDebug(13050) << "KateIndentJScript::indent: callFunction(): " << errorMsg << endl;
    return -2;
  }

  const int indentLevel = val->toInt32(m_script->interpreter()->globalExec());
  kDebug() << "new indentation: " << indentLevel << endl;
  return indentLevel;
}
//END KateIndentJScript



//BEGIN KateJScriptHelpers
bool KateJScriptHelpers::parseScriptHeader(const QString& url,
                                           KateJScriptHeader& scriptHeader)
{
  // a valid script file -must- have the following format:
  // The first line must contain the string 'kate-script'.
  // All following lines have to have the format 'key : value'. So the value
  // is separated by a colon. Leading non-letter characters are ignored, that
  // include C and C++ comments for example.
  // Parsing the header stops at the first line with no ':'.

  QFile file(QFile::encodeName(url));
  if (!file.open(QIODevice::ReadOnly)) {
    kDebug(13050) << "Script parse error: Cannot open file " << url << endl;
    return false;
  }

  kDebug(13050) << "Update script: " << url << endl;
  QTextStream ts(&file);
  ts.setCodec("UTF-8");
  if (!ts.readLine().contains("kate-script")) {
    kDebug(13050) << "Script parse error: No header found in " << url << endl;
    file.close();
    return false;
  }

  QString line;
  while (!(line = ts.readLine()).isNull()) {
    int colon = line.indexOf(':');
    if (colon <= 0)
      break; // no colon -> end of header found

    // if -1 then 0. if >= 0, move after star.
    int start = 0; // start points to first letter. idea: skip '*' and '//'
    while (start < line.length() && !line.at(start).isLetter())
      ++start;

    QString key = line.mid(start, colon - start).trimmed();
    QString value = line.right(line.length() - (colon + 1)).trimmed();
    scriptHeader.pairs[key] = value;

    kDebug(13050) << "found pair: (" << key << " | " << value << ")" << endl;
  }
  file.close();
  return true;
}

KateJScriptHeaderVector KateJScriptHelpers::findScripts(const QString& rcFile,
                                                        const QString& resourceDir,
                                                        const QStringList &keys)
{
  KConfig cfgFile(rcFile, KConfig::NoGlobals);
  KConfigGroup config = cfgFile.group("General");

  bool force = false;
  // If KatePart version does not match, better force a true reload
  if (QString(KATEPART_VERSION) != config.readEntry ("kate-version", QString("0.0"))) {
    config.writeEntry("kate-version", QString(KATEPART_VERSION));
    force = true;
  }

  // get a list of all .js files (false: not recursive, true: no duplicates)
  QStringList list = KGlobal::dirs()->findAllResources("data", resourceDir, KStandardDirs::NoDuplicates);
  KateJScriptHeaderVector files;
  files.reserve(list.size());

  // iterate through the files and read info out of cache or file
  for (QStringList::ConstIterator fileit = list.begin(); fileit != list.end(); ++fileit)
  {
    // each file has a group
    QString group = "Cache "+ *fileit;
    config.changeGroup(group);

    // stat the file to get the last-modified-time
    struct stat sbuf;
    memset (&sbuf, 0, sizeof(sbuf));
    stat(QFile::encodeName(*fileit), &sbuf);

    // check whether file is already cached
    bool useCache = false;
    if (!force && cfgFile.hasGroup(group)) {
      config.changeGroup(group);
      useCache = (sbuf.st_mtime == config.readEntry("last-modified", 0));
    }

    // read key/value pairs from the cached file if possible
    // otherwise, parse it and then save the needed infos to the cache
    KateJScriptHeader scriptHeader;
    scriptHeader.url = *fileit;
    if (useCache) {
      for (QStringList::ConstIterator keyit = keys.begin(); keyit != keys.end(); ++keyit) {
        QString value = config.readEntry(*keyit, QString("unset"));
        if (value != "unset")
          scriptHeader.pairs[*keyit] = value;
      }
      files.push_back(scriptHeader);
    } else if (parseScriptHeader(*fileit, scriptHeader)) {
      config.changeGroup(group);
      config.writeEntry("last-modified", int(sbuf.st_mtime));
      // iterate keys and save cache
      for (QStringList::ConstIterator keyit = keys.begin(); keyit != keys.end(); ++keyit)
        if (scriptHeader.pairs.contains(*keyit))
          config.writeEntry(*keyit, scriptHeader.pairs[*keyit]);
      files.push_back(scriptHeader);
    }
  }

  config.sync();
  return files;
}
//END KateJScriptHelpers
// kate: space-indent on; indent-width 2; replace-tabs on;
