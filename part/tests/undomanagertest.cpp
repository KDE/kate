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

#include "tests/undomanagertest.h"

#include "moc_undomanagertest.cpp"

#include <qtest_kde.h>
#include <ksycoca.h>

#include <ktexteditor/editorchooser.h>
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

UndoManagerTest::UndoManagerTest()
    : QObject()
    , m_doc(0)
    , m_view(0)
{
}

UndoManagerTest::~UndoManagerTest()
{
}

void UndoManagerTest::init()
{
    if ( !KSycoca::isAvailable() )
        QSKIP( "ksycoca not available", SkipAll );

    KateGlobal::self();

    KTextEditor::Editor* editor = KTextEditor::EditorChooser::editor();
    QVERIFY(editor);
    editor->createDocument(this);
    m_doc = KateGlobal::self()->kateDocuments().last();
    QVERIFY(m_doc);

    KTextEditor::View *v = m_doc->createView(0);
    QApplication::setActiveWindow(v);
    m_view = m_doc->activeKateView();
    QVERIFY(m_view);

    m_view->show();
}

void UndoManagerTest::cleanup()
{
//    delete m_view;
//    delete m_doc;
}

void UndoManagerTest::testSimpleUndo()
{
    m_doc->clear();
    m_doc->typeChars(m_view, "the first line");
    m_view->keyReturn();
    m_doc->typeChars(m_view, "the second");
    
    QCOMPARE(m_view->cursorPosition(), KTextEditor::Cursor(1, 10));
}
