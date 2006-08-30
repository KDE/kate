/* This file is part of the KDE libraries
   Copyright (C) 2005 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2005 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2006 Dominik Haumann <dhdev@gmx.de>

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
#include <kjs/PropertyNameArray.h>

#include <qfile.h>
#include <qfileinfo.h>
#include <qregexp.h>
#include <qtextstream.h>


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
  putDirect(KJS::lengthPropertyName,length,KJS::DontDelete|KJS::ReadOnly|KJS::DontEnum);
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
      IndexOf,              // (line, string)
      IsSpace,              // (line, column)
      StartsWith,           // (line, string, bool), bool: ignore leading spaces
      EndsWith,             // (line, string, bool), bool: ignore trailing spaces
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
      VirtualLineLength,    // (line, tab-width)

      EditBegin,
      EditEnd,

      // helper functions to speed up kjs
      FirstColumn,          // (line)
      FirstVirtualColumn,   // (line, tab-width)
      LastColumn,           // (line)
      LastVirtualColumn,    // (line, tab-width)
      ToVirtualColumn,      // (line, column, tab-width)
      FromVirtualColumn,    // (line, virtual-column, tab-width)
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

      // config settings
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
      EndOfSelection
    };

  public:
    KateView *view;

    static const KJS::ClassInfo info;
};

class KateJSIndenter : public KJS::JSObject
{
  public:
    KateJSIndenter (KJS::ExecState *exec);
    /*
    KJS::Value get( KJS::ExecState *exec, const  KJS::Identifier &propertyName) const;

    KJS::Value getValueProperty(KJS::ExecState *exec, int token) const;

    void put(KJS::ExecState *exec, const KJS::Identifier &propertyName, const KJS::Value& value, int attr = KJS::None);

    void putValueProperty(KJS::ExecState *exec, int token, const KJS::Value& value, int attr);
    */
    const KJS::ClassInfo* classInfo() const { return &info; }

    enum {
      ProcessChar,
      ProcessNewLine,
      ProcessLine,
      ProcessSection,
      ProcessIndent,
      Dummy
    };

  public:

    static const KJS::ClassInfo info;
};

#include "katejscript.lut.h"

//END

KateJScriptInterpreterContext::KateJScriptInterpreterContext ()
 : m_global (new KateJSGlobal ())
 , m_interpreter (new KJS::Interpreter (m_global))
 , m_document (wrapDocument(m_interpreter->globalExec(), 0))
 , m_view (wrapView(m_interpreter->globalExec(), 0))
{
  m_interpreter->ref();
  // put some stuff into env., this should stay for all executions.
  m_interpreter->globalObject()->put(m_interpreter->globalExec(), "document", m_document);
  m_interpreter->globalObject()->put(m_interpreter->globalExec(), "view", m_view);
  m_interpreter->globalObject()->put(m_interpreter->globalExec(), "debug",
        new KateJSGlobalFunctions(KateJSGlobalFunctions::Debug,1));
}

KateJScriptInterpreterContext::~KateJScriptInterpreterContext ()
{
  KJS::Interpreter::collect();
  m_interpreter->deref();
}

KJS::JSObject *KateJScriptInterpreterContext::wrapDocument (KJS::ExecState *exec, KateDocument *doc)
{
  return new KateJSDocument(exec, doc);
}

KJS::JSObject *KateJScriptInterpreterContext::wrapView (KJS::ExecState *exec, KateView *view)
{
  return new KateJSView(exec, view);
}

bool KateJScriptInterpreterContext::execute (KateView *view, const QString &script, QString &errorMsg)
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
    KJS::ExecState *exec = m_interpreter->globalExec();

    KJS::JSValue *exVal = comp.value();

    char *msg = exVal->toString(exec).ascii();

    int lineno = -1;

    if (exVal->type() == KJS::ObjectType)
    {
      KJS::JSValue *lineVal = exVal->getObject()->get(exec,"line");

      if (lineVal->type() == KJS::NumberType)
        lineno = int(lineVal->toNumber(exec));
    }

    errorMsg = i18n("Exception, line %1: %2", lineno, msg);
    return false;
  }

  return true;
}

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
  indexOf              KateJSDocument::IndexOf           DontDelete|Function 2
  isSpace              KateJSDocument::IsSpace           DontDelete|Function 2
  startsWith           KateJSDocument::StartsWith        DontDelete|Function 3
  endsWith             KateJSDocument::EndsWith          DontDelete|Function 3
  matchesAt            KateJSDocument::MatchesAt         DontDelete|Function 3
