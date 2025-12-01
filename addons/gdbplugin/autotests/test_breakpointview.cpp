/*
 *    SPDX-FileCopyrightText: 2025 Waqar Ahmed <waqar.17a@gmail.com>
 *    SPDX-License-Identifier: LGPL-2.0-or-later
 */
#include "breakpointview.h"

#include <QAbstractItemModel>
#include <QHash>
#include <QTreeView>
#include <QtTest/QTest>

#include <KTextEditor/Application>
#include <KTextEditor/Document>
#include <KTextEditor/Editor>

class KApp : public QObject
{
    Q_OBJECT
public:
    Q_INVOKABLE KTextEditor::Document *findUrl(const QUrl &url)
    {
        return docs.value(url, nullptr);
    }

    QHash<QUrl, KTextEditor::Document *> docs;
};

class BreakpointBackend : public BackendInterface
{
    Q_OBJECT

public:
    bool isRunning = false;
    QHash<QUrl, QList<dap::Breakpoint>> breakpoints;
    int idCounter = 0;

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
            brk.id = idCounter++;
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
    void testBreakpointChangedEvent();
    void testBreakpointRemovedEvent();
    void testBreakpointNewEvent();
    void testBreakpointWithDocument();
    // TODO: test that if backend sets breakpoint at a different location, the mark in document is correct
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
        auto backend = std::make_unique<BreakpointBackend>();
        backend->isRunning = running;
        qDebug() << "backend.running" << running;

        auto bv = std::make_unique<BreakpointView>(nullptr, backend.get(), nullptr);

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
    }
}

void BreakpointViewTest::testBreakpointChangedEvent()
{
    const QUrl url1 = QUrl(QStringLiteral("/file"));
    auto backend = std::make_unique<BreakpointBackend>();
    backend->isRunning = true;
    auto bv = std::make_unique<BreakpointView>(nullptr, backend.get(), nullptr);

    bv->setBreakpoint(url1, 3, std::nullopt);
    bv->setBreakpoint(url1, 4, std::nullopt);

    QCOMPARE(QStringLiteral("* Line Breakpoints\n"
                            "** [x]file:3\n"
                            "** [x]file:4\n"),
             stringifyModel(bv->m_treeview->model()));

    backend->breakpoints[url1].front().line = 5;
    Q_EMIT backend->breakpointEvent(backend->breakpoints[url1].front(), BackendInterface::Changed);
    QCOMPARE(QStringLiteral("* Line Breakpoints\n"
                            "** [x]file:4\n"
                            "** [x]file:5\n"),
             stringifyModel(bv->m_treeview->model()));

    backend->breakpoints[url1].front().line = 3;
    Q_EMIT backend->breakpointEvent(backend->breakpoints[url1].front(), BackendInterface::Changed);
    QCOMPARE(QStringLiteral("* Line Breakpoints\n"
                            "** [x]file:3\n"
                            "** [x]file:4\n"),
             stringifyModel(bv->m_treeview->model()));
}

void BreakpointViewTest::testBreakpointRemovedEvent()
{
    const QUrl url1 = QUrl(QStringLiteral("/file"));
    auto backend = std::make_unique<BreakpointBackend>();
    backend->isRunning = true;
    auto bv = std::make_unique<BreakpointView>(nullptr, backend.get(), nullptr);

    bv->setBreakpoint(url1, 3, std::nullopt);
    bv->setBreakpoint(url1, 4, std::nullopt);

    Q_EMIT backend->breakpointEvent(backend->breakpoints[url1].front(), BackendInterface::Removed);
    QCOMPARE(QStringLiteral("* Line Breakpoints\n"
                            "** [x]file:4\n"),
             stringifyModel(bv->m_treeview->model()));
}

