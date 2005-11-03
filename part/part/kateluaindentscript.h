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

#ifndef _KATELUAINDENTSCRIPT_H_
#define _KATELUAINDENTSCRIPT_H_

#include "kateindentscriptabstracts.h"

struct lua_State;

class KateLUAIndentScriptImpl: public KateIndentScriptImplAbstract {
  public:
    KateLUAIndentScriptImpl(const QString& internalName,
        const QString  &filePath, const QString &niceName,
        const QString &copyright, double version);
    ~KateLUAIndentScriptImpl();

    virtual bool processChar( class KateView *view, QChar c, QString &errorMsg );
    virtual bool processLine( class KateView *view, const KateDocCursor &line, QString &errorMsg );
    virtual bool processNewline( class KateView *view, const KateDocCursor &begin, bool needcontinue, QString &errorMsg );
  protected:
    virtual void decRef();
  private:
    bool setupInterpreter(QString &errorMsg);
    void deleteInterpreter();
    struct lua_State *m_interpreter;
};

class KateLUAIndentScriptManager: public KateIndentScriptManagerAbstract
{

  public:
    KateLUAIndentScriptManager ();
    virtual ~KateLUAIndentScriptManager ();
    virtual KateIndentScript script(const QString &scriptname);
  private:
    /**
     * go, search our scripts
     * @param force force cache updating?
     */
    void collectScripts (bool force = false);
    void parseScriptHeader(const QString &filePath,
        QString *niceName,QString *copyright,double *version);
    QHash<QString, KateLUAIndentScriptImpl*> m_scripts;
};

#endif

#endif
// kate: space-indent on; indent-width 2; replace-tabs on;
