/* This file is part of the KDE libraries
   Copyright (C) 2005 Joseph Wenninger <jowenn@kde.org>

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

#include "config.h"
#ifdef HAVE_LUA

#include "kateluascript.h"
#include "katedocument.h"
#include "kateview.h"
#include "katehighlight.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <qfile.h>
#include <qfileinfo.h>
#include <kstandarddirs.h>

#include <kconfig.h>
#include <kglobal.h>
#include <klocale.h>

extern "C" {
#include <lua.h>
#include <lualib.h>
}

#define ONCHAR 1
#define ONNEWLINE 2
#define ONCHARSTR "kateonchar"
#define ONNEWLINESTR "kateonnewline"

#define katelua_registerFunc(n,f,t) \
        (lua_pushstring(m_interpreter, n), \
         lua_pushcfunction(m_interpreter, f), \
         lua_settable(m_interpreter, t))

#define katelua_registerNumConst(n,v,t) \
        (lua_pushstring(m_interpreter, n), \
         lua_pushnumber(m_interpreter, v), \
         lua_settable(m_interpreter, t))

//BEGIN temporary, try to use registry later
static KateDocument *katelua_doc;
static KateView *katelua_view;
//END



//BEGIN STATIC BINDING FUNCTIONS
typedef struct KATELUA_FUNCTIONS {
  char *name;
  lua_CFunction func;
} KATELUA_FUNCTIONS;

static int katelua_katedebug(lua_State *L) {
  int n=lua_gettop(L);
  for (int i=1;i<=n;i++) {
    if (lua_isnil(L,i)) kdDebug(13060)<<"NIL VALUE"<<endl;
    else if (lua_isstring(L,i)) kdDebug(13060)<<lua_tostring(L,i)<<endl;
    else if (lua_isboolean(L,i)) kdDebug(13060)<<(bool)lua_toboolean(L,i)<<endl;
    else if (lua_isnumber(L,i)) kdDebug(13060)<<lua_tonumber(L,i)<<endl;
    else kdDebug(13060)<<"Invalid type for katedebug:"<<lua_type(L,i)<<endl;
  }
  return 0;
}

static int katelua_indenter_register(lua_State *L) {
  int n=lua_gettop(L);
  if (n!=2) {
    lua_pushstring(L,i18n("indenter.register requires 2 parameters (event id, function to call)").toUtf8().data());
    lua_error(L);
  }
  if ( (!lua_isfunction(L,2)) || (!lua_isnumber(L,1)))
  {
    /*if (lua_isnumber(L,1)) kdDebug(13060)<<"A"<<endl;
    if (lua_isfunction(L,2)) kdDebug(13060)<<"B"<<endl;
    kdDebug(13060)<<lua_type(L,2)<<endl;*/
    lua_pushstring(L,i18n("indenter.register requires 2 parameters (event id (number), function to call (function))").toUtf8().data());
    lua_error(L);
  }
  switch ((int)lua_tonumber(L,1))
  {
    case ONCHAR:
      lua_pushstring(L,ONCHARSTR);
      lua_pushstring(L,ONCHARSTR);
      break;
    case ONNEWLINE:
      lua_pushstring(L,ONNEWLINESTR);
      lua_pushstring(L,ONNEWLINESTR);
      break;
    default:
      lua_pushstring(L,i18n("indenter.register:invalid event id").toUtf8().data());
      lua_error(L);
  }
  lua_gettable(L,LUA_REGISTRYINDEX);
  if (!lua_isnil(L,lua_gettop(L))) {
      lua_pushstring(L,i18n("indenter.register:there is already a function set for given").toUtf8().data());
      lua_error(L);
  }
  lua_pop(L,1);
  lua_pushvalue(L,2);
  lua_settable(L,LUA_REGISTRYINDEX);
  kdDebug(13060)<<"katelua_indenter_register: Success"<<endl;
  return 0;
}

