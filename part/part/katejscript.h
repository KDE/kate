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

#include <qdict.h>

/**
 * Some common stuff
 */
class KateDocument;
class KateView;
class QString;

/**
 * Cool, this is all we need here
 */
namespace KJS {
  class Object;
  class ObjectImp;
  class Interpreter;
  class ExecState;
}

/**
 * Whole Kate Part scripting in one classs
 */
class KateJScript
{
  public:
    /**
     * generate new global interpreter for part scripting
     */
    KateJScript ();

    /**
     * be destructive
     */
    ~KateJScript ();

    /**
     * creates a JS wrapper object for given KateDocument
     * @param exec execution state, to find out interpreter to use
     * @param doc document object to wrap
     * @return new js wrapper object
     */
    KJS::ObjectImp *wrapDocument (KJS::ExecState *exec, KateDocument *doc);

    /**
     * creates a JS wrapper object for given KateDocument
     * @param exec execution state, to find out interpreter to use
     * @param doc document object to wrap
     * @return new js wrapper object
     */
    KJS::ObjectImp *wrapView (KJS::ExecState *exec, KateView *view);

    /**
     * execute given script
     * the script will get the doc and view exposed via document and view object
     * in global scope
     * @param doc document to expose
     * @param view view to expose
     * @param script source code of script to execute
     * @return success or not?
     */
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

class KateJScriptManager
{
  public:
    class Script
    {
      public:
        QString name;
        QString filename;
    };

  public:
    KateJScriptManager ();
    ~KateJScriptManager ();

  private:
    void collectScripts (bool force = false);

  private:
    QDict<KateJScriptManager::Script> m_scripts;
};

#endif
