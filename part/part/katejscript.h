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

#include "../interfaces/document.h"

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
     * @param view view to expose
     * @param script source code of script to execute
     * @param errorMsg error to return if no success
     * @return success or not?
     */
    bool execute (KateView *view, const QString &script, QString &errorMsg);

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

class KateJScriptManager : public Kate::Command
{
  public:
    class Script
    {
      public:
        QString desktopFilename ()
        {
          return filename.left(filename.length()-2).append ("desktop");
        }

      public:
        /**
         * command name, as used for command line and more
         */
        QString name;

        /**
         * filename of the script
         */
        QString filename;

        /**
         * has it a desktop file?
         */
        bool desktopFileExists;
    };

  public:
    KateJScriptManager ();
    ~KateJScriptManager ();

  private:
    void collectScripts (bool force = false);

  //
  // Here we deal with the Kate::Command stuff
  //
  public:
    /**
     * execute command
     * @param view view to use for execution
     * @param cmd cmd string
     * @param errorMsg error to return if no success
     * @return success
     */
    bool exec( class Kate::View *view, const QString &cmd, QString &errorMsg );

    bool help( class Kate::View *view, const QString &cmd, QString &msg );

    /**
     * supported commands as prefixes
     * @return prefix list
     */
    QStringList cmds();

  private:
    QDict<KateJScriptManager::Script> m_scripts;
};

#endif
