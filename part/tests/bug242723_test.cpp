/* This file is part of the KDE libraries
   Copyright (C) 2011 Dominik Haumann <dhaumann kde org>

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

#include "bug242723_test.h"
#include "moc_bug242723_test.cpp"

#include <qtest_kde.h>

#include <katedocument.h>
#include <kateconfig.h>
#include <kateview.h>

using namespace KTextEditor;

QTEST_KDEMAIN(RemoveTrailingSpacesTest, GUI)

RemoveTrailingSpacesTest::RemoveTrailingSpacesTest()
  : QObject()
{
}

RemoveTrailingSpacesTest::~RemoveTrailingSpacesTest()
{
}

// remove trailing spaces should be blocked during the paste,
// as paste 1. removes text, 2. then remove-trailing-spaces
// moves content and then 3. insert text works on the modified
// content by 2.
void RemoveTrailingSpacesTest::testCopyPasteBug242723()
{
    KateDocument doc (false, false, false);
    doc.config()->setRemoveTrailingDyn(true);
    
    // one edit action -> revision now 1, last saved still -1
    QString content("blah ( int a,\n"
                    "       int b,\n"
                    "       int c )\n");

    doc.insertText(Cursor(0, 0), content);
    
    QCOMPARE(doc.text(), content);

    KateView* view = static_cast<KateView*>(doc.createView(0));

    view->setSelection(Range(1, 7, 2, 12));

    view->copy();
    view->paste();
    
    QString after = doc.text();

    QCOMPARE(doc.text(), content);
}
