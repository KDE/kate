// This file is part of the KDE libraries
// Copyright (C) 2010 Dominik Haumann <dhaumann kde org>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) version 3.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public License
// along with this library; see the file COPYING.LIB.  If not, write to
// the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
// Boston, MA 02110-1301, USA.

#ifndef KATE_SCRIPTHELPERS_H
#define KATE_SCRIPTHELPERS_H

#include <QtScript/QScriptValue>
#include "katepartprivate_export.h"

class QScriptEngine;
class QScriptContext;

namespace Kate {
  /** Top-level script functions */
  namespace Script {

    /** read complete file contents, helper */
    KATEPART_TESTS_EXPORT bool readFile(const QString& sourceUrl, QString& sourceCode);
    
    KATEPART_TESTS_EXPORT QScriptValue read(QScriptContext *context, QScriptEngine *engine);
    KATEPART_TESTS_EXPORT QScriptValue require(QScriptContext *context, QScriptEngine *engine);
    KATEPART_TESTS_EXPORT QScriptValue debug(QScriptContext *context, QScriptEngine *engine);
    KATEPART_TESTS_EXPORT QScriptValue i18n(QScriptContext *context, QScriptEngine *engine);
    KATEPART_TESTS_EXPORT QScriptValue i18nc(QScriptContext *context, QScriptEngine *engine);
    KATEPART_TESTS_EXPORT QScriptValue i18np(QScriptContext *context, QScriptEngine *engine);
    KATEPART_TESTS_EXPORT QScriptValue i18ncp(QScriptContext *context, QScriptEngine *engine);
  }
}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