# FIXME: add
#        search(start-line, start-col, text, bool case-sensitive)
#        searchBackwards(start-line, start-col, text, bool case-sensitive)
#        replaceText(line, column, line, column, string)
#        lastIndexOf(line, string)
#        isBalanced(line, column, line, column, open, close)

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
KJS_IMPLEMENT_PROTOTYPE("KateJSDocument",KateJSDocumentProto,KateJSDocumentProtoFunc)

const KJS::ClassInfo KateJSDocument::info = { "KateJSDocument", 0, 0, 0 };

JSValue* KateJSDocumentProtoFunc::callAsFunction(KJS::ExecState *exec, KJS::JSObject *thisObj, const KJS::List &args)
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
      return KJS::String (doc->text(KTextEditor::Range(args[0]->toUInt32(exec), args[1]->toUInt32(exec), args[2]->toUInt32(exec), args[3]->toUInt32(exec))));

    case KateJSDocument::TextLine:
      if (exception.invalidArgs(1)) break;
      return KJS::String (doc->line (args[0]->toUInt32(exec)));

    case KateJSDocument::WordAt:
      if (exception.invalidArgs(2)) break;
      return KJS::String (doc->getWord(KTextEditor::Cursor(args[0]->toUInt32(exec), args[1]->toUInt32(exec))));

    case KateJSDocument::CharAt: {
      if (exception.invalidArgs(2)) break;
      QChar c = doc->character (KTextEditor::Cursor(args[0]->toUInt32(exec), args[1]->toUInt32(exec)));
      return KJS::String (c.isNull() ? "" : QString(c));
    }

    case KateJSDocument::FirstChar: {
      if (exception.invalidArgs(1)) break;
      KateTextLine::Ptr textLine = doc->plainKateTextLine(args[0]->toUInt32(exec));
      if (!textLine) return KJS::String("");
      // check for isNull(), as the returned character then would be "\0"
      QChar c = textLine->at(textLine->firstChar());
      return KJS::String (c.isNull() ? "" : QString(c));
    }

    case KateJSDocument::LastChar: {
      if (exception.invalidArgs(1)) break;
      KateTextLine::Ptr textLine = doc->plainKateTextLine(args[0]->toUInt32(exec));
      if (!textLine) return KJS::String("");
      // check for isNull(), as the returned character then would be "\0"
      QChar c = textLine->at(textLine->lastChar());
      return KJS::String (c.isNull() ? "" : QString(c));
    }

    case KateJSDocument::IndexOf:
      if (exception.invalidArgs(2)) break;
      return KJS::Number (doc->line(args[0]->toUInt32(exec)).indexOf(QChar((uint)args[1]->toUInt32(exec))));

    case KateJSDocument::IsSpace:
      if (exception.invalidArgs(2)) break;
      return KJS::Boolean (doc->character (KTextEditor::Cursor(args[0]->toUInt32(exec), args[1]->toUInt32(exec))).isSpace());

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
      return KJS::Boolean (textLine ? textLine->matchesAt(args[1]->toUInt32(exec), args[2]->toString(exec).qstring()) : false);
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
      return KJS::Boolean (doc->insertText (KTextEditor::Cursor(args[0]->toUInt32(exec), args[1]->toUInt32(exec)), args[2]->toString(exec).qstring()));

    case KateJSDocument::RemoveText:
      if (exception.invalidArgs(4)) break;
      return KJS::Boolean (doc->removeText(KTextEditor::Range(args[0]->toUInt32(exec), args[1]->toUInt32(exec), args[2]->toUInt32(exec), args[3]->toUInt32(exec))));

    case KateJSDocument::InsertLine:
      if (exception.invalidArgs(2)) break;
      return KJS::Boolean (doc->insertLine (args[0]->toUInt32(exec), args[1]->toString(exec).qstring()));

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
      if (exception.invalidArgs(0)) break;
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
      // use highlighting information, if available (assume, { and } have same attribute)
      bool hasAttribute = (cursor.currentChar() == '}' || cursor.currentChar() == '{');
      // force highlighting of current line
      if (hasAttribute)
        doc->kateTextLine(cursor.line());
      int attribute = cursor.currentAttrib();
      int count = 1;

      // Move backwards char by char and find the opening brace
      while (cursor.moveBackward(1)) {
        if (!hasAttribute || cursor.currentAttrib() == attribute) {
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
      }

      return KJS::Null();
    }

    case KateJSDocument::FindLeftParenthesis: {
      if (exception.invalidArgs(2)) break;
      KateDocCursor cursor(args[0]->toUInt32(exec), args[1]->toUInt32(exec), doc);
      // use highlighting information, if available
      bool hasAttribute = (cursor.currentChar() == ')' || cursor.currentChar() == '(');
      // force highlighting of current line
      if (hasAttribute)
        doc->kateTextLine(cursor.line());
      int attribute = cursor.currentAttrib();
      int count = 1;

      // Move backwards char by char and find the opening parenthesis
      while (cursor.moveBackward(1)) {
        if (!hasAttribute || cursor.currentAttrib() == attribute) {
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
      return KJS::Boolean( doc->highlight()->isInWord( args[0]->toString(exec).qstring().at(0), args[1]->toUInt32(exec) ) );

    case KateJSDocument::CanBreakAt:
      if (exception.invalidArgs(2)) break;
      return KJS::Boolean( doc->highlight()->canBreakAt( args[0]->toString(exec).qstring().at(0), args[1]->toUInt32(exec) ) );

    case KateJSDocument::CanComment:
      if (exception.invalidArgs(2)) break;
      return KJS::Boolean( doc->highlight()->canComment( args[0]->toUInt32(exec), args[1]->toUInt32(exec) ) );

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

bool KateJSDocument::getOwnPropertySlot( KJS::ExecState *exec, const  KJS::Identifier &propertyName, KJS::PropertySlot &slot)
{
  return KJS::getStaticValueSlot<KateJSDocument,KJS::JSObject>(exec, &KateJSDocumentTable, this, propertyName, slot );
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

void KateJSDocument::put(KJS::ExecState *exec, const KJS::Identifier &propertyName, KJS::JSValue *value, int attr)
{
  KJS::lookupPut<KateJSDocument,KJS::JSObject>(exec, propertyName, value, attr, &KateJSDocumentTable, this );
}

void KateJSDocument::putValueProperty(KJS::ExecState *exec, int token, KJS::JSValue* value, int attr)
{
  if (!doc)
    return;
}

KateJSDocument::KateJSDocument (KJS::ExecState *exec, KateDocument *_doc)
    : KJS::JSObject (KateJSDocumentProto::self(exec))
    , doc (_doc)
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

JSValue* KateJSViewProtoFunc::callAsFunction(KJS::ExecState *exec, KJS::JSObject *thisObj, const KJS::List &args)
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
      return KJS::Boolean( view->setCursorPosition( KTextEditor::Cursor (args[0]->toUInt32(exec), args[1]->toUInt32(exec)) ) );

    case KateJSView::VirtualCursorPosition: {
      if (exception.invalidArgs(0)) break;
      KJS::JSObject *object = exec->lexicalInterpreter()->builtinObject()->construct(exec, KJS::List::empty());
      object->put(exec, "line", KJS::Number(view->cursorPosition().line()));
      object->put(exec, "column", KJS::Number(view->cursorPositionVirtual().column()));
      return object;
    }

    case KateJSView::SetVirtualCursorPosition:
      if (exception.invalidArgs(0)) break;
      return KJS::Boolean( view->setCursorPositionVisual( KTextEditor::Cursor (args[0]->toUInt32(exec), args[1]->toUInt32(exec)) ) );

    case KateJSView::Selection:
      if (exception.invalidArgs(0)) break;
      return KJS::String( view->selectionText() );

    case KateJSView::HasSelection:
      if (exception.invalidArgs(0)) break;
      return KJS::Boolean( view->selection() );

    case KateJSView::SetSelection:
      if (exception.invalidArgs(4)) break;
      return KJS::Boolean( view->setSelection(KTextEditor::Range(args[0]->toInt32(exec), args[1]->toInt32(exec), args[2]->toUInt32(exec), args[3]->toUInt32(exec))) );

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
  if (!view)
    return;
}

//END

//BEGIN KateJScriptManager

KateJScriptManager::KateJScriptManager ():KTextEditor::Command(),m_jscript(0)
{
  collectScripts ();
}

KateJScriptManager::~KateJScriptManager ()
{
  delete m_jscript;
  qDeleteAll(m_scripts);
}

void KateJScriptManager::collectScripts (bool force)
{
// If there's something in myModeList the Mode List was already built so, don't do it again
  if (!m_scripts.isEmpty())
    return;

  // We'll store the scripts list in this config
  KConfig config("katepartjscriptrc", false, false);

  // figure out if the kate install is too new
  config.setGroup ("General");
  if (config.readEntry ("Version", 0) > config.readEntry ("CachedVersion",0))
  {
    config.writeEntry ("CachedVersion", config.readEntry ("Version",0));
    force = true;
  }

  // Let's get a list of all the .js files
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

    // If the group exist and we're not forced to read the .js file, let's build myModeList for katepartjscriptrc
    if (!force && config.hasGroup(Group) && (sbuf.st_mtime == config.readEntry("lastModified",0)))
    {
    }
    else
    {
      kDebug (13050) << "add script: " << *it << endl;

      QString desktopFile =  (*it).left((*it).length()-2).append ("desktop");

      kDebug (13050) << "add script (desktop file): " << desktopFile << endl;

      QFileInfo dfi (desktopFile);

      if (dfi.exists())
      {
        KConfig df (desktopFile, true, false);
        df.setDesktopGroup ();

        // get cmdname, fallback to baseName, if it is empty, therefor not use the kconfig fallback
        QString cmdname = df.readEntry ("X-Kate-Command");
        if (cmdname.isEmpty())
        {
          QFileInfo fi (*it);
          cmdname = fi.baseName();
        }

        if (m_scripts[cmdname])
          continue;

        KateJScriptManager::Script *s = new KateJScriptManager::Script ();

        s->command = cmdname;
        s->name = df.readEntry ("Name");
        s->description = df.readEntry ("Comment");
        s->filename = *it;
        s->desktopFileExists = true;

        kDebug() << s->name << ":: " << s->description << endl;

        m_scripts.insert (s->command, s);
      }
      else // no desktop file around, fall back to scriptfilename == commandname
      {
        kDebug (13050) << "add script: fallback, no desktop file around!" << endl;

        QFileInfo fi (*it);

        if (m_scripts[fi.baseName()])
          continue;

        KateJScriptManager::Script *s = new KateJScriptManager::Script ();

        s->command = fi.baseName();
        s->filename = *it;
        s->desktopFileExists = false;
        s->name = i18n("Unnamed");
        s->description = i18n("No description available.");

        m_scripts.insert (s->command, s);
      }
    }
  }

  // Syncronize with the file katepartjscriptrc
  config.sync();
}

bool KateJScriptManager::exec( KTextEditor::View *view, const QString &_cmd, QString &errorMsg )
{
  // cast it hardcore, we know that it is really a kateview :)
  KateView *v = (KateView*) view;

  if ( !v )
  {
    errorMsg = i18n("Could not access view");
    return false;
  }

   //create a list of args
  QStringList args( _cmd.split( QRegExp("\\s+") ,QString::SkipEmptyParts) );
  QString cmd ( args.first() );
  args.removeFirst();

  kDebug(13050) << "try to exec: " << cmd << endl;

  QString source;
  if (cmd==QString("js-run-myself")) {
    source=v->doc()->text();
  } else {
    if (!m_scripts[cmd])
    {
      errorMsg = i18n("Command not found");
      return false;
    }

    QFile file (m_scripts[cmd]->filename);

    if ( !file.open( QIODevice::ReadOnly ) )
      {
      errorMsg = i18n("JavaScript file not found");
      return false;
    }

    QTextStream stream( &file );
    //stream.setEncoding (QTextStream::UnicodeUTF8);
    stream.setCodec ("UTF-8");

    source = stream.readAll ();

    file.close();
  }
  if (!m_jscript) m_jscript=new KateJScriptInterpreterContext();
  return m_jscript->execute(v, source, errorMsg);
}

bool KateJScriptManager::help( KTextEditor::View *, const QString &cmd, QString &msg )
{
  if (cmd=="js-run-myself") {msg=i18n("This executes the current document as a javascript within kate"); return true;}
  if (!m_scripts[cmd] || !m_scripts[cmd]->desktopFileExists)
    return false;

  KConfig df (m_scripts[cmd]->desktopFilename(), true, false);
  df.setDesktopGroup ();

  msg = df.readEntry ("X-Kate-Help");

  if (msg.isEmpty())
    return false;

  return true;
}

const QStringList &KateJScriptManager::cmds()
{
  static QStringList l;

  l.clear();
  l << "js-run-myself";
  foreach (KateJScriptManager::Script* script, m_scripts)
    l << script->command;

   return l;
}

QString KateJScriptManager::name (const QString& cmd) const
{
  return m_scripts.contains( cmd ) ? m_scripts[cmd]->name : QString();
}

QString KateJScriptManager::description (const QString& cmd) const
{
  return m_scripts.contains( cmd ) ? m_scripts[cmd]->description : QString();
}

QString KateJScriptManager::category (const QString& cmd) const
{
  return i18n("Scripts");
}

//END




//BEGIN KateJSIndenter

// -------------------------------------------------------------------------
/* Source for KateJSIndenterProtoTable.
@begin KateJSIndenterProtoTable 1
  Dummy                 KateJSIndenter::Dummy             DontDelete
@end
*/

/* Source for KateJSIndenterTable.
@begin KateJSIndenterTable 5
  processChar           KateJSIndenter::ProcessChar       DontDelete
  processNewLine        KateJSIndenter::ProcessNewLine    DontDelete
  processLine           KateJSIndenter::ProcessLine       DontDelete
  processSection        KateJSIndenter::ProcessSection    DontDelete
  processIndent         KateJSIndenter::ProcessIndent     DontDelete
@end
*/

KateJSIndenter::KateJSIndenter (KJS::ExecState *exec)
    : KJS::JSObject (KateJSViewProto::self(exec))
{
}

KJS_DEFINE_PROTOTYPE(KateJSIndenterProto)
KJS_IMPLEMENT_PROTOFUNC(KateJSIndenterProtoFunc)
KJS_IMPLEMENT_PROTOTYPE("KateJSIndenter", KateJSIndenterProto, KateJSIndenterProtoFunc)

const KJS::ClassInfo KateJSIndenter::info = { "KateJSIndenter", 0, &KateJSIndenterTable, 0 };

JSValue* KateJSIndenterProtoFunc::callAsFunction(KJS::ExecState *exec, KJS::JSObject *thisObj, const KJS::List &args)
{
  KJS_CHECK_THIS( KateJSIndenter, thisObj );

  return KJS::Undefined();
}

//END

//BEGIN KateIndentJScript
KateIndentJScript::KateIndentJScript(KateIndentJScriptManager *manager,
    const QString& internalName, const QString  &filePath, const QString &niceName,
    const QString &license, const QString &author, int version, double kateVersion)
  : m_manager(manager),
    m_internalName (internalName),
    m_filePath (filePath),
    m_niceName(niceName),
    m_license(license),
    m_author(author),
    m_version(version),
    m_kateVersion(kateVersion),
    m_indenter(0),
    m_interpreter(0)
{
}


KateIndentJScript::~KateIndentJScript()
{
  deleteInterpreter();
}

void KateIndentJScript::deleteInterpreter()
{
    m_docWrapper=0;
    m_viewWrapper=0;
    delete m_indenter;
    m_indenter=0;
    if (m_interpreter) {
      m_interpreter->deref();
      m_interpreter=0;
    }
}

bool KateIndentJScript::setupInterpreter(QString &errorMsg)
{
  if (m_interpreter)
    return true;

  kDebug(13050)<<"Setting up interpreter"<<endl;
  m_interpreter = new KJS::Interpreter(new KateJSGlobal());
  KJS::ExecState *exec = m_interpreter->globalExec();
  m_docWrapper = new KateJSDocument(exec, 0);
  m_viewWrapper = new KateJSView(exec, 0);
  m_indenter = new KateJSIndenter(exec);
  m_interpreter->globalObject()->put(exec, "document", m_docWrapper, KJS::DontDelete | KJS::ReadOnly);
  m_interpreter->globalObject()->put(exec, "view", m_viewWrapper, KJS::DontDelete | KJS::ReadOnly);
  m_interpreter->globalObject()->put(exec, "debug", new KateJSGlobalFunctions(KateJSGlobalFunctions::Debug, 1));
  m_interpreter->globalObject()->put(exec, "indenter", m_indenter, KJS::DontDelete | KJS::ReadOnly);
  QFile file (filePath());

  if ( !file.open( QIODevice::ReadOnly ) )
  {
    errorMsg = i18n("File not found: %1", filePath());
    deleteInterpreter();
    return false;
  }

  QTextStream stream( &file );
  stream.setCodec("UTF-8");
  QString source = stream.readAll ();
  file.close();

  KJS::Completion comp (m_interpreter->evaluate("", 0, source));

  if (comp.complType() == KJS::Throw)
  {
    KJS::ExecState *exec = exec;
    KJS::JSValue *exVal = comp.value();
    char *msg = exVal->toString(exec).ascii();

    int lineno = -1;
    if (exVal->type() == KJS::ObjectType)
    {
      KJS::JSValue *lineVal = exVal->getObject()->get(exec,"line");

      if (lineVal->type() == KJS::NumberType)
        lineno = int(lineVal->toNumber(exec));
    }

    errorMsg = i18n("Exception, line %1: %2", lineno, msg);
    //deleteInterpreter();
    return false;
  }

  return true;
}

inline static bool kateIndentJScriptCall(KateView *view, QString &errorMsg, KateJSDocument *docWrapper, KateJSView *viewWrapper,
        KJS::Interpreter *interpreter, KJS::JSObject *lookupobj,const KJS::Identifier& func,KJS::List params)
{
 // no view, no fun
  if (!view)
  {
    errorMsg = i18n("Could not access view");
    return false;
  }

  KJS::ExecState *exec = interpreter->globalExec();

  // lookup function
  KJS::JSObject *o = lookupobj->get(exec, func)->toObject(exec);
  if (exec->hadException())
  {
    KJS::JSObject *error = exec->exception()->toObject(exec);
    QString message = error->get(exec, "message")->toString(exec).qstring();
    errorMsg = i18n("Exception: Unable to find function '%1': %2", func.qstring(), message);
    exec->clearException();
    return false;
  }

  // init doc & view with new pointers!
  docWrapper->doc = view->doc();
  viewWrapper->view = view;

  o->call(exec, interpreter->globalObject(), params);
  if (exec->hadException())
  {
    KJS::JSObject *error = exec->exception()->toObject(exec);
    QString message = error->get(exec, "message")->toString(exec).qstring();

    int lineno = -1;
    KJS::JSValue *lineVal = error->get(exec, "line");
    if (lineVal->type() == KJS::NumberType)
      lineno = int(lineVal->toNumber(exec));

    errorMsg = i18n("Exception in line %1: %2", lineno, message);
    exec->clearException();
    return false;
  }
  return true;
}

bool KateIndentJScript::processChar(KateView *view, QChar c, QString &errorMsg )
{

  kDebug(13050) << "KateIndentJScript::processChar" << endl;
  if (!setupInterpreter(errorMsg)) return false;
  KJS::List params;
  params.append(KJS::String(QString(c)));
  return kateIndentJScriptCall(view, errorMsg,
                               m_docWrapper, m_viewWrapper,
                               m_interpreter, m_indenter,
                               KJS::Identifier("processChar"),
                               params);
}

bool KateIndentJScript::processLine(KateView *view, const KateDocCursor &line, QString &errorMsg )
{
  kDebug(13050) << "KateIndentJScript::processLine" << endl;
  if (!setupInterpreter(errorMsg)) return false;
  KJS::List params;
  params.append(KJS::Number(line.line()));
  return kateIndentJScriptCall(view, errorMsg,
                               m_docWrapper, m_viewWrapper,
                               m_interpreter, m_indenter,
                               KJS::Identifier("processLine"),
                               params);
}

bool KateIndentJScript::processNewline(KateView *view, const KateDocCursor &begin,
                                       bool needcontinue, QString &errorMsg )
{
  kDebug(13050) << "KateIndentJScript::processNewline" << endl;
  if (!setupInterpreter(errorMsg)) return false;
  KJS::List params;
  params.append(KJS::Number(begin.line()));
  params.append(KJS::Number(begin.column()));
  return kateIndentJScriptCall(view, errorMsg,
                               m_docWrapper, m_viewWrapper,
                               m_interpreter, m_indenter,
                               KJS::Identifier("processNewLine"),
                               params);
}

bool KateIndentJScript::processSection( KateView *view, const KateDocCursor &begin,
                                        const KateDocCursor &end, QString &errorMsg )
{
  kDebug(13050) << "KateIndentJScript::processSection" << endl;
  if (!setupInterpreter(errorMsg)) return false;
  KJS::List params;
  params.append(KJS::Number(begin.line()));
  params.append(KJS::Number(end.line()));
  return kateIndentJScriptCall(view, errorMsg,
                               m_docWrapper, m_viewWrapper,
                               m_interpreter, m_indenter,
                               KJS::Identifier("processSection"),
                               params);
}

bool KateIndentJScript::processIndent( KateView *view, uint line, int levels, QString &errorMsg )
{
  kDebug(13050) << "KateIndentJScript::processIndent" << endl;
  if (!setupInterpreter(errorMsg)) return false;
  KJS::List params;
  params.append(KJS::Number(line));
  params.append(KJS::Number(levels));
  return kateIndentJScriptCall(view, errorMsg,
                               m_docWrapper, m_viewWrapper,
                               m_interpreter, m_indenter,
                               KJS::Identifier("processIndent"),
                               params);
}

const QString& KateIndentJScript::internalName()
{
   return m_internalName;
}

const QString& KateIndentJScript::filePath()
{
   return m_filePath;
}

const QString& KateIndentJScript::niceName()
{
  return m_niceName;
}

const QString& KateIndentJScript::license()
{
  return m_license;
}

const QString& KateIndentJScript::author()
{
  return m_author;
}

int KateIndentJScript::version()
{
  return m_version;
}

double KateIndentJScript::kateVersion()
{
  return m_kateVersion;
}
//END

//BEGIN KateIndentJScriptManager
KateIndentJScriptManager::KateIndentJScriptManager()
{
  collectScripts ();
}

KateIndentJScriptManager::~KateIndentJScriptManager ()
{
  qDeleteAll(m_scripts);
}

void KateIndentJScriptManager::collectScripts (bool force)
{
// If there's something in myModeList the Mode List was already built so, don't do it again
  if (!m_scripts.isEmpty())
    return;


  // We'll store the scripts list in this config
  KConfig config("katepartindentjscriptrc", false, false);
#if 0
  // figure out if the kate install is too new
  config.setGroup ("General");
  if (config.readEntry ("Version",0) > config.readEntry ("CachedVersion",0))
  {
    config.writeEntry ("CachedVersion", config.readEntry ("Version",0));
    force = true;
  }
#endif

  // Let's get a list of all the .js files
  QStringList list = KGlobal::dirs()->findAllResources("data","katepart/scripts/indent/*.js",false,true);

  // Let's iterate through the list and build the Mode List
  for ( QStringList::Iterator it = list.begin(); it != list.end(); ++it )
  {
    // Each file has a group ed:
    QString Group="Cache "+ *it;

    // Let's go to this group
    config.setGroup(Group);

    // stat the file
    struct stat sbuf;
    memset (&sbuf, 0, sizeof(sbuf));
    stat(QFile::encodeName(*it), &sbuf);

    // If the group exist and we're not forced to read the .js file, let's
    // build myModeList for katepartjscriptrc
    bool readnew=false;
    if (!force && config.hasGroup(Group))
    {
      config.setGroup(Group);
      if (sbuf.st_mtime == config.readEntry("lastModified", 0)) {
        QString filePath = *it;
        QString internalName = config.readEntry("internalName", "KATE-ERROR");
        if (internalName == "KATE-ERROR") readnew = true;
        else
        {
          QString niceName = config.readEntry("niceName", internalName);
          QString license = config.readEntry("license", i18n("(Unknown)"));
          QString author = config.readEntry("author", i18n("(Unknown)"));
          int version = config.readEntry("version", 0);
          double kateVersion = config.readEntry("kateVersion", 0.0);
          KateIndentJScript *s = new KateIndentJScript(this, internalName,
              filePath, niceName, license, author, version, kateVersion);
          m_scripts.insert (internalName, s);
          m_scriptsList.append(s);
        }
      } else readnew=true;
    }
    else readnew=true;
    if (readnew)
    {
      QFileInfo fi (*it);

      if (m_scripts[fi.baseName()])
        continue;

      QString internalName = fi.baseName();
      QString filePath = *it;
      QString niceName = internalName;
      QString license;
      QString author;
      int version = 0;
      double kateVersion = 0.0;
      bool success = parseScriptHeader(filePath, niceName, license, author,
                                       version, kateVersion);
      // save the information for retrieval
      config.setGroup(Group);
      config.writeEntry("lastModified", int(sbuf.st_mtime));
      config.writeEntry("internalName", internalName);
      config.writeEntry("niceName", niceName);
      config.writeEntry("license", license);
      config.writeEntry("author", author);
      config.writeEntry("version", version);
      config.writeEntry("kateVersion", kateVersion);

      if (success) {
        KateIndentJScript *s = new KateIndentJScript(this,
          internalName, filePath, niceName, license, author, version, kateVersion);
        m_scripts.insert(internalName, s);
        m_scriptsList.append(s);
      }
    }
  }

  // Syncronize with the file katepartjscriptrc
  config.sync();
}

bool KateIndentJScriptManager::parseScriptHeader(const QString &filePath,
    QString &niceName, QString &license, QString &author, int &version, double &kateversion)
{
  QFile f(QFile::encodeName(filePath));
  if (!f.open(QIODevice::ReadOnly) ) {
    kDebug(13050)<<"Header could not be parsed, because file could not be opened"<<endl;
    return false;
  }
  QTextStream st(&f);
  st.setCodec("UTF-8");
  if (!st.readLine().toLower().startsWith("/** kate-js-indenter")) {
    kDebug(13050) << "No header found" << endl;
    f.close();
    return false;
  }
  // here the real parsing begins
  QString line;
  QRegExp endExpr("^\\s*\\*[/]?\\s*$");
  QRegExp keyValue("^\\s*\\*\\s*(.+):(.*)$");

  while (!(line = st.readLine()).isNull()) {
    if (endExpr.exactMatch(line)) {
      kDebug(13050) << "end of config block" << endl;
      break;
    }
    if (keyValue.exactMatch(line)) {
      QStringList sl = keyValue.capturedTexts();
      kDebug(13050) << "key:" << sl[1] << endl << "value:" << sl[2] << endl;

      QString key = sl[1].trimmed().toLower();
      QString value = sl[2].trimmed();
      if (key == "name") niceName = value;
      else if (key == "license") license = value;
      else if (key == "author") author = value;
      else if (key == "kateversion") kateversion = value.toDouble(0);
      else if (key == "version") version = value.toInt();
    }
  }
  f.close();
  return true;
}
//END
// kate: space-indent on; indent-width 2; replace-tabs on;
