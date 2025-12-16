/*
 *    SPDX-FileCopyrightText: 2025 Waqar Ahmed <waqar.17a@gmail.com>
 *    SPDX-License-Identifier: LGPL-2.0-or-later
 */
#include "breakpointview.h"

#include <QAbstractItemModel>
#include <QHash>
#include <QMenu>
#include <QTest>
#include <QTreeView>

#include <KTextEditor/Application>
#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/View>

class KApp : public QObject
{
    Q_OBJECT
public:
    Q_INVOKABLE KTextEditor::Document *findUrl(const QUrl &url) // NOLINT(readability-make-member-function-const)
    {
        return docs.value(url, nullptr);
    }

    Q_INVOKABLE QList<KTextEditor::Document *> documents() // NOLINT(readability-make-member-function-const)
    {
        return docs.values();
    }

    void addDoc(const QUrl &url, KTextEditor::Document *doc)
    {
        docs[url] = doc;
        Q_EMIT KTextEditor::Editor::instance()->application()->documentCreated(doc);
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

    // Emulates the behaviour where a backend sets the breakpoint at
    // a different line than what was requested
    int offsetLinesBy = 0;

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

    bool supportsMovePC() const override
    {
        return false;
    }
    bool supportsRunToCursor() const override
    {
        return false;
    }
    bool supportsFunctionBreakpoints() const override
    {
        return true;
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
            dap::Breakpoint brk{b.line + offsetLinesBy};
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

    void setFunctionBreakpoints(const QList<dap::FunctionBreakpoint> &breakpoints) override
    {
        QList<dap::Breakpoint> response;
        response.reserve(breakpoints.size());
        for (const auto &_ : breakpoints) {
            dap::Breakpoint bp;
            bp.id = idCounter++;
            bp.instructionReference = QStringLiteral("0xffee2211");
            bp.verified = true;
            response.push_back(bp);
        }

        Q_EMIT functionBreakpointsSet(breakpoints, response);
    }

    QList<dap::ExceptionBreakpointsFilter> exceptionBreakpointFilters() const override
    {
        return {};
    }

    void setExceptionBreakpoints(const QStringList &filters) override
    {
        std::ignore = filters;
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
public:
    explicit BreakpointViewTest(QObject *parent = nullptr)
        : QObject(parent)
    {
        auto editor = KTextEditor::Editor::instance();
        kapp = new KApp;
        auto kteApp = new KTextEditor::Application(kapp);
        editor->setApplication(kteApp);
    }

private Q_SLOTS:
    void cleanup()
    {
        kapp->docs.clear();
    }

    void testLineBreakpointsBasic();
    void testBreakpointChangedEvent();
    void testBreakpointRemovedEvent();
    void testBreakpointNewEvent();
    void testBreakpointWithDocument();
    void testBreakpointSetAtDifferentLine();
    void testAddRemoveBreakpointRequested();
    void testListBreakpointsRequested();
    void testBreakpointsGetAddedToDocOnViewCreation();
    void testRunToCursor();
    void testFunctionBreakpoints();
    void testFunctionBreakpointUncheckCheck();
    void testUserRemovedBreakpoint();

private:
    std::unique_ptr<KTextEditor::Document> createDocument(const QUrl &url)
    {
        Q_ASSERT(url.isValid());
        auto doc = std::unique_ptr<KTextEditor::Document>(KTextEditor::Editor::instance()->createDocument(nullptr));
        doc->openUrl(url);
        kapp->addDoc(url, doc.get());
        return doc;
    }

private:
    KApp *kapp = nullptr;
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

static QString stringifyLineBreakpoints(QAbstractItemModel *model)
{
    const auto lineBreakpointIndex = model->index(0, 0, {});
    return stringifyModel(model, lineBreakpointIndex, 1);
}

static QString stringifyFuncBreakpoints(QAbstractItemModel *model)
{
    const auto funcBreakpointIndex = model->index(1, 0, {});
    return stringifyModel(model, funcBreakpointIndex, 1);
}

QAction *getAction(QMenu &menu, const char *name)
{
    const auto actions = menu.actions();
    for (auto a : actions) {
        if (a->text().remove(u'&') == QString::fromUtf8(name)) {
            return a;
        }
    }
    return nullptr;
}

QModelIndex indexForString(QAbstractItemModel *model, const char *str)
{
    const auto indexes =
        model->match(model->index(0, 0), Qt::DisplayRole, QVariant(QString::fromUtf8(str)), 1, Qt::MatchFlags(Qt::MatchRecursive | Qt::MatchExactly));
    return indexes.size() == 1 ? indexes.front() : QModelIndex();
}

void BreakpointViewTest::testLineBreakpointsBasic()
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
                 stringifyLineBreakpoints(bv->m_treeview->model()));

        QCOMPARE(bv->m_treeview->isExpanded(bv->m_treeview->model()->index(0, 0)), true);

        // uncheck second breakpoint
        const auto lineBreakpointParent = bv->m_treeview->model()->index(0, 0, {});
        bv->m_treeview->model()->setData(bv->m_treeview->model()->index(1, 0, lineBreakpointParent), QVariant(Qt::Unchecked), Qt::CheckStateRole);
        QCOMPARE(QStringLiteral("* Line Breakpoints\n"
                                "** [x]file:3\n"
                                "** []file:4\n"),
                 stringifyLineBreakpoints(bv->m_treeview->model()));

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
                 stringifyLineBreakpoints(bv->m_treeview->model()));

        // Remove breakpoint
        bv->setBreakpoint(QUrl(QStringLiteral("/file")), 3, std::nullopt);
        QCOMPARE(QStringLiteral("* Line Breakpoints\n"
                                "** [x]file:2\n"
                                "** []file:4\n"),
                 stringifyLineBreakpoints(bv->m_treeview->model()));

        // check second breakpoint
        bv->m_treeview->model()->setData(bv->m_treeview->model()->index(1, 0, bv->m_treeview->model()->index(0, 0, {})),
                                         QVariant(Qt::Checked),
                                         Qt::CheckStateRole);
        QCOMPARE(QStringLiteral("* Line Breakpoints\n"
                                "** [x]file:2\n"
                                "** [x]file:4\n"),
                 stringifyLineBreakpoints(bv->m_treeview->model()));
        QCOMPARE(bv->allBreakpoints().size(), 1);
        fileBreaks = bv->allBreakpoints()[QUrl(QStringLiteral("/file"))];
        QCOMPARE(fileBreaks.size(), 2);
        QCOMPARE(fileBreaks[0].line, 2);
        QCOMPARE(fileBreaks[1].line, 4);

        // Clear all
        bv->clearLineBreakpoints();
        QCOMPARE(bv->allBreakpoints().size(), 0);
        QCOMPARE(QStringLiteral("* Line Breakpoints\n"), stringifyLineBreakpoints(bv->m_treeview->model()));
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
             stringifyLineBreakpoints(bv->m_treeview->model()));

