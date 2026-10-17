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
    void testItemColumns();
    void testValueTooltipShowsItemCounts();
    void testTooltipsEscapeHtml();
    void testExpandedScopeIsRefreshedAfterStep();
    void testCollapsedScopeIsRefreshedAfterStep();
    void testExtraScopesAreRemoved();
    void testExpandedVariableStaysExpandedAfterStep();
    void testExtraVariablesAreRemoved();
    void testVariableWithoutMembersAnymore();
    void testClearForgetsVariables();
};

namespace
{
dap::Variable var(const QString &name, const QString &value, int reference = 0)
{
    return dap::Variable(name, value, reference);
}

QStringList childNames(QTreeWidgetItem *item)
{
    QStringList names;
    for (int i = 0; i < item->childCount(); ++i) {
        names << item->child(i)->text(0);
    }
    return names;
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
    QCOMPARE(requestedReferences(spy), QList<int>{1});

    view.addVariables(1, {var(QStringLiteral("p"), QString(), 4), var(QStringLiteral("i"), QStringLiteral("1"))});
    QCOMPARE(childNames(locals), (QStringList{QStringLiteral("p"), QStringLiteral("i")}));

    auto *p = locals->child(0);
    QVERIFY(p->childCount() > 0);
    p->setExpanded(true);
    QCOMPARE(requestedReferences(spy), QList<int>{4});

    view.addVariables(4, {var(QStringLiteral("x"), QStringLiteral("1")), var(QStringLiteral("y"), QStringLiteral("2"))});
    QCOMPARE(childNames(p), (QStringList{QStringLiteral("x"), QStringLiteral("y")}));

    // a variable without members can't be expanded
    QCOMPARE(locals->child(1)->childCount(), 0);
}

void LocalsViewTest::testItemColumns()
{
    dap::Variable changed = var(QStringLiteral("i"), QStringLiteral("42"));
    changed.type = QStringLiteral("int");
    changed.valueChanged = true;
    Fixture f({changed, var(QStringLiteral("j"), QStringLiteral("7"))});

    auto *i = f.locals()->child(0);
    QCOMPARE(i->text(0), QStringLiteral("i"));
    QCOMPARE(i->text(1), QStringLiteral("int"));
    QCOMPARE(i->text(2), QStringLiteral("42"));
    // changed values are bold
    QVERIFY(i->font(0).bold());
    QVERIFY(!f.locals()->child(1)->font(0).bold());
}

void LocalsViewTest::testValueTooltipShowsItemCounts()
{
    dap::Variable array = var(QStringLiteral("arr"), QString(), 5);
    array.indexedVariables = 3;
    dap::Variable object = var(QStringLiteral("p"), QString(), 6);
    object.namedVariables = 2;
    Fixture f({array, object});

    QVERIFY2(f.locals()->child(0)->toolTip(2).contains(QStringLiteral("3")), qPrintable(f.locals()->child(0)->toolTip(2)));
    QVERIFY2(f.locals()->child(1)->toolTip(2).contains(QStringLiteral("2")), qPrintable(f.locals()->child(1)->toolTip(2)));
}

void LocalsViewTest::testTooltipsEscapeHtml()
{
    // e.g. gdb shows the symbol for addresses
    dap::Variable rip = var(QStringLiteral("rip"), QStringLiteral("0x1234 <main+75>"));
    rip.type = QStringLiteral("std::vector<int>");
    Fixture f({rip});

    const auto *item = f.locals()->child(0);
    QVERIFY2(item->toolTip(2).contains(QStringLiteral("&lt;main+75&gt;")), qPrintable(item->toolTip(2)));
    QVERIFY2(item->toolTip(0).contains(QStringLiteral("std::vector&lt;int&gt;")), qPrintable(item->toolTip(0)));
}

void LocalsViewTest::testExpandedScopeIsRefreshedAfterStep()
{
    Fixture f({var(QStringLiteral("i"), QStringLiteral("1"))});
    auto *iBefore = f.locals()->child(0);

    // after a step the scopes are sent again, with new references
    f.view.insertScopes({dap::Scope(10, QStringLiteral("Locals"))});
    QVERIFY(f.locals()->isExpanded());
    QCOMPARE(requestedReferences(f.spy), QList<int>{10});

    f.view.addVariables(10, {var(QStringLiteral("i"), QStringLiteral("2"))});
    // the items are updated in place, nothing collapses
    QCOMPARE(f.locals()->child(0), iBefore);
    QCOMPARE(f.locals()->child(0)->text(2), QStringLiteral("2"));
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

void LocalsViewTest::testExtraScopesAreRemoved()
{
    LocalsView view;
    auto *tree = view.findChild<QTreeWidget *>();
    view.insertScopes({dap::Scope(1, QStringLiteral("Locals")), dap::Scope(2, QStringLiteral("Registers"))});
    QCOMPARE(tree->topLevelItemCount(), 2);
    QCOMPARE(tree->topLevelItem(1)->text(0), QStringLiteral("Registers"));

    view.insertScopes({dap::Scope(3, QStringLiteral("Locals"))});
    QCOMPARE(tree->topLevelItemCount(), 1);

    view.insertScopes({});
    QCOMPARE(tree->topLevelItemCount(), 0);
}

void LocalsViewTest::testExpandedVariableStaysExpandedAfterStep()
{
    Fixture f({var(QStringLiteral("p"), QString(), 4)});
    auto *p = f.locals()->child(0);
    p->setExpanded(true);
    f.view.addVariables(4, {var(QStringLiteral("x"), QStringLiteral("1"))});
    f.spy.clear();

    // step: the scope and then the variables come with new references
    f.view.insertScopes({dap::Scope(10, QStringLiteral("Locals"))});
    QCOMPARE(requestedReferences(f.spy), QList<int>{10});
    f.view.addVariables(10, {var(QStringLiteral("p"), QString(), 40)});
    QCOMPARE(f.locals()->child(0), p);
    QVERIFY(p->isExpanded());
    QCOMPARE(requestedReferences(f.spy), QList<int>{40});

    f.view.addVariables(40, {var(QStringLiteral("x"), QStringLiteral("2"))});
    QCOMPARE(childNames(p), QStringList{QStringLiteral("x")});
    QCOMPARE(p->child(0)->text(2), QStringLiteral("2"));
}

void LocalsViewTest::testExtraVariablesAreRemoved()
{
    Fixture f({var(QStringLiteral("a"), QStringLiteral("1")), var(QStringLiteral("b"), QStringLiteral("2")), var(QStringLiteral("c"), QStringLiteral("3"))});

    f.view.insertScopes({dap::Scope(10, QStringLiteral("Locals"))});
    f.view.addVariables(10, {var(QStringLiteral("a"), QStringLiteral("1"))});
    QCOMPARE(childNames(f.locals()), QStringList{QStringLiteral("a")});

    f.view.insertScopes({dap::Scope(20, QStringLiteral("Locals"))});
    f.view.addVariables(20, {var(QStringLiteral("a"), QStringLiteral("1")), var(QStringLiteral("d"), QStringLiteral("4"))});
    QCOMPARE(childNames(f.locals()), (QStringList{QStringLiteral("a"), QStringLiteral("d")}));
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
