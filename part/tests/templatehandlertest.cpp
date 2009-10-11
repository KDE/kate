/* This file is part of the KDE libraries
   Copyright (C) 2009 Dominik Haumann <dhaumann kde org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "tests/templatehandlertest.h"
#include "moc_templatehandlertest.cpp"

#include <qtest_kde.h>
#include <ksycoca.h>

#include <ktexteditor/editor.h>
#include <ktexteditor/document.h>

#include <katedocument.h>
#include <kateview.h>
#include <katecompletionwidget.h>
#include <katecompletionmodel.h>
#include <katerenderer.h>
#include <kateconfig.h>
#include <kateglobal.h>

#include <katesmartrange.h>

TemplateHandlerTest::TemplateHandlerTest()
    : QObject()
    , m_doc(0)
    , m_view(0)
{
}

TemplateHandlerTest::~TemplateHandlerTest()
{
}

void TemplateHandlerTest::init()
{
    if ( !KSycoca::isAvailable() )
        QSKIP( "ksycoca not available", SkipAll );

    KTextEditor::Editor* editor = KateGlobal::self();
    QVERIFY(editor);
    m_doc = qobject_cast<KateDocument*>(editor->createDocument(this));
    QVERIFY(m_doc);

    m_view = qobject_cast<KateView*>(m_doc->createView(0));
    QApplication::setActiveWindow(m_view);
    QVERIFY(m_view);

//     m_view->show();
}

void TemplateHandlerTest::cleanup()
{
    delete m_view;
    delete m_doc;
}

void TemplateHandlerTest::testTemplateCode()
{
  //qobject_cast<KTextEditor::TemplateInterface*>(activeView())->insertTemplateText(activeView()->cursorPosition(),"for ${index} \\${NOPLACEHOLDER} ${index} ${blah} ${fullname} \\$${Placeholder} \\${${PLACEHOLDER2}}\n next line:${ANOTHERPLACEHOLDER} $${DOLLARBEFOREPLACEHOLDER} {NOTHING} {\n${cursor}\n}",QMap<QString,QString>());
  m_view->insertTemplateText(m_view->cursorPosition(),"for ${index} \\${NOPLACEHOLDER} ${index} ${blah} \\$${Placeholder} \\${${PLACEHOLDER2}}\n next line:${ANOTHERPLACEHOLDER} $${DOLLARBEFOREPLACEHOLDER} {NOTHING} {\n${cursor}\n}",QMap<QString,QString>());
}

// kate: space-indent on; indent-width 2; replace-tabs on;