static int katelua_document_attribute(lua_State *L) {
  if (lua_gettop(L)!=2) {
      lua_pushstring(L,i18n("document.attribute:Two parameters needed (line, column)").toUtf8().data());
      lua_error(L);
  }
  if ((!lua_isnumber(L,1)) || (!lua_isnumber(L,2)))  {
      lua_pushstring(L,i18n("document.attribute:Two parameters needed (line, column) (2x number)").toUtf8().data());
      lua_error(L);
  }
  lua_pushnumber(L,katelua_doc->kateTextLine(lua_tonumber(L,1))->attribute(lua_tonumber(L,2)));
  return 1;
}

static int katelua_document_canBreakAt(lua_State *L) {
  if (lua_gettop(L)!=2) {
      lua_pushstring(L,i18n("document.canBreakAt:Two parameters needed (char, attribute)").toUtf8().data());
      lua_error(L);
  }
  if ((!lua_isstring(L,1)) || (!lua_isnumber(L,2)))  {
      lua_pushstring(L,i18n("document.canBreakAt:Two parameters needed (char, attribute) (string (one character), number").toUtf8().data());
      lua_error(L);
  }
  QString str(QString::fromUtf8(lua_tostring(L,1)));
  if (str.isEmpty())
    lua_pushboolean(L,true);
  else
    lua_pushboolean(L,katelua_doc->highlight()->canBreakAt(str.at(0),lua_tonumber(L,2)));
  return 1;
}

static int katelua_document_canComment(lua_State *L) {
  if (lua_gettop(L)!=2) {
      lua_pushstring(L,i18n("document.canComment:Two parameters needed (start_attribute, end_attribute)").toUtf8().data());
      lua_error(L);
  }
  if ((!lua_isstring(L,1)) || (!lua_isnumber(L,2)))  {
      lua_pushstring(L,i18n("document.canComment:Two parameters needed (start_attribute, end_attribute) (2xnumber)").toUtf8().data());
      lua_error(L);
  }
    lua_pushboolean(L,katelua_doc->highlight()->canComment(lua_tonumber(L,1),lua_tonumber(L,2)));
  return 1;
}

static int katelua_document_clear(lua_State *L) {
  if (lua_gettop(L)!=0) {
      lua_pushstring(L,i18n("document.clear:No parameters needed)").toUtf8().data());
      lua_error(L);
  }
  katelua_doc->clear();
  lua_pushboolean(L,true);
  return 1;
}

static int katelua_document_commentStart(lua_State *L) {
  if (lua_gettop(L)!=1) {
      lua_pushstring(L,i18n("document.commentStart:One parameter needed (attribute)").toUtf8().data());
      lua_error(L);
  }
  if (!lua_isstring(L,1))  {
      lua_pushstring(L,i18n("document.commentStart:One parameters needed (attribute) (number)").toUtf8().data());
      lua_error(L);
  }
  lua_pushstring(L,katelua_doc->highlight()->getCommentStart(lua_tonumber(L,1)).toUtf8().data());
  return 1;
}

static int katelua_document_commentEnd(lua_State *L) {
  if (lua_gettop(L)!=1) {
      lua_pushstring(L,i18n("document.commentEnd:One parameter needed (attribute)").toUtf8().data());
      lua_error(L);
  }
  if (!lua_isstring(L,1))  {
      lua_pushstring(L,i18n("document.commentEnd:One parameters needed (attribute) (number)").toUtf8().data());
      lua_error(L);
  }
  lua_pushstring(L,katelua_doc->highlight()->getCommentEnd(lua_tonumber(L,1)).toUtf8().data());
  return 1;
}

static int katelua_document_editBegin(lua_State *L) {
  if (lua_gettop(L)!=0) {
      lua_pushstring(L,i18n("document.editBegin:No parameters needed)").toUtf8().data());
      lua_error(L);
  }
  katelua_doc->editBegin();
  lua_pushboolean(L,true);
  return 1;
}

static int katelua_document_editEnd(lua_State *L) {
  if (lua_gettop(L)!=0) {
      lua_pushstring(L,i18n("document.editEnd:No parameters needed)").toUtf8().data());
      lua_error(L);
  }
  katelua_doc->editEnd();
  lua_pushboolean(L,true);
  return 1;
}


