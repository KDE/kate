/*
    SPDX-FileCopyrightText: 2026 Waqar Ahmed <waqar.ahmed@kdab.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "MatchModel.h"
#include "kateapptest.h"

#include <KTextEditor/Application>
#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>

#include <QAbstractItemModel>
#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPushButton>
#include <QStackedWidget>
#include <QTest>
#include <QTreeView>
#include <QUrl>

using namespace Qt::StringLiterals;

// Whole app tests of the search plugin: searches like a user does, in a complete Kate
class SearchPluginTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void init();

    void testSearchInOpenFiles();
    void testGoToNextMatch();
    void testSearchInFolder();
    void testReplaceChecked();
    void testReplaceInFolder();
    void testReplaceSingleMatch();

private:
    // write text into the file fileName in the temp dir and open it
    [[nodiscard]] KTextEditor::View *openText(const QString &text, const QString &fileName);
    // open the search tool view and search for text in the given place, waits until the search is done
    [[nodiscard]] bool search(const QString &text, MatchModel::SearchPlaces place);
    // the results of the current result tab as text, a line per file and an indented line per match:
    // "<file relative to the temp dir>" and "  <line>:<column>: <line text with the match in brackets>", 1-based
    [[nodiscard]] QString results() const;
    // the tree view of the current result tab
    [[nodiscard]] QTreeView *resultsView() const;
    // replace the checked matches of the current result tab with text, waits until the replace is done
    [[nodiscard]] bool replaceChecked(const QString &text);

    // one Kate for all tests, init() resets what the tests change
    std::unique_ptr<KateAppTest> m_app;
    // the search tool view, all the search widgets are in it
    QWidget *m_toolView = nullptr;
};

static const auto SearchPlugin = u"katesearchplugin"_s;

void SearchPluginTest::initTestCase()
{
    m_app = std::make_unique<KateAppTest>(QStringList{SearchPlugin});

    m_toolView = m_app->toolView(u"kate_plugin_katesearch"_s);
    QVERIFY(m_toolView);
}

void SearchPluginTest::cleanupTestCase()
{
    m_app.reset();
}

void SearchPluginTest::init()
{
    // each test opens the documents it searches in, the search options it uses are set by search()
    QVERIFY(m_app->closeAllDocuments());
}

KTextEditor::View *SearchPluginTest::openText(const QString &text, const QString &fileName)
{
    return m_app->openFile(m_app->writeFile(fileName, text));
}

bool SearchPluginTest::search(const QString &text, MatchModel::SearchPlaces place)
{
    QAction *findInFiles = m_app->action(SearchPlugin, u"search_in_files"_s);
    if (!findInFiles) {
        qWarning("No search_in_files action");
        return false;
    }
    findInFiles->trigger();

    auto searchCombo = m_toolView->findChild<QComboBox *>(u"searchCombo"_s);
    auto placeCombo = m_toolView->findChild<QComboBox *>(u"searchPlaceCombo"_s);
    auto searchButton = m_toolView->findChild<QPushButton *>(u"searchButton"_s);
    if (!searchCombo || !placeCombo || !searchButton) {
        qWarning("Search widgets not found");
        return false;
    }

    placeCombo->setCurrentIndex(place);
    searchCombo->setCurrentText(text);

    // like for the user, the button gets enabled after typing
    if (!QTest::qWaitFor([searchButton] {
            return searchButton->isEnabled();
        })) {
        qWarning("The search button didn't get enabled");
        return false;
    }
    searchButton->click();

    // the button is disabled while searching
    if (!QTest::qWaitFor([searchButton] {
            return searchButton->isEnabled();
        })) {
        qWarning("The search didn't finish");
        return false;
    }
    return true;
}

QTreeView *SearchPluginTest::resultsView() const
{
    auto resultWidget = m_toolView->findChild<QStackedWidget *>(u"resultWidget"_s);
    return resultWidget && resultWidget->currentWidget() ? resultWidget->currentWidget()->findChild<QTreeView *>(u"treeView"_s) : nullptr;
}

bool SearchPluginTest::replaceChecked(const QString &text)
{
    auto replaceCombo = m_toolView->findChild<QComboBox *>(u"replaceCombo"_s);
    auto replaceCheckedButton = m_toolView->findChild<QPushButton *>(u"replaceCheckedBtn"_s);
    auto searchButton = m_toolView->findChild<QPushButton *>(u"searchButton"_s);
    if (!replaceCombo || !replaceCheckedButton || !searchButton) {
        qWarning("Replace widgets not found");
        return false;
    }
    if (!replaceCheckedButton->isEnabled()) {
        qWarning("The replace checked button is disabled");
        return false;
    }

    replaceCombo->setCurrentText(text);
    replaceCheckedButton->click();

    // the search button is disabled while replacing, file by file
    if (!QTest::qWaitFor([searchButton] {
            return searchButton->isEnabled();
        })) {
        qWarning("The replace didn't finish");
        return false;
    }
    return true;
}

QString SearchPluginTest::results() const
{
    QTreeView *treeView = resultsView();
    if (!treeView || !treeView->model()) {
        qWarning("No results view");
        return {};
    }

    // the model has one info item, with the files as children, with the matches as children
    QString result;
    const QDir tempDir(m_app->tempDir());
    const QAbstractItemModel *model = treeView->model();
    const QModelIndex info = model->index(0, 0);
    for (int f = 0; f < model->rowCount(info); ++f) {
        const QModelIndex file = model->index(f, 0, info);
        result += tempDir.relativeFilePath(file.data(MatchModel::FileUrlRole).toUrl().toLocalFile()) + u'\n';
        for (int m = 0; m < model->rowCount(file); ++m) {
            const QModelIndex match = model->index(m, 0, file);
            result += u"  %1:%2: "_s.arg(match.data(MatchModel::StartLineRole).toInt() + 1).arg(match.data(MatchModel::StartColumnRole).toInt() + 1);
            result += match.data(MatchModel::PreMatchRole).toString() + u'[' + match.data(MatchModel::MatchRole).toString() + u']'
                + match.data(MatchModel::PostMatchRole).toString() + u'\n';
        }
    }
    return result;
}

void SearchPluginTest::testSearchInOpenFiles()
{
    QVERIFY(openText(u"foo bar\nbaz Foo\n"_s, u"a.txt"_s));
    QVERIFY(openText(u"nothing here\n"_s, u"b.txt"_s));
    QVERIFY(openText(u"xfoo\n"_s, u"c.txt"_s));

    QVERIFY(search(u"foo"_s, MatchModel::OpenFiles));

    // case insensitive per default, files without matches are not listed
    QCOMPARE(results(),
             u"a.txt\n"
             u"  1:1: [foo] bar\n"
             u"  2:5: baz [Foo]\n"
             u"c.txt\n"
             u"  1:2: x[foo]\n"_s);
}

void SearchPluginTest::testGoToNextMatch()
{
    QVERIFY(openText(u"one foo\ntwo\nthree foo\n"_s, u"a.txt"_s));
    KTextEditor::View *view = openText(u"foo\n"_s, u"b.txt"_s);
    QVERIFY(view);

    QVERIFY(search(u"foo"_s, MatchModel::CurrentFile));
    QCOMPARE(results(),
             u"b.txt\n"
             u"  1:1: [foo]\n"_s);

    // the matches of the other file
    QVERIFY(m_app->openFile(m_app->tempDir() + u"/a.txt"_s) != nullptr);
    QVERIFY(search(u"foo"_s, MatchModel::CurrentFile));
    QCOMPARE(results(),
             u"a.txt\n"
             u"  1:5: one [foo]\n"
             u"  3:7: three [foo]\n"_s);

    QAction *next = m_app->action(SearchPlugin, u"go_to_next_match"_s);
    QVERIFY(next);

    // goes through the matches, the cursor at the start of each
    next->trigger();
    KTextEditor::View *active = m_app->mainWindow()->activeView();
    QVERIFY(active);
    QCOMPARE(active->document()->url().fileName(), u"a.txt"_s);
    QCOMPARE(active->cursorPosition(), KTextEditor::Cursor(0, 4));

    next->trigger();
    QCOMPARE(m_app->mainWindow()->activeView()->cursorPosition(), KTextEditor::Cursor(2, 6));
}

void SearchPluginTest::testSearchInFolder()
{
    // files on disk only, not opened
    QVERIFY(!m_app->writeFile(u"folder/x.txt"_s, u"needle\n"_s).isEmpty());
    QVERIFY(!m_app->writeFile(u"folder/sub/y.txt"_s, u"hay\nhay needle\n"_s).isEmpty());
    QVERIFY(!m_app->writeFile(u"other/z.txt"_s, u"needle\n"_s).isEmpty());

    // the folder to search in
    auto folderRequester = m_toolView->findChild<QWidget *>(u"folderRequester"_s);
    QVERIFY(folderRequester);
    QVERIFY(folderRequester->setProperty("url", QUrl::fromLocalFile(m_app->tempDir() + u"/folder"_s)));
    auto recursive = m_toolView->findChild<QCheckBox *>(u"recursiveCheckBox"_s);
    QVERIFY(recursive);
    recursive->setChecked(true);

    QVERIFY(search(u"needle"_s, MatchModel::Folder));
    QCOMPARE(results(),
             u"folder/sub/y.txt\n"
             u"  2:5: hay [needle]\n"
             u"folder/x.txt\n"
             u"  1:1: [needle]\n"_s);
}

void SearchPluginTest::testReplaceChecked()
{
    KTextEditor::View *a = openText(u"foo bar\nbaz Foo foo\n"_s, u"a.txt"_s);
    KTextEditor::View *b = openText(u"xfoo\n"_s, u"b.txt"_s);
    KTextEditor::View *c = openText(u"no match\n"_s, u"c.txt"_s);
    QVERIFY(a && b && c);

    QVERIFY(search(u"foo"_s, MatchModel::OpenFiles));
    QCOMPARE(results(),
             u"a.txt\n"
             u"  1:1: [foo] bar\n"
             u"  2:5: baz [Foo] foo\n"
             u"  2:9: baz Foo [foo]\n"
             u"b.txt\n"
             u"  1:2: x[foo]\n"_s);

    // uncheck the second match of a.txt, like clicking its check box
    QTreeView *treeView = resultsView();
    QVERIFY(treeView);
    QAbstractItemModel *model = treeView->model();
    const QModelIndex fileA = model->index(0, 0, model->index(0, 0));
    QCOMPARE(fileA.data(MatchModel::FileUrlRole).toUrl().fileName(), u"a.txt"_s);
    const QModelIndex secondMatch = model->index(1, 0, fileA);
    QVERIFY(model->setData(secondMatch, Qt::Unchecked, Qt::CheckStateRole));
    QCOMPARE(secondMatch.data(Qt::CheckStateRole).value<Qt::CheckState>(), Qt::Unchecked);

    QVERIFY(replaceChecked(u"qux"_s));

    // all checked matches are replaced, in all files, the unchecked one is kept
    QCOMPARE(a->document()->text(), u"qux bar\nbaz Foo qux\n"_s);
    QCOMPARE(b->document()->text(), u"xqux\n"_s);
    QCOMPARE(c->document()->text(), u"no match\n"_s);

    // only in the documents, not saved
    QVERIFY(a->document()->isModified());
    QFile fileOnDisk(m_app->tempDir() + u"/a.txt"_s);
    QVERIFY(fileOnDisk.open(QIODevice::ReadOnly));
    QCOMPARE(QString::fromUtf8(fileOnDisk.readAll()), u"foo bar\nbaz Foo foo\n"_s);
}

void SearchPluginTest::testReplaceInFolder()
{
    // files on disk only, not opened
    const QString xPath = m_app->writeFile(u"replace/x.txt"_s, u"needle\nneedle\n"_s);
    const QString yPath = m_app->writeFile(u"replace/y.txt"_s, u"hay needle\n"_s);
    QVERIFY(!xPath.isEmpty() && !yPath.isEmpty());

    auto folderRequester = m_toolView->findChild<QWidget *>(u"folderRequester"_s);
    QVERIFY(folderRequester);
    QVERIFY(folderRequester->setProperty("url", QUrl::fromLocalFile(m_app->tempDir() + u"/replace"_s)));

    QVERIFY(search(u"needle"_s, MatchModel::Folder));
    QCOMPARE(results(),
             u"replace/x.txt\n"
             u"  1:1: [needle]\n"
             u"  2:1: [needle]\n"
             u"replace/y.txt\n"
             u"  1:5: hay [needle]\n"_s);

    QVERIFY(replaceChecked(u"pin"_s));

    // the files get opened to replace in them
    const auto documentText = [](const QString &path) {
        KTextEditor::Document *doc = KTextEditor::Editor::instance()->application()->findUrl(QUrl::fromLocalFile(path));
        return doc ? doc->text() : u"<not open>"_s;
    };
    QCOMPARE(documentText(xPath), u"pin\npin\n"_s);
    QCOMPARE(documentText(yPath), u"hay pin\n"_s);
}

void SearchPluginTest::testReplaceSingleMatch()
{
    KTextEditor::View *view = openText(u"one foo\ntwo foo\nthree foo\n"_s, u"a.txt"_s);
    QVERIFY(view);

    QVERIFY(search(u"foo"_s, MatchModel::CurrentFile));
    QCOMPARE(results(),
             u"a.txt\n"
             u"  1:5: one [foo]\n"
             u"  2:5: two [foo]\n"
             u"  3:7: three [foo]\n"_s);

    auto replaceCombo = m_toolView->findChild<QComboBox *>(u"replaceCombo"_s);
    auto replaceButton = m_toolView->findChild<QPushButton *>(u"replaceButton"_s);
    QVERIFY(replaceCombo && replaceButton);
    QVERIFY(replaceButton->isEnabled());
    replaceCombo->setCurrentText(u"bar"_s);

    // without a selected match, "Replace" only goes to the first match, the user should see what gets replaced
    replaceButton->click();
    QCOMPARE(view->document()->text(), u"one foo\ntwo foo\nthree foo\n"_s);
    QCOMPARE(m_app->mainWindow()->activeView(), view);
    QCOMPARE(view->cursorPosition(), KTextEditor::Cursor(0, 4));

    // now it replaces the selected match and goes to the next one
    replaceButton->click();
    QCOMPARE(view->document()->text(), u"one bar\ntwo foo\nthree foo\n"_s);
    QCOMPARE(view->cursorPosition(), KTextEditor::Cursor(1, 4));

    replaceButton->click();
    QCOMPARE(view->document()->text(), u"one bar\ntwo bar\nthree foo\n"_s);
    QCOMPARE(view->cursorPosition(), KTextEditor::Cursor(2, 6));
}

QTEST_MAIN(SearchPluginTest)

#include "SearchPluginTest.moc"
