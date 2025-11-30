/*
 *    SPDX-FileCopyrightText: 2025 Waqar Ahmed <waqar.17a@gmail.com>
 *    SPDX-License-Identifier: LGPL-2.0-or-later
 */
#include "breakpointview.h"

#include <QAbstractItemModel>
#include <QHash>
#include <QTreeView>
#include <QtTest/QTest>

class BreakpointBackend : public BackendInterface
{
    Q_OBJECT

public:
    bool isRunning = false;
    QHash<QUrl, QList<dap::Breakpoint>> breakpoints;

    BreakpointBackend(QObject *parent = nullptr)
        : BackendInterface(parent)
    {
    }

    bool debuggerRunning() const override
    {
        return isRunning;
    }
    bool debuggerBusy() const override
    {
        return false;
    }
    bool hasBreakpoint(QUrl const &url, int line) const override
    {
        const auto bps = breakpoints.value(url);
        for (const auto &b : bps) {
            return b.line == line;
        }
        return false;
    }
    bool supportsMovePC() const override
    {
        return false;
    }
    bool supportsRunToCursor() const override
    {
        return false;
    }
    bool canSetBreakpoints() const override
    {
        return true;
    }
    bool canContinue() const override
    {
        return true;
    }
    bool canMove() const override
    {
        return true;
    }
    void setBreakpoints(const QUrl &url, const QList<dap::SourceBreakpoint> &sourceBreaks) override
    {
        QList<dap::Breakpoint> bps;
        bps.reserve(sourceBreaks.size());
        for (const auto &b : sourceBreaks) {
            dap::Breakpoint brk{b.line};
            brk.source = dap::Source(url);
            brk.source.value().name = url.fileName();
            bps.push_back(brk);
        }
        breakpoints[url] = bps;
        Q_EMIT breakPointsSet(url, breakpoints[url]);
    }
    void movePC(QUrl const &, int) override
    {
    }
    void runToCursor(QUrl const &, int) override
    {
    }
    void issueCommand(QString const &) override
    {
    }
    QString targetName() const override
    {
        return QString();
    }
    void setFileSearchPaths(const QStringList &) override
    {
    }

public Q_SLOTS:
    void slotInterrupt() override
    {
    }
    void slotStepInto() override
    {
    }
    void slotStepOver() override
    {
    }
    void slotStepOut() override
    {
    }
    void slotContinue() override
    {
    }
    void slotKill() override
    {
    }
    void slotReRun() override
    {
    }
    QString slotPrintVariable(const QString &) override
    {
        return {};
    }

    void slotQueryLocals(bool) override
    {
    }
    void changeStackFrame(int) override
    {
    }
    void changeThread(int) override
    {
    }
    void changeScope(int) override
    {
    }
    void requestVariable(int) override
    {
    }
};

class BreakpointViewTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testBasic();
    // TODO: test that if backend sets breakpoint at a different location, the mark in document is correct
    // TODO: test breakpoint events
};

static QString stringifyModel(QAbstractItemModel *model, const QModelIndex index = {}, int depth = 0)
{
    QString ret;
    if (index.isValid()) {
        if (depth > 0) {
            ret += QString(depth, u'*');
        }
        const QString checkedStr = index.data(Qt::CheckStateRole).isValid()
            ? index.data(Qt::CheckStateRole).value<Qt::CheckState>() == Qt::Checked ? QStringLiteral("[x]") : QStringLiteral("[]")
            : QString();
        ret += QStringLiteral(" %1%2\n").arg(checkedStr).arg(index.data().toString());
    }

    for (int r = 0; r < model->rowCount(index); r++) {
        const auto child = model->index(r, 0, index);
        ret += stringifyModel(model, child, depth + 1);
    }
    return ret;
}

void BreakpointViewTest::testBasic()
{
    const auto isRunning = {false, true};
    for (auto running : isRunning) {
        auto backend = new BreakpointBackend;
        backend->isRunning = running;
        qDebug() << "backend.running" << running;

        auto bv = new BreakpointView(nullptr, backend, nullptr);

        bv->setBreakpoint(QUrl(QStringLiteral("/file")), 3, std::nullopt);
        bv->setBreakpoint(QUrl(QStringLiteral("/file")), 4, std::nullopt);

        QCOMPARE(QStringLiteral("* Line Breakpoints\n"
                                "** [x]file:3\n"
                                "** [x]file:4\n"),
                 stringifyModel(bv->m_treeview->model()));

        // uncheck second breakpoint
        bv->m_treeview->model()->setData(bv->m_treeview->model()->index(1, 0, bv->m_treeview->model()->index(0, 0, {})),
                                         QVariant(Qt::Unchecked),
                                         Qt::CheckStateRole);
        QCOMPARE(QStringLiteral("* Line Breakpoints\n"
                                "** [x]file:3\n"
                                "** []file:4\n"),
                 stringifyModel(bv->m_treeview->model()));

        QCOMPARE(bv->allBreakpoints().size(), 1);
        auto fileBreaks = bv->allBreakpoints()[QUrl(QStringLiteral("/file"))];
        QCOMPARE(fileBreaks.size(), 1);
        QCOMPARE(fileBreaks.constFirst().line, 3);

        // add a breakpoint at line 2, expect it to be at the top as breakpoints are sorted by line
        bv->setBreakpoint(QUrl(QStringLiteral("/file")), 2, std::nullopt);

        QCOMPARE(QStringLiteral("* Line Breakpoints\n"
                                "** [x]file:2\n"
                                "** [x]file:3\n"
                                "** []file:4\n"),
                 stringifyModel(bv->m_treeview->model()));

        // Remove breakpoint
        bv->setBreakpoint(QUrl(QStringLiteral("/file")), 3, std::nullopt);
        QCOMPARE(QStringLiteral("* Line Breakpoints\n"
                                "** [x]file:2\n"
                                "** []file:4\n"),
                 stringifyModel(bv->m_treeview->model()));

        // check second breakpoint
        bv->m_treeview->model()->setData(bv->m_treeview->model()->index(1, 0, bv->m_treeview->model()->index(0, 0, {})),
                                         QVariant(Qt::Checked),
                                         Qt::CheckStateRole);
        QCOMPARE(QStringLiteral("* Line Breakpoints\n"
                                "** [x]file:2\n"
                                "** [x]file:4\n"),
                 stringifyModel(bv->m_treeview->model()));
        QCOMPARE(bv->allBreakpoints().size(), 1);
        fileBreaks = bv->allBreakpoints()[QUrl(QStringLiteral("/file"))];
        QCOMPARE(fileBreaks.size(), 2);
        QCOMPARE(fileBreaks[0].line, 2);
        QCOMPARE(fileBreaks[1].line, 4);

        // Clear all
        bv->clearLineBreakpoints();
        QCOMPARE(bv->allBreakpoints().size(), 0);
        QCOMPARE(QStringLiteral("* Line Breakpoints\n"), stringifyModel(bv->m_treeview->model()));

        delete bv;
        delete backend;
    }
}

QTEST_MAIN(BreakpointViewTest)

#include "test_breakpointview.moc"
