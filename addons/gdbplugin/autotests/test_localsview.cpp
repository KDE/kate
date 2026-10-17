/*
    SPDX-FileCopyrightText: 2026 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "dap/entities.h"
#include "localsview.h"

#include <QSignalSpy>
#include <QTest>
#include <QTreeWidget>

class LocalsViewTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testStructsAreExpandable();
    void testCollapsedScopeIsRefreshedAfterStep();
    void testVariableWithoutMembersAnymore();
    void testClearForgetsVariables();
};

namespace
{
dap::Variable var(const QString &name, const QString &value, int reference = 0)
{
    return dap::Variable(name, value, reference);
}

QList<int> requestedReferences(QSignalSpy &spy)
{
    QList<int> refs;
    for (const auto &args : std::as_const(spy)) {
        refs << args.at(0).toInt();
    }
    spy.clear();
    return refs;
}

// a view with an expanded "Locals" scope (reference 1) and the given variables in it
struct Fixture {
    Fixture(const QList<dap::Variable> &variables)
    {
        tree = view.findChild<QTreeWidget *>();
        view.insertScopes({dap::Scope(1, QStringLiteral("Locals"))});
        locals()->setExpanded(true);
        view.addVariables(1, variables);
        spy.clear();
    }

    QTreeWidgetItem *locals() const
    {
        return tree->topLevelItem(0);
    }

    LocalsView view;
    QTreeWidget *tree = nullptr;
    QSignalSpy spy{&view, &LocalsView::requestVariable};
};
}

void LocalsViewTest::testStructsAreExpandable()
{
    // gdb sends an empty value for structs and arrays, their members are retrieved
    // with the variablesReference when the item is expanded, bug 526140
    LocalsView view;
    auto *tree = view.findChild<QTreeWidget *>();
    QVERIFY(tree);
    QSignalSpy spy(&view, &LocalsView::requestVariable);

    view.insertScopes({dap::Scope(1, QStringLiteral("Locals"))});
    QCOMPARE(tree->topLevelItemCount(), 1);
    auto *locals = tree->topLevelItem(0);
    locals->setExpanded(true);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toInt(), 1);

    view.addVariables(1, {dap::Variable(QStringLiteral("p"), QString(), 4), dap::Variable(QStringLiteral("i"), QStringLiteral("1"), 0)});
    QCOMPARE(locals->childCount(), 2);

    auto *p = locals->child(0);
    QVERIFY(p->childCount() > 0);
    p->setExpanded(true);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toInt(), 4);

    view.addVariables(4, {dap::Variable(QStringLiteral("x"), QStringLiteral("1"), 0), dap::Variable(QStringLiteral("y"), QStringLiteral("2"), 0)});
    QCOMPARE(p->childCount(), 2);
    QCOMPARE(p->child(0)->text(0), QStringLiteral("x"));
    QCOMPARE(p->child(1)->text(0), QStringLiteral("y"));

    // a variable without members can't be expanded
    QCOMPARE(locals->child(1)->childCount(), 0);
}

void LocalsViewTest::testCollapsedScopeIsRefreshedAfterStep()
{
    Fixture f({var(QStringLiteral("i"), QStringLiteral("1"))});
    f.locals()->setExpanded(false);

    f.view.insertScopes({dap::Scope(10, QStringLiteral("Locals"))});
    QVERIFY(!f.locals()->isExpanded());
    QVERIFY(requestedReferences(f.spy).isEmpty());

    // expanding it later must fetch the new variables, not show the stale ones
    f.locals()->setExpanded(true);
    QCOMPARE(requestedReferences(f.spy), QList<int>{10});
}

void LocalsViewTest::testVariableWithoutMembersAnymore()
{
    // e.g. an expanded pointer that is null now
    Fixture f({var(QStringLiteral("ptr"), QStringLiteral("0x1234"), 4)});
    auto *ptr = f.locals()->child(0);
    ptr->setExpanded(true);
    f.view.addVariables(4, {var(QStringLiteral("x"), QStringLiteral("1"))});
    f.spy.clear();

    f.view.insertScopes({dap::Scope(10, QStringLiteral("Locals"))});
    QCOMPARE(requestedReferences(f.spy), QList<int>{10});
    f.view.addVariables(10, {var(QStringLiteral("ptr"), QStringLiteral("0x0"))});

    // nothing to request, the old members are gone
    QVERIFY(requestedReferences(f.spy).isEmpty());
    QCOMPARE(ptr->childCount(), 0);
    QCOMPARE(ptr->text(2), QStringLiteral("0x0"));
}

void LocalsViewTest::testClearForgetsVariables()
{
    Fixture f({var(QStringLiteral("p"), QString(), 4)});
    f.locals()->child(0)->setExpanded(true);

    // e.g. the debugger stopped, a late variables response must not touch the deleted items
    f.view.clear();
    QCOMPARE(f.tree->topLevelItemCount(), 0);
    f.view.addVariables(4, {var(QStringLiteral("x"), QStringLiteral("1"))});
    QCOMPARE(f.tree->topLevelItemCount(), 0);
}

QTEST_MAIN(LocalsViewTest)

#include "test_localsview.moc"