    backend->breakpoints[url1].front().line = 5;
    Q_EMIT backend->breakpointEvent(backend->breakpoints[url1].front(), BackendInterface::Changed);
    QCOMPARE(QStringLiteral("* Line Breakpoints\n"
                            "** [x]file:4\n"
                            "** [x]file:5\n"),
             stringifyLineBreakpoints(bv->m_treeview->model()));

    backend->breakpoints[url1].front().line = 3;
    Q_EMIT backend->breakpointEvent(backend->breakpoints[url1].front(), BackendInterface::Changed);
    QCOMPARE(QStringLiteral("* Line Breakpoints\n"
                            "** [x]file:3\n"
                            "** [x]file:4\n"),
             stringifyLineBreakpoints(bv->m_treeview->model()));

    backend->breakpoints[url1].back().line = 2;
    Q_EMIT backend->breakpointEvent(backend->breakpoints[url1].back(), BackendInterface::Changed);
    QCOMPARE(QStringLiteral("* Line Breakpoints\n"
                            "** [x]file:2\n"
                            "** [x]file:3\n"),
             stringifyLineBreakpoints(bv->m_treeview->model()));
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
             stringifyLineBreakpoints(bv->m_treeview->model()));
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
             stringifyLineBreakpoints(bv->m_treeview->model()));
}