static int katelua_document_textline(lua_State *L) {
  if (lua_gettop(L)!=1) {
      lua_pushstring(L,i18n("document.line:One parameter (line number) required").toUtf8().data());
      lua_error(L);
  }
  if (!lua_isnumber(L,1)) {
      lua_pushstring(L,i18n("document.line:One parameter (line number) required (number)").toUtf8().data());
      lua_error(L);
  }
  lua_pushstring(L,katelua_doc->line(lua_tonumber(L,1)).toUtf8().data());
  return 1;
}

static int katelua_document_removeText(lua_State *L) {
  if (lua_gettop(L)!=4) {
      lua_pushstring(L,i18n("document.removeText:Four parameters needed (start line, start col,end line, end col)").toUtf8().data());
      lua_error(L);
  }
  if ((!lua_isnumber(L,1)) || (!lua_isnumber(L,2))  ||(!lua_isnumber(L,3)) || (!lua_isnumber(L,4)))  {
      lua_pushstring(L,i18n("document.removeText:Four parameters needed (start line, start col,end line, end col) (4x number)").toUtf8().data());
      lua_error(L);
  }
  lua_pushboolean(L,katelua_doc->removeText(KTextEditor::Range(lua_tonumber(L,1),lua_tonumber(L,2),lua_tonumber(L,3),lua_tonumber(L,4))));
  return 1;
}

static int katelua_document_insertText(lua_State *L) {
  if (lua_gettop(L)!=3) {
      lua_pushstring(L,i18n("document.insertText:Three parameters needed (line,col,text)").toUtf8().data());
      lua_error(L);
  }
  if ((!lua_isnumber(L,1)) || (!lua_isnumber(L,2))  ||(!lua_isstring(L,3)) )  {
      lua_pushstring(L,i18n("document.removeText:Three parameters needed (line,col,text) (number,number,string)").toUtf8().data());
      lua_error(L);
  }
  lua_pushboolean(L,katelua_doc->insertText(KTextEditor::Cursor(lua_tonumber(L,1),lua_tonumber(L,2)),QString::fromUtf8(lua_tostring(L,3))));
  return 1;
}

static int katelua_view_cursorline(lua_State *L) {
  lua_pushnumber(L,katelua_view->cursorPosition().line());
  return 1;
}
static int katelua_view_cursorcolumn(lua_State *L) {
  lua_pushnumber(L,katelua_view->cursorPosition().column());
  return 1;
}
static int katelua_view_cursorposition(lua_State *L) {
  lua_pushnumber(L,katelua_view->cursorPosition().line());
  lua_pushnumber(L,katelua_view->cursorPosition().column());
  return 2;

}
static int katelua_view_setcursorpositionreal(lua_State *L) {
  return 0;
}

static const struct KATELUA_FUNCTIONS katelua_documenttable[12]= {
{"attribute",katelua_document_attribute},
{"canBreakAt",katelua_document_canBreakAt},
{"canComment",katelua_document_canComment},
{"clear",katelua_document_clear},
{"commentStart",katelua_document_commentStart},
{"commentEnd",katelua_document_commentEnd},
{"editBegin",katelua_document_editBegin},
{"editEnd",katelua_document_editEnd},
{"textLine",katelua_document_textline},
{"removeText",katelua_document_removeText},
{"insertText",katelua_document_insertText},
{0,0}
};

static const struct KATELUA_FUNCTIONS katelua_viewtable[5]= {
{"cursorLine",katelua_view_cursorline},
{"cursorColumn",katelua_view_cursorcolumn},
{"cursorPosition",katelua_view_cursorposition},
{"setCursorPositionReal",katelua_view_setcursorpositionreal},
{0,0}
};

static void  kateregistertable(lua_State* m_interpreter,const KATELUA_FUNCTIONS funcs[],char * tablename) {
  lua_newtable(m_interpreter);
  int table=lua_gettop(m_interpreter);
  for (uint i=0;funcs[i].name!=0;i++)
  {
    katelua_registerFunc(funcs[i].name,funcs[i].func,table);
  }

  lua_pushstring(m_interpreter,tablename);
  lua_pushvalue(m_interpreter,table);
  lua_settable(m_interpreter,LUA_GLOBALSINDEX);
  lua_pop(m_interpreter,1);

}