void BreakpointViewTest::testBreakpointNewEvent()
{
    const QUrl url1 = QUrl(QStringLiteral("/file"));
    auto backend = std::make_unique<BreakpointBackend>();
    backend->isRunning = true;
    auto bv = std::make_unique<BreakpointView>(nullptr, backend.get(), nullptr);

    bv->setBreakpoint(url1, 3, std::nullopt);
    bv->setBreakpoint(url1, 4, std::nullopt);

    dap::Breakpoint brk{1};
    brk.id = backend->idCounter++;
    brk.source = dap::Source(url1);
    brk.source.value().name = url1.fileName();

    backend->breakpoints[url1] << brk;
    Q_EMIT backend->breakpointEvent(backend->breakpoints[url1].last(), BackendInterface::New);

    QCOMPARE(QStringLiteral("* Line Breakpoints\n"
                            "** [x]file:1\n"
                            "** [x]file:3\n"
                            "** [x]file:4\n"),
             stringifyModel(bv->m_treeview->model()));
}

void BreakpointViewTest::testBreakpointWithDocument()
{
    auto backend = std::make_unique<BreakpointBackend>();
    backend->isRunning = true;
    auto bv = std::make_unique<BreakpointView>(nullptr, backend.get(), nullptr);

    auto editor = KTextEditor::Editor::instance();
    auto app = new KApp;
    auto kteApp = new KTextEditor::Application(app);
    editor->setApplication(kteApp);
    auto doc = std::unique_ptr<KTextEditor::Document>(editor->createDocument(nullptr));

    const auto url = QUrl::fromLocalFile(QStringLiteral(":/kxmlgui5/kate/kateui.rc"));
    QVERIFY(url.isValid());
    doc->openUrl(url);
    app->docs[url] = doc.get();

    connect(doc.get(), &KTextEditor::Document::markChanged, bv.get(), &BreakpointView::updateBreakpoints);

    doc->setMark(2, KTextEditor::Document::BreakpointActive);
    doc->setMark(5, KTextEditor::Document::BreakpointActive);
    QCOMPARE(doc->marks().size(), 2);
    QCOMPARE(QStringLiteral("* Line Breakpoints\n"
                            "** [x]kateui.rc:3\n"
                            "** [x]kateui.rc:6\n"),
             stringifyModel(bv->m_treeview->model()));

    // Emit a breakpoint new event
    dap::Breakpoint brk{8};
    brk.id = backend->idCounter++;
    brk.source = dap::Source(url);
    brk.source.value().name = url.fileName();
    backend->breakpoints[url] << brk;
    Q_EMIT backend->breakpointEvent(backend->breakpoints[url].last(), BackendInterface::New);

    // Expect breakpoint in doc and model
    QCOMPARE(QStringLiteral("* Line Breakpoints\n"
                            "** [x]kateui.rc:3\n"
                            "** [x]kateui.rc:6\n"
                            "** [x]kateui.rc:8\n"),
             stringifyModel(bv->m_treeview->model()));
    QVERIFY((doc->mark(7) & KTextEditor::Document::BreakpointActive) != 0);
    QCOMPARE(doc->marks().size(), 3);

    // Test removing breakpoint from doc
    doc->removeMark(2, KTextEditor::Document::BreakpointActive);
    QCOMPARE(QStringLiteral("* Line Breakpoints\n"
                            "** [x]kateui.rc:6\n"
                            "** [x]kateui.rc:8\n"),
             stringifyModel(bv->m_treeview->model()));

    // Breakpoint changed by backend, expect document to have breakpoint mark at new location
    backend->breakpoints[url].front().line = 2;
    Q_EMIT backend->breakpointEvent(backend->breakpoints[url].front(), BackendInterface::Changed);
    QCOMPARE(QStringLiteral("* Line Breakpoints\n"
                            "** [x]kateui.rc:2\n"
                            "** [x]kateui.rc:8\n"),
             stringifyModel(bv->m_treeview->model()));
    QCOMPARE(doc->marks().size(), 2);
    QVERIFY((doc->mark(1) & KTextEditor::Document::BreakpointActive) != 0);
}

QTEST_MAIN(BreakpointViewTest)

#include "test_breakpointview.moc"