void BreakpointViewTest::testBreakpointWithDocument()
{
    auto backend = std::make_unique<BreakpointBackend>();
    backend->isRunning = true;
    const auto url = QUrl::fromLocalFile(QStringLiteral(":/kxmlgui5/kate/kateui.rc"));
    auto doc = createDocument(url);
    auto bv = std::make_unique<BreakpointView>(nullptr, backend.get(), nullptr);

    doc->setMark(2, KTextEditor::Document::BreakpointActive);
    doc->setMark(5, KTextEditor::Document::BreakpointActive);
    QCOMPARE(doc->marks().size(), 2);
    QCOMPARE(QStringLiteral("* Line Breakpoints\n"
                            "** [x]kateui.rc:3\n"
                            "** [x]kateui.rc:6\n"),
             stringifyLineBreakpoints(bv->m_treeview->model()));

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
             stringifyLineBreakpoints(bv->m_treeview->model()));
    QVERIFY((doc->mark(7) & KTextEditor::Document::BreakpointActive) != 0);
    QCOMPARE(doc->marks().size(), 3);

    // Test removing breakpoint from doc
    doc->removeMark(2, KTextEditor::Document::BreakpointActive);
    QCOMPARE(QStringLiteral("* Line Breakpoints\n"
                            "** [x]kateui.rc:6\n"
                            "** [x]kateui.rc:8\n"),
             stringifyLineBreakpoints(bv->m_treeview->model()));

    // Breakpoint changed by backend, expect document to have breakpoint mark at new location
    backend->breakpoints[url].front().line = 2;
    Q_EMIT backend->breakpointEvent(backend->breakpoints[url].front(), BackendInterface::Changed);
    QCOMPARE(QStringLiteral("* Line Breakpoints\n"
                            "** [x]kateui.rc:2\n"
                            "** [x]kateui.rc:8\n"),
             stringifyLineBreakpoints(bv->m_treeview->model()));
    QCOMPARE(doc->marks().size(), 2);
    QVERIFY((doc->mark(1) & KTextEditor::Document::BreakpointActive) != 0);
}

void BreakpointViewTest::testBreakpointSetAtDifferentLine()
{
    // Test that, if the backend sets the breakpoint at a different location
    // than what was requested, the UI shows it correctly i.e.,
    // Document mark should be at the correct line, the model should show correct breakpoint
    auto backend = std::make_unique<BreakpointBackend>();
    backend->isRunning = true;
    const auto url = QUrl::fromLocalFile(QStringLiteral(":/kxmlgui5/kate/kateui.rc"));
    auto doc = createDocument(url);

    auto bv = std::make_unique<BreakpointView>(nullptr, backend.get(), nullptr);

    backend->offsetLinesBy = 3;

    doc->setMark(2, KTextEditor::Document::BreakpointActive);
    QCOMPARE(doc->marks().size(), 1);
    QVERIFY((doc->mark(2 + backend->offsetLinesBy) & KTextEditor::Document::BreakpointActive) != 0);
    QCOMPARE(QStringLiteral("* Line Breakpoints\n"
                            "** [x]kateui.rc:6\n"),
             stringifyLineBreakpoints(bv->m_treeview->model()));
}

