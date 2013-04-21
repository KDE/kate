/*  This file is part of the Kate project.
 *
 *  Copyright (C) 2013 Dominik Haumann <dhaumann@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "messagetest.h"

#include <qtest_kde.h>

#include <kateglobal.h>
#include <katedocument.h>
#include <kateview.h>
#include <messageinterface.h>
#include <katemessagewidget.h>

using namespace KTextEditor;

QTEST_KDEMAIN(MessageTest, GUI)

void MessageTest::initTestCase()
{
    KateGlobal::self()->incRef();
}

void MessageTest::cleanupTestCase()
{
    KateGlobal::self()->decRef();
}

void MessageTest::testPostMessage()
{
    KateDocument doc(false, false, false);

    KateView* view = static_cast<KateView*>(doc.createView(0));
    view->show();
    view->resize(400, 300);

    QPointer<Message> message = new Message(Message::Information, "Message text");
    message->setPosition(Message::TopInView);

    // posing message should succeed
    QVERIFY(doc.postMessage(message));

    //
    // show message for one second, then delete again
    //
    QTest::qWait(500);
    QVERIFY(view->messageWidget());
    QVERIFY(view->messageWidget()->isVisible());

    QVERIFY(message != 0);
    delete message;
    QTest::qWait(600); // fadeout animation takes 500 ms
    QVERIFY(!view->messageWidget()->isVisible());
}

void MessageTest::testAutoHide()
{
    KateDocument doc(false, false, false);

    KateView* view = static_cast<KateView*>(doc.createView(0));
    view->show();
    view->resize(400, 300);
    QTest::qWait(500); // make sure the widget is really shown in correct size

    //
    // show a message with autoHide. Check, if it's deleted correctly
    // auto hide mode: Message::Immediate
    //
    QPointer<Message> message = new Message(Message::Information, "Message text");
    message->setPosition(Message::TopInView);
    message->setAutoHide(2000);
    message->setAutoHideMode(Message::Immediate);

    doc.postMessage(message);

    QTest::qWait(500);
    QVERIFY(view->messageWidget()->isVisible());

    // should be deleted after 2.1 seconds
    QTest::qWait(1600);
    QVERIFY(message.data() == 0);

    // message widget should be hidden after 2.6 seconds
    QTest::qWait(500);
    QVERIFY(!view->messageWidget()->isVisible());
}

void MessageTest::testAutoHideAfterUserInteraction()
{
    KateDocument doc(false, false, false);

    KateView* view = static_cast<KateView*>(doc.createView(0));
    view->show();
    view->resize(400, 300);
    QTest::qWait(500); // make sure the widget is really shown in correct size

    //
    // show a message with autoHide. Check, if it's deleted correctly
    // auto hide mode: Message::AfterUserInteraction
    //
    QPointer<Message> message = new Message(Message::Information, "Message text");
    message->setPosition(Message::TopInView);
    message->setAutoHide(2000);
    QVERIFY(message->autoHideMode() == Message::AfterUserInteraction);

    doc.postMessage(message);

    QTest::qWait(1000);
    QVERIFY(view->messageWidget()->isVisible());

    // now trigger user interaction after 1 second
    view->resize(400, 310);

    // should still be there after deleted after another 1.9 seconds
    QTest::qWait(1900);
    QVERIFY(message.data() != 0);
    QVERIFY(view->messageWidget()->isVisible());

    // another 200ms later: 3.1 seconds are gone, message should be deleted
    // and fade animation should be active
    QTest::qWait(200);
    QVERIFY(message.data() == 0);
    QVERIFY(view->messageWidget()->isVisible());

    // after a total of 3.6 seconds, widget should be hidden
    QTest::qWait(500);
    QVERIFY(!view->messageWidget()->isVisible());
}

void MessageTest::testMessageQueue()
{
    KateDocument doc(false, false, false);

    KateView* view = static_cast<KateView*>(doc.createView(0));
    view->show();
    view->resize(400, 300);
    QTest::qWait(500); // make sure the widget is really shown in correct size

    //
    // add two messages, both with autoHide to 1 second, and check that the queue is processed correctly
    // auto hide mode: Message::Immediate
    //
    QPointer<Message> m1 = new Message(Message::Information, "Info text");
    m1->setPosition(Message::TopInView);
    m1->setAutoHide(1000);
    m1->setAutoHideMode(Message::Immediate);

    QPointer<Message> m2 = new Message(Message::Error, "Error text");
    m2->setPosition(Message::TopInView);
    m2->setAutoHide(1000);
    m2->setAutoHideMode(Message::Immediate);

    // post both messages
    QVERIFY(doc.postMessage(m1));
    QVERIFY(doc.postMessage(m2));

    // after 0.5s, first message should be visible, (timer of m1 triggered)
    QTest::qWait(500);
    QVERIFY(view->messageWidget()->isVisible());
    QVERIFY(m1.data() != 0);
    QVERIFY(m2.data() != 0);

    // after 1.1s, first message is deleted, and hide animation is active
    QTest::qWait(600);
    QVERIFY(view->messageWidget()->isVisible());
    QVERIFY(m1.data() == 0);
    QVERIFY(m2.data() != 0);

    // timer of m2 triggered after 1.5s, i.e. after hide animation if finished
    QTest::qWait(500);

    // after 2.1s, second message should be visible
    QTest::qWait(500);
    QVERIFY(view->messageWidget()->isVisible());
    QVERIFY(m2.data() != 0);

    // after 2.6s, second message is deleted, and hide animation is active
    QTest::qWait(500);
    QVERIFY(view->messageWidget()->isVisible());
    QVERIFY(m2.data() == 0);

    // after a total of 3.1s, animation is finished and widget is hidden
    QTest::qWait(500);
    QVERIFY(!view->messageWidget()->isVisible());
}

void MessageTest::testSimplePriority()
{
    KateDocument doc(false, false, false);

    KateView* view = static_cast<KateView*>(doc.createView(0));
    view->show();
    view->resize(400, 300);
    QTest::qWait(500); // make sure the widget is really shown in correct size

    //
    // add two messages
    // - m1: no auto hide timer, priority 0
    // - m2: auto hide timer of 1 second, priority 1
    // test: m1 should be hidden in favour of m2
    //
    QPointer<Message> m1 = new Message(Message::Positive, "m1");
    m1->setPosition(Message::TopInView);
    QVERIFY(m1->priority() == 0);

    QPointer<Message> m2 = new Message(Message::Error, "m2");
    m2->setPosition(Message::TopInView);
    m2->setAutoHide(1000);
    m2->setAutoHideMode(Message::Immediate);
    m2->setPriority(1);
    QVERIFY(m2->priority() == 1);

    // post m1
    QVERIFY(doc.postMessage(m1));

    // after 1s, message should be displayed
    QTest::qWait(1000);
    QVERIFY(view->messageWidget()->isVisible());
    QCOMPARE(view->messageWidget()->text(), QString("m1"));
    QVERIFY(m1.data() != 0);

    // post m2, m1 should be hidden, and m2 visible
    QVERIFY(doc.postMessage(m2));
    QVERIFY(m2.data() != 0);

    // after 0.6 seconds, m2 is visible
    QTest::qWait(600);
    QCOMPARE(view->messageWidget()->text(), QString("m2"));
    QVERIFY(m2.data() != 0);

    // after 1.6 seconds, m2 is hidden again and m1 is visible again
    QTest::qWait(1000);
    QVERIFY(view->messageWidget()->isVisible());
    QVERIFY(m1.data() != 0);
    QVERIFY(m2.data() == 0);

    // finally check m1 agagin
    QTest::qWait(1000);
    QCOMPARE(view->messageWidget()->text(), QString("m1"));
}
// kate: indent-width 4; remove-trailing-spaces all;