//END STATIC BINDING FUNCTIONS


//BEGIN KateLUAIndentScriptImpl
KateLUAIndentScriptImpl::KateLUAIndentScriptImpl(KateIndentScriptManagerAbstract *manager, const QString& internalName,
        const QString  &filePath, const QString &niceName,
        const QString &license, bool hasCopyright, double version):
          KateIndentScriptImplAbstract(manager,internalName,filePath,niceName,license,hasCopyright,version),m_interpreter(0)/*,m_indenter(0)*/
{
}


KateLUAIndentScriptImpl::~KateLUAIndentScriptImpl()
{
  deleteInterpreter();
}

void KateLUAIndentScriptImpl::decRef()
{
  KateIndentScriptImplAbstract::decRef();
  if (refCount()==0)
  {
    deleteInterpreter();
  }
}

void KateLUAIndentScriptImpl::deleteInterpreter()
{
  if (m_interpreter)
  {
    lua_close(m_interpreter);
    m_interpreter=0;
  }
}

bool KateLUAIndentScriptImpl::setupInterpreter(QString &errorMsg)
{
  if (m_interpreter) return true;
  m_interpreter=lua_open();

  if (!m_interpreter)
  {
    errorMsg=i18n("LUA interpreter could not be initialized");
    return false;
  }
  luaopen_base(m_interpreter);
  luaopen_string( m_interpreter );
  luaopen_table( m_interpreter );
  luaopen_math( m_interpreter );
  luaopen_io( m_interpreter );
  luaopen_debug( m_interpreter );


  /*indenter callback setup table*/
  lua_newtable(m_interpreter);
  int indentertable=lua_gettop(m_interpreter);
  katelua_registerFunc("register",katelua_indenter_register,indentertable);
  katelua_registerNumConst("OnChar",ONCHAR,indentertable);
  katelua_registerNumConst("OnNewline",ONNEWLINE,indentertable);
  lua_pushstring(m_interpreter,"indenter");
  lua_pushvalue(m_interpreter,indentertable);
  lua_settable(m_interpreter,LUA_GLOBALSINDEX);
  lua_pop(m_interpreter,1);

  /*debug*/
  katelua_registerFunc("katedebug",katelua_katedebug,LUA_GLOBALSINDEX);

  /*document interface*/
  kateregistertable(m_interpreter,katelua_documenttable,"document");
  /*view interface*/
  kateregistertable(m_interpreter,katelua_viewtable,"view");

  /*open script*/
  lua_pushstring(m_interpreter,"dofile");
  lua_gettable(m_interpreter,LUA_GLOBALSINDEX);
  QByteArray fn=QFile::encodeName(filePath());
  lua_pushstring(m_interpreter,fn.data());
  int execresult=lua_pcall(m_interpreter,1,1,0);
  if (execresult==0) {
    kdDebug(13060)<<"Lua script has been loaded successfully. Lua interpreter version:"<<lua_version()<<endl;
    return true;
  } else {
    errorMsg=i18n("Lua indenting script had errors: %1").arg(lua_tostring(m_interpreter,lua_gettop(m_interpreter)));
    kdDebug(13060)<<errorMsg<<endl;
    deleteInterpreter();

    return false;
  }
}


bool KateLUAIndentScriptImpl::processChar(KateView *view, QChar c, QString &errorMsg )
{
  if (!setupInterpreter(errorMsg)) return false;
  katelua_doc=((KateView*)view)->doc();
  katelua_view=view;
  int oldtop=lua_gettop(m_interpreter);
  lua_pushstring(m_interpreter,ONCHARSTR);
  lua_gettable(m_interpreter,LUA_REGISTRYINDEX);
  bool result=true;
  if (!lua_isnil(m_interpreter,lua_gettop(m_interpreter)))
  {
    lua_pushstring(m_interpreter,QString(c).toUtf8().data());
    if (lua_pcall(m_interpreter,1,0,0)!=0)
    {
      errorMsg=i18n("Lua indenting script had errors: %1").arg(lua_tostring(m_interpreter,lua_gettop(m_interpreter)));
      kdDebug(13060)<<errorMsg<<endl;
      result=false;
    }
  }
  lua_settop(m_interpreter,oldtop);
  return result;
}