void BreakpointViewTest::testAddRemoveBreakpointRequested()
{
    auto backend = std::make_unique<BreakpointBackend>();
    backend->isRunning = true;
    auto bv = std::make_unique<BreakpointView>(nullptr, backend.get(), nullptr);
    const auto url = QUrl::fromLocalFile(QStringLiteral(":/kxmlgui5/kate/kateui.rc"));
    auto doc = createDocument(url);

    backend->addBreakpointRequested(url, 1);
    QCOMPARE(QStringLiteral("* Line Breakpoints\n"
                            "** [x]kateui.rc:1\n"),
             stringifyLineBreakpoints(bv->m_treeview->model()));
    QVERIFY((doc->mark(0) & KTextEditor::Document::BreakpointActive) != 0);

    // try to breakpoint same line again
    QString err;
    connect(backend.get(), &BackendInterface::outputError, this, [&err](const QString &s) {
        err = s;
    });
    backend->addBreakpointRequested(url, 1);
    QCOMPARE(QStringLiteral("* Line Breakpoints\n"
                            "** [x]kateui.rc:1\n"),
             stringifyLineBreakpoints(bv->m_treeview->model()));
    QCOMPARE(err, QStringLiteral("line 1 already has a breakpoint"));

    backend->addBreakpointRequested(url, 2);
    QCOMPARE(QStringLiteral("* Line Breakpoints\n"
                            "** [x]kateui.rc:1\n"
                            "** [x]kateui.rc:2\n"),
             stringifyLineBreakpoints(bv->m_treeview->model()));
    QVERIFY((doc->mark(1) & KTextEditor::Document::BreakpointActive) != 0);

    backend->removeBreakpointRequested(url, 2);
    QCOMPARE(QStringLiteral("* Line Breakpoints\n"
                            "** [x]kateui.rc:1\n"),
             stringifyLineBreakpoints(bv->m_treeview->model()));
    QVERIFY((doc->mark(1) & KTextEditor::Document::BreakpointActive) == 0);

    backend->removeBreakpointRequested(url, 1);
    QCOMPARE(QStringLiteral("* Line Breakpoints\n"), stringifyLineBreakpoints(bv->m_treeview->model()));
    QVERIFY((doc->mark(0) & KTextEditor::Document::BreakpointActive) == 0);
}

void BreakpointViewTest::testListBreakpointsRequested()
{
    auto backend = std::make_unique<BreakpointBackend>();
    backend->isRunning = true;
    auto bv = std::make_unique<BreakpointView>(nullptr, backend.get(), nullptr);
    const QUrl url1(QStringLiteral("/file"));
    const QUrl url2(QStringLiteral("/file2"));

    bv->setBreakpoint(url1, 3, std::nullopt);
    bv->setBreakpoint(url1, 4, std::nullopt);

    bv->setBreakpoint(url2, 3, std::nullopt);
    bv->setBreakpoint(url2, 8, std::nullopt);

    QString out;
    connect(backend.get(), &BackendInterface::outputText, backend.get(), [&out](const QString &text) {
        out = text;
    });

    backend->listBreakpointsRequested();
    QCOMPARE(out,
             QStringLiteral("\n[0.!] /file: 3->3\n"
                            "[1.!] /file: 4->4\n"
                            "[2.!] /file2: 3->3\n"
                            "[3.!] /file2: 8->8\n"));
}

void BreakpointViewTest::testBreakpointsGetAddedToDocOnViewCreation()
{
    auto backend = std::make_unique<BreakpointBackend>();
    backend->isRunning = true;
    const auto url = QUrl::fromLocalFile(QStringLiteral(":/kxmlgui5/kate/kateui.rc"));
    auto bv = std::make_unique<BreakpointView>(nullptr, backend.get(), nullptr);

    bv->setBreakpoint(url, 3, std::nullopt);
    bv->setBreakpoint(url, 4, std::nullopt);

    QCOMPARE(QStringLiteral("* Line Breakpoints\n"
                            "** [x]kateui.rc:3\n"
                            "** [x]kateui.rc:4\n"),
             stringifyLineBreakpoints(bv->m_treeview->model()));

    auto doc = createDocument(url);
    QCOMPARE(doc->mark(2), 0);

    auto view = std::unique_ptr<KTextEditor::View>(doc->createView(nullptr));
    QCOMPARE(doc->mark(2), KTextEditor::Document::BreakpointActive);
}

