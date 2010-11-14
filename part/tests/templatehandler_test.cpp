/* This file is part of the KDE libraries
   Copyright (C) 2010 Bernhard Beschow <bbeschow@cs.tu-berlin.de>

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

#include "templatehandler_test.h"
#include "moc_templatehandler_test.cpp"

#include <qtest_kde.h>

#include <katedocument.h>
#include <kateview.h>

QTEST_KDEMAIN(TemplateHandlerTest, GUI)

using namespace KTextEditor;


void TemplateHandlerTest::testUndo()
{
    const QString snippet = "for (${type} ${index} = ; ${index} < ; ++${index})\n"
                            "{\n"
                            "\t${index}\n"
                            "}";
    QMap<QString, QString> initialValues;
    initialValues.insert("type", "int");
    initialValues.insert("index", "i");

    KateDocument doc(false, false, false, 0, this);
    KateView view(&doc, 0);

    doc.insertTemplateTextImplementation(Cursor(0, 0), snippet, initialValues, 0, &view);

    const QString result = "for (int i = ; i < ; ++i)\n"
                           "{\n"
                           "\ti\n"
                           "}";
    QCOMPARE(doc.text(), result);

    doc.replaceText(Range(0, 9, 0, 10), "j");

    const QString result2 = "for (int j = ; j < ; ++j)\n"
                            "{\n"
                            "\tj\n"
                            "}";
    QCOMPARE(doc.text(), result2);

    doc.undo();

    QCOMPARE(doc.text(), result);

    doc.redo();

    QCOMPARE(doc.text(), result2);

    doc.insertText(Cursor(0, 10), "j");
    doc.insertText(Cursor(0, 11), "j");

    const QString result3 = "for (int jjj = ; jjj < ; ++jjj)\n"
                            "{\n"
                            "\tjjj\n"
                            "}";
    QCOMPARE(doc.text(), result3);

    doc.undo();
    doc.undo();

    QCOMPARE(doc.text(), result2);

    doc.redo();
    doc.redo();

    QCOMPARE(doc.text(), result3);

    doc.undo();
    doc.undo();
    doc.undo();

    QCOMPARE(doc.text(), result);
}
