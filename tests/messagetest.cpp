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
#include <ktexteditor/messageinterface.h>
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

    QPointer<Message> message = new Message("Message text", Message::Information);
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
    QPointer<Message> message = new Message("Message text", Message::Information);
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
    QPointer<Message> message = new Message("Message text", Message::Information);
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
    QPointer<Message> m1 = new Message("Info text", Message::Information);
    m1->setPosition(Message::TopInView);
    m1->setAutoHide(1000);
    m1->setAutoHideMode(Message::Immediate);

    QPointer<Message> m2 = new Message("Error text", Message::Error);
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

    // after 1.2s, first message is deleted, and hide animation is active
    QTest::qWait(700);
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

void MessageTest::testPriority()
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
    // test:
    // - m1 should be hidden in favour of m2
    // - changing text of m1 while m2 is displayed should not change the displayed text
    //
    QPointer<Message> m1 = new Message("m1", Message::Positive);
    m1->setPosition(Message::TopInView);
    QVERIFY(m1->priority() == 0);

    QPointer<Message> m2 = new Message("m2", Message::Error);
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

    // alter text of m1 when m2 is visible, shouldn't influence m2
    QTest::qWait(600);
    m1->setText("m1 changed");

    // after 0.7 seconds, m2 is visible
    QTest::qWait(100);
    QCOMPARE(view->messageWidget()->text(), QString("m2"));
    QVERIFY(m2.data() != 0);

    // after 1.6 seconds, m2 is hidden again and m1 is visible again
    QTest::qWait(900);
    QVERIFY(view->messageWidget()->isVisible());
    QVERIFY(m1.data() != 0);
    QVERIFY(m2.data() == 0);

    // finally check m1 agagin
    QTest::qWait(1000);
    QCOMPARE(view->messageWidget()->text(), QString("m1 changed"));
}

void MessageTest::testCreateView()
{
    KateDocument doc(false, false, false);

    //
    // - first post a message
    // - then create two views
    //
    // test:
    // - verify that both views get the message
    // - verify that, once the message is deleted, both views hide the message
    //
    QPointer<Message> m1 = new Message("message", Message::Positive);
    m1->setPosition(Message::TopInView);
    QVERIFY(m1->priority() == 0);

    // first post message to doc without views
    QVERIFY(doc.postMessage(m1));

    // now create views
    KateView* v1 = static_cast<KateView*>(doc.createView(0));
    KateView* v2 = static_cast<KateView*>(doc.createView(0));
    v1->show();
    v2->show();
    v1->resize(400, 300);
    v2->resize(400, 300);

    // make sure both views show the message
    QTest::qWait(500);
    QVERIFY(v1->messageWidget()->isVisible());
    QVERIFY(v2->messageWidget()->isVisible());
    QCOMPARE(v1->messageWidget()->text(), QString("message"));
    QCOMPARE(v2->messageWidget()->text(), QString("message"));
    QVERIFY(m1.data() != 0);

    // delete message, then check after fadeout time 0f 0.5s whether message is gone
    delete m1;
    QTest::qWait(600);
    QVERIFY(!v1->messageWidget()->isVisible());
    QVERIFY(!v2->messageWidget()->isVisible());
}

void MessageTest::testHideView()
{
    KateDocument doc(false, false, false);

    KateView* view = static_cast<KateView*>(doc.createView(0));
    view->show();
    view->resize(400, 300);

    // create message that hides after 2s immediately
    QPointer<Message> message = new Message("Message text", Message::Information);
    message->setAutoHide(2000);
    message->setAutoHideMode(Message::Immediate);
    message->setPosition(Message::TopInView);

    // posting message should succeed
    QVERIFY(doc.postMessage(message));

    //
    // test:
    // - show the message for 1.5s, then hide the view
    // - the auto hide timer will continue, no matter what
    // - showing the view again after the auto hide timer is finished + animation time really hide the widget
    //
    QTest::qWait(1100);
    QVERIFY(view->messageWidget()->isVisible());
    QCOMPARE(view->messageWidget()->text(), QString("Message text"));

    // hide view
    view->hide();

    // wait 1s, message should be null (after total of 2100 ms)
    QTest::qWait(1000);
    QVERIFY(message.data() == 0);

    // show view again, message contents should be fading for the lasting 400 ms
    view->show();
    QVERIFY(view->messageWidget()->isVisible());
    QCOMPARE(view->messageWidget()->text(), QString("Message text"));

    // wait another 0.5s, then message widget should be hidden
    QTest::qWait(500);
    QVERIFY(message.data() == 0);
    QVERIFY(!view->messageWidget()->isVisible());
}

void MessageTest::testHideViewAfterUserInteraction()
{
    KateDocument doc(false, false, false);

    KateView* view = static_cast<KateView*>(doc.createView(0));
    view->show();
    view->resize(400, 300);
    QTest::qWait(100);

    // create message that hides after 2s immediately
    QPointer<Message> message = new Message("Message text", Message::Information);
    message->setAutoHide(2000);
    QVERIFY(message->autoHideMode() == Message::AfterUserInteraction);
    message->setPosition(Message::TopInView);

    // posting message should succeed
    QVERIFY(doc.postMessage(message));

    //
    // test:
    // - show the message for 1.5s, then hide the view
    // - this should stop the autoHide timer
    // - showing the view again should restart the autoHide timer (again 2s)
    //
    QTest::qWait(1500);
    QVERIFY(view->messageWidget()->isVisible());
    QCOMPARE(view->messageWidget()->text(), QString("Message text"));

    // hide view
    view->hide();

    // wait 1s, check that message is still valid
    QTest::qWait(1000);
    QVERIFY(message.data() != 0);

    // show view again, and trigger user interaction through resize
    view->show();
    QTest::qWait(500);
    view->resize(400, 310);

    // wait 1.5s and check that message is still displayed
    QTest::qWait(1500);
    QVERIFY(message.data() != 0);
    QVERIFY(view->messageWidget()->isVisible());
    QCOMPARE(view->messageWidget()->text(), QString("Message text"));

    // wait another 0.6s, then the message is deleted
    QTest::qWait(600);
    QVERIFY(message.data() == 0);
    QVERIFY(view->messageWidget()->isVisible());

    // another 0.5s, and the message widget should be hidden
    QTest::qWait(600);
    QVERIFY(!view->messageWidget()->isVisible());
}

// kate: indent-width 4; remove-trailing-spaces all;