void BreakpointViewTest::testRunToCursor()
{
    auto backend = std::make_unique<BreakpointBackend>();
    backend->isRunning = true;
    const auto url = QUrl::fromLocalFile(QStringLiteral(":/kxmlgui5/kate/kateui.rc"));
    auto bv = std::make_unique<BreakpointView>(nullptr, backend.get(), nullptr);

    // use clicks run to cursor
    backend->offsetLinesBy = 2;
    bv->runToPosition(url, 5);

    QCOMPARE(QStringLiteral("* Line Breakpoints\n"
                            "** [x]kateui.rc:7\n"),
             stringifyLineBreakpoints(bv->m_treeview->model()));

    backend->offsetLinesBy = 0;

    // backend stops at the breakpoint
    bv->onStoppedAtLine(url, 7);

    QCOMPARE(QStringLiteral("* Line Breakpoints\n"), stringifyLineBreakpoints(bv->m_treeview->model()));
}

void BreakpointViewTest::testFunctionBreakpoints()
{
    auto backend = std::make_unique<BreakpointBackend>();
    backend->isRunning = true;
    const auto url = QUrl::fromLocalFile(QStringLiteral(":/kxmlgui5/kate/kateui.rc"));
    auto bv = std::make_unique<BreakpointView>(nullptr, backend.get(), nullptr);

    bv->addFunctionBreakpoint(QStringLiteral("func1"));
    QCOMPARE(QStringLiteral("* Function Breakpoints\n"
                            "** [x]func1 (0xffee2211) [verified]\n"),
             stringifyFuncBreakpoints(bv->m_treeview->model()));

    // try add it again, no change
    bv->addFunctionBreakpoint(QStringLiteral("func1"));
    QCOMPARE(QStringLiteral("* Function Breakpoints\n"
                            "** [x]func1 (0xffee2211) [verified]\n"),
             stringifyFuncBreakpoints(bv->m_treeview->model()));

    bv->addFunctionBreakpoint(QStringLiteral("func2"));
    QCOMPARE(QStringLiteral("* Function Breakpoints\n"
                            "** [x]func1 (0xffee2211) [verified]\n"
                            "** [x]func2 (0xffee2211) [verified]\n"),
             stringifyFuncBreakpoints(bv->m_treeview->model()));

    const auto funcBreakpointParent = bv->m_treeview->model()->index(1, 0, {});
    bv->m_treeview->model()->setData(bv->m_treeview->model()->index(1, 0, funcBreakpointParent), QVariant(Qt::Unchecked), Qt::CheckStateRole);
    QCOMPARE(QStringLiteral("* Function Breakpoints\n"
                            "** []func2 (0xffee2211) [verified]\n"
                            "** [x]func1 (0xffee2211) [verified]\n"),
             stringifyFuncBreakpoints(bv->m_treeview->model()));

    {
        const auto index = indexForString(bv->m_treeview->model(), "func1 (0xffee2211) [verified]");
        QVERIFY(index.isValid());

        QMenu menu;
        bv->buildContextMenu(index, &menu);
        auto a = getAction(menu, "Remove Breakpoint");
        QVERIFY(a);
        a->trigger();
        QCOMPARE(QStringLiteral("* Function Breakpoints\n"
                                "** []func2 (0xffee2211) [verified]\n"),
                 stringifyFuncBreakpoints(bv->m_treeview->model()));
    }

    {
        const auto index = indexForString(bv->m_treeview->model(), "func2 (0xffee2211) [verified]");
        QVERIFY(index.isValid());
        QMenu menu;
        bv->buildContextMenu(index, &menu);
        auto a = getAction(menu, "Remove Breakpoint");
        QVERIFY(a);
        a->trigger();
        QCOMPARE(QStringLiteral("* Function Breakpoints\n"), stringifyFuncBreakpoints(bv->m_treeview->model()));
    }
}

