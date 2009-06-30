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

    KTextEditor::Editor* editor = KateGlobal::self();
    QVERIFY(editor);
    m_doc = qobject_cast<KateDocument*>(editor->createDocument(this));
    QVERIFY(m_doc);

    m_view = qobject_cast<KateView*>(m_doc->createView(0));
    QApplication::setActiveWindow(m_view);
    QVERIFY(m_view);

//     m_view->show();
}

void UndoManagerTest::cleanup()
{
    delete m_view;
    delete m_doc;
}

void UndoManagerTest::testSimpleUndo()
{
    m_doc->setText("aaaa bbbb cccc\n"
                   "dddd  ffff");
    m_view->setCursorPosition(KTextEditor::Cursor(1, 5));

    m_doc->typeChars(m_view, "eeee");

    // cursor position: "dddd eeee| ffff"
    QCOMPARE(m_view->cursorPosition(), KTextEditor::Cursor(1, 9));

    // undo once to remove "eeee", cursor position: "dddd | ffff"
    m_doc->undo();
    QCOMPARE(m_view->cursorPosition(), KTextEditor::Cursor(1, 5));

    // redo once to insert "eeee" again. cursor position: "dddd eeee| ffff"
    m_doc->redo();
    QCOMPARE(m_view->cursorPosition(), KTextEditor::Cursor(1, 9));
}

void UndoManagerTest::testSelectionUndo()
{
    m_doc->setText("aaaa bbbb cccc\n"
    "dddd eeee ffff");
    m_view->setCursorPosition(KTextEditor::Cursor(1, 9));
    KTextEditor::Range selectionRange(KTextEditor::Cursor(0, 5),
                                      KTextEditor::Cursor(1, 9));
    m_view->setSelection(selectionRange);

    m_doc->typeChars(m_view, "eeee");

    // cursor position: "aaaa eeee| ffff", no selection anymore
    QCOMPARE(m_view->cursorPosition(), KTextEditor::Cursor(0, 9));
    QCOMPARE(m_view->selection(), false);

    // undo to remove "eeee" and add selection and text again
    m_doc->undo();
    QCOMPARE(m_view->cursorPosition(), KTextEditor::Cursor(1, 9));
    QCOMPARE(m_view->selection(), true);
    QCOMPARE(m_view->selectionRange(), selectionRange);

    // redo to insert "eeee" again and remove selection
    // cursor position: "aaaa eeee| ffff", no selection anymore
    m_doc->redo();
    QCOMPARE(m_view->cursorPosition(), KTextEditor::Cursor(0, 9));
    QCOMPARE(m_view->selection(), false);
}
