/* This file is part of the KDE libraries
  Copyright (C) 2010 Joseph Wenninger <jowenn@kde.org>

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

#include "templateinterface.h"
#include "templateinterface2.h"


using namespace KTextEditor;

TemplateScript::~TemplateScript()
{
}




TemplateScriptRegistrar::TemplateScriptRegistrar()
{
}

TemplateScriptRegistrar::~TemplateScriptRegistrar()
{
}

TemplateInterface2::TemplateInterface2()
  : TemplateInterface()
  , d(0)
{
}

TemplateInterface2::~TemplateInterface2()
{
}

bool TemplateInterface2::insertTemplateText ( const Cursor& insertPosition,
                                              const QString &templateString,
                                              const QMap<QString, QString> &initialValues,
                                              TemplateScript* templateScript)
{
  QMap<QString,QString> enhancedInitValues(initialValues);
  if (!KTE_INTERNAL_setupIntialValues(templateString,&enhancedInitValues)) {
    return false;
  }
  return insertTemplateTextImplementation( insertPosition, templateString, enhancedInitValues, templateScript);
}

// kate: space-indent on; indent-width 2; replace-tabs on;
