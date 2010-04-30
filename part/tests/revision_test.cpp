/* This file is part of the KDE libraries
   Copyright (C) 2010 Dominik Haumann <dhaumann kde org>

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

#include "revision_test.h"
#include "moc_revision_test.cpp"

#include <qtest_kde.h>

#include <katedocument.h>
#include <katebuffer.h>
#include <katetexthistory.h>
#include <ktexteditor/movingcursor.h>
#include <ktexteditor/movingrange.h>

using namespace KTextEditor;

QTEST_KDEMAIN(RevisionTest, GUI)

namespace QTest {
    template<>
    char *toString(const KTextEditor::Cursor &cursor)
    {
        QByteArray ba = "Cursor[" + QByteArray::number(cursor.line())
                           + ", " + QByteArray::number(cursor.column()) + "]";
        return qstrdup(ba.data());
    }
}


RevisionTest::RevisionTest()
  : QObject()
{
}

RevisionTest::~RevisionTest()
{
}

// tests: MovingInterface
// - lockRevision()
// - revision()
// - unlockRevision()
// - transformCursor()
// - transformRange()
void RevisionTest::testRevision()
{
    KateDocument doc (false, false, false);
    Kate::TextHistory& history = doc.buffer().history();

    // initial revision is -1, as there is no stored revision
    QVERIFY(doc.lastSavedRevision() == -1);
    QCOMPARE(doc.revision(), (qint64) 0);

    // one edit action -> revision now 1, last saved still -1
    doc.insertText(Cursor(0, 0), "0000");

    qint64 rev = doc.revision();
    QCOMPARE(rev, (qint64) 1);

    // now lock current revision 1
    doc.lockRevision(rev);

    // wrapLine + insertText + wrapLine + insertText
    doc.insertText(Cursor(0, 2), "\n1111\n2222");

    // create some cursors, then transform them
    Cursor c01(0, 1);
    Cursor stayOnInsert(0, 2);
    Cursor moveOnInsert(0, 2);
    doc.transformCursor(c01, MovingCursor::MoveOnInsert, rev, -1);
    doc.transformCursor(moveOnInsert, MovingCursor::MoveOnInsert, rev, -1);
    doc.transformCursor(stayOnInsert, MovingCursor::StayOnInsert, rev, -1);

    QCOMPARE(c01, Cursor(0, 1));
    QCOMPARE(stayOnInsert, Cursor(0, 2));
    QCOMPARE(moveOnInsert, Cursor(2, 4));


    // free revision and lock current again
    doc.unlockRevision(rev); // FIXME: cullmann?
    rev = doc.revision();
    doc.lockRevision(rev);

    // now undo, the cursors should move to original positions again
    doc.undo();

    // reset cursors
    doc.transformCursor(c01, MovingCursor::MoveOnInsert, rev, -1);
    doc.transformCursor(moveOnInsert, MovingCursor::MoveOnInsert, rev, -1);
    doc.transformCursor(stayOnInsert, MovingCursor::StayOnInsert, rev, -1);

    QCOMPARE(c01, Cursor(0, 1));
    QCOMPARE(stayOnInsert, Cursor(0, 2));
    QCOMPARE(moveOnInsert, Cursor(0, 2));


/*transformRange(KTextEditor::Range &     range,
               KTextEditor::MovingRange::InsertBehaviors   insertBehaviors,
               MovingRange::EmptyBehavior      emptyBehavior,
               qint64      fromRevision,
               qint64      toRevision = -1
               )*/
}