bool KateLUAIndentScriptImpl::processLine(KateView *view, const KateDocCursor &line, QString &errorMsg )
{
  if (!setupInterpreter(errorMsg)) return false;
  return true;
}

bool KateLUAIndentScriptImpl::processNewline( KateView *view, const KateDocCursor &begin, bool needcontinue, QString &errorMsg )
{
  if (!setupInterpreter(errorMsg)) return false;
  katelua_doc=((KateView*)view)->doc();
  katelua_view=view;
  int oldtop=lua_gettop(m_interpreter);
  lua_pushstring(m_interpreter,ONNEWLINESTR);
  lua_gettable(m_interpreter,LUA_REGISTRYINDEX);
  bool result=true;
  if (!lua_isnil(m_interpreter,lua_gettop(m_interpreter)))
  {
    if (lua_pcall(m_interpreter,0,0,0)!=0)
    {
      errorMsg=i18n("Lua indenting script had errors: %1").arg(lua_tostring(m_interpreter,lua_gettop(m_interpreter)));
      kdDebug(13060)<<errorMsg<<endl;
      result=false;
    }
  }
  lua_settop(m_interpreter,oldtop);
  return result;
}
//END

//BEGIN KateLUAIndentScriptManager
KateLUAIndentScriptManager::KateLUAIndentScriptManager():KateIndentScriptManagerAbstract()
{
  collectScripts();
}

KateLUAIndentScriptManager::~KateLUAIndentScriptManager ()
{
  qDeleteAll(m_scripts);
}

void KateLUAIndentScriptManager::collectScripts (bool force)
{
// If there's something in myModeList the Mode List was already built so, don't do it again
  if (!m_scripts.isEmpty())
    return;

  kdDebug(13060)<<"================================================="<<endl<<"Trying to find Lua scripts"<<endl
      <<"================================================="<<endl;

  // We'll store the scripts list in this config
  KConfig config("katepartluaindentscriptrc", false, false);
#if 0
  // figure out if the kate install is too new
  config.setGroup ("General");
  if (config.readEntry ("Version") > config.readNumEntry ("CachedVersion", QVariant(0)).toInt())
  {
    config.writeEntry ("CachedVersion", config.readEntry ("Version", QVariant(0)).toInt());
    force = true;
  }
#endif

  // Let's get a list of all the .js files
  QStringList list = KGlobal::dirs()->findAllResources("data","katepart/scripts/indent/*.lua",false,true);

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
    kdDebug(13060)<<"Lua script file:"<<(*it)<<endl;
    // If the group exist and we're not forced to read the .js file, let's build myModeList for katepartjscriptrc
    bool readnew=false;
    if (!force && config.hasGroup(Group) &&
        (sbuf.st_mtime == qvariant_cast<int>(config.readEntry("lastModified",0))))
    {
        config.setGroup(Group);
        QString filePath=*it;
        QString internalName=config.readEntry("internlName","KATE-ERROR");
        if (internalName=="KATE-ERROR") readnew=true;
        else
        {
          QString niceName=config.readEntry("niceName",internalName);
          QString copyright=config.readEntry("copyright",i18n("(Unknown)"));
          double  version=qvariant_cast<double>(config.readEntry("version",0.0));
          QString license=config.readEntry("license",i18n("(Unknown)"));
          bool hasCopyright=config.readEntry("hasCopyright",false);
          KateLUAIndentScriptImpl *s=new KateLUAIndentScriptImpl(this,
            internalName,filePath,niceName,license,hasCopyright,version);
          m_scripts.insert (internalName, s);
        }
    }
    else readnew=true;
    if (readnew)
    {
        QFileInfo fi (*it);

        if (m_scripts[fi.baseName()])
          continue;

        QString internalName=fi.baseName();
        QString filePath=*it;
        QString niceName=internalName;
        QString license=i18n("(Unknown)");
	QString copyright;
        bool hasCopyright=false;
        double   version=0.0;
        parseScriptHeader(filePath,&niceName,&copyright,&version);
        /*save the information for retrieval*/
        config.setGroup(Group);
        config.writeEntry("lastModified",int(sbuf.st_mtime));
        config.writeEntry("internalName",internalName);
        config.writeEntry("niceName",niceName);
        config.writeEntry("license",license);
        config.writeEntry("hasCopyright",hasCopyright);
        config.writeEntry("copyright",copyright);
        config.writeEntry("version",version);
        KateLUAIndentScriptImpl *s=new KateLUAIndentScriptImpl(this,
          internalName,filePath,niceName,license,hasCopyright,version);
        m_scripts.insert (internalName, s);
    }
  }

  // Syncronize with the file katepartjscriptrc
  config.sync();
}