void BreakpointViewTest::testFunctionBreakpointUncheckCheck()
{
    auto backend = std::make_unique<BreakpointBackend>();
    backend->isRunning = true;
    const auto url = QUrl::fromLocalFile(QStringLiteral(":/kxmlgui5/kate/kateui.rc"));
    auto bv = std::make_unique<BreakpointView>(nullptr, backend.get(), nullptr);

    bv->addFunctionBreakpoint(QStringLiteral("func1"));
    bv->addFunctionBreakpoint(QStringLiteral("func2"));
    bv->addFunctionBreakpoint(QStringLiteral("func3"));
    bv->addFunctionBreakpoint(QStringLiteral("func4"));

    QCOMPARE(QStringLiteral("* Function Breakpoints\n"
                            "** [x]func1 (0xffee2211) [verified]\n"
                            "** [x]func2 (0xffee2211) [verified]\n"
                            "** [x]func3 (0xffee2211) [verified]\n"
                            "** [x]func4 (0xffee2211) [verified]\n"),
             stringifyFuncBreakpoints(bv->m_treeview->model()));

    auto index = indexForString(bv->m_treeview->model(), "func4 (0xffee2211) [verified]");
    QVERIFY(index.isValid());
    bv->m_treeview->model()->setData(index, Qt::Unchecked, Qt::CheckStateRole);

    QCOMPARE(QStringLiteral("* Function Breakpoints\n"
                            "** []func4 (0xffee2211) [verified]\n"
                            "** [x]func1 (0xffee2211) [verified]\n"
                            "** [x]func2 (0xffee2211) [verified]\n"
                            "** [x]func3 (0xffee2211) [verified]\n"),
             stringifyFuncBreakpoints(bv->m_treeview->model()));

    index = indexForString(bv->m_treeview->model(), "func2 (0xffee2211) [verified]");
    QVERIFY(index.isValid());
    bv->m_treeview->model()->setData(index, Qt::Unchecked, Qt::CheckStateRole);

    index = indexForString(bv->m_treeview->model(), "func3 (0xffee2211) [verified]");
    QVERIFY(index.isValid());
    bv->m_treeview->model()->setData(index, Qt::Unchecked, Qt::CheckStateRole);

    QCOMPARE(QStringLiteral("* Function Breakpoints\n"
                            "** []func4 (0xffee2211) [verified]\n"
                            "** []func2 (0xffee2211) [verified]\n"
                            "** []func3 (0xffee2211) [verified]\n"
                            "** [x]func1 (0xffee2211) [verified]\n"),
             stringifyFuncBreakpoints(bv->m_treeview->model()));

    index = indexForString(bv->m_treeview->model(), "func2 (0xffee2211) [verified]");
    QVERIFY(index.isValid());
    bv->m_treeview->model()->setData(index, Qt::Checked, Qt::CheckStateRole);

    QCOMPARE(QStringLiteral("* Function Breakpoints\n"
                            "** []func4 (0xffee2211) [verified]\n"
                            "** []func3 (0xffee2211) [verified]\n"
                            "** [x]func2 (0xffee2211) [verified]\n"
                            "** [x]func1 (0xffee2211) [verified]\n"),
             stringifyFuncBreakpoints(bv->m_treeview->model()));
}

void BreakpointViewTest::testUserRemovedBreakpoint()
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
             stringifyLineBreakpoints(bv->m_treeview->model()));

    {
        const auto index = indexForString(bv->m_treeview->model(), "file:3");
        QVERIFY(index.isValid());
        QMenu menu;
        bv->buildContextMenu(index, &menu);
        auto a = getAction(menu, "Remove Breakpoint");
        QVERIFY(a);
        a->trigger();
        QCOMPARE(QStringLiteral("* Line Breakpoints\n"
                                "** [x]file:4\n"),
                 stringifyLineBreakpoints(bv->m_treeview->model()));
    }

    // uncheck breakpoint
    const auto index = indexForString(bv->m_treeview->model(), "file:4");
    QVERIFY(index.isValid());
    bv->m_treeview->model()->setData(index, Qt::Unchecked, Qt::CheckStateRole);
    QCOMPARE(QStringLiteral("* Line Breakpoints\n"
                            "** []file:4\n"),
             stringifyLineBreakpoints(bv->m_treeview->model()));

    {
        const auto index = indexForString(bv->m_treeview->model(), "file:4");
        QMenu menu;
        bv->buildContextMenu(index, &menu);
        auto a = getAction(menu, "Remove Breakpoint");
        QVERIFY(a);
        a->trigger();
        QCOMPARE(QStringLiteral("* Line Breakpoints\n"), stringifyLineBreakpoints(bv->m_treeview->model()));
    }
}

QTEST_MAIN(BreakpointViewTest)

#include "test_breakpointview.moc"