KateIndentScript KateLUAIndentScriptManager::script(const QString &scriptname) {
  KateLUAIndentScriptImpl *s=m_scripts[scriptname];
  kdDebug(13050)<<scriptname<<"=="<<s<<endl;
  return KateIndentScript(s);
}

void KateLUAIndentScriptManager::parseScriptHeader(const QString &filePath,
        QString *niceName,QString *copyright,double *version)
{
#if 0
  QFile f(QFile::encodeName(filePath));
  if (!f.open(QIODevice::ReadOnly) ) {
    kdDebug(13050)<<"Header could not be parsed, because file could not be opened"<<endl;
    return;
  }
  QTextStream st(&f);
  st.setEncoding (QTextStream::UnicodeUTF8);
  if (!st.readLine().toUpper().startsWith("/**KATE")) {
    kdDebug(13050)<<"No header found"<<endl;
    f.close();
    return;
  }
  // here the real parsing begins
  kdDebug(13050)<<"Parsing indent script header"<<endl;
  enum {NOTHING=0,COPYRIGHT=1} currentState=NOTHING;
  QString line;
  QString tmpblockdata="";
  QRegExp endExpr("[\\s\\t]*\\*\\*\\/[\\s\\t]*$");
  QRegExp keyValue("[\\s\\t]*\\*\\s*(.+):(.*)$");
  QRegExp blockContent("[\\s\\t]*\\*(.*)$");
  while (!(line=st.readLine()).isNull()) {
    if (endExpr.exactMatch(line)) {
      kdDebug(13050)<<"end of config block"<<endl;
      if (currentState==NOTHING) break;
      if (currentState==COPYRIGHT) {
        *copyright=tmpblockdata;
        break;
      }
      Q_ASSERT(0);
    }
    if (currentState==NOTHING)
    {
      if (keyValue.exactMatch(line)) {
        QStringList sl=keyValue.capturedTexts();
        kdDebug(13050)<<"key:"<<sl[1]<<endl<<"value:"<<sl[2]<<endl;
        kdDebug(13050)<<"key-length:"<<sl[1].length()<<endl<<"value-length:"<<sl[2].length()<<endl;
        QString key=sl[1];
        QString value=sl[2];
        if (key=="NAME") (*niceName)=value.trimmed();
        else if (key=="VERSION") (*version)=value.trimmed().toDouble(0);
        else if (key=="COPYRIGHT")
        {
          tmpblockdata="";
          if (value.trimmed().length()>0)  tmpblockdata=value;
          currentState=COPYRIGHT;
        } else kdDebug(13050)<<"ignoring key"<<endl;
      }
    } else {
      if (blockContent.exactMatch(line))
      {
        QString  bl=blockContent.capturedTexts()[1];
        //kdDebug(13050)<<"block content line:"<<bl<<endl<<bl.length()<<" "<<bl.isEmpty()<<endl;
        if (bl.isEmpty())
        {
          (*copyright)=tmpblockdata;
          kdDebug(13050)<<"Copyright block:"<<endl<<(*copyright)<<endl;
          currentState=NOTHING;
        } else tmpblockdata=tmpblockdata+"\n"+bl;
      }
    }
  }
  f.close();
#endif
}
//END

#endif
// kate: space-indent on; indent-width 2; replace-tabs on;
