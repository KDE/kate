
#include "kate_doc_manager_tests.h"
#include "katedocmanager.h"
#include "katemainwindow.h"

#include <KLocalizedString>

#include <QCommandLineParser>
#include <QSignalSpy>
#include <QTest>

QTEST_MAIN(KateDocManagerTests)

namespace
{
QCommandLineParser &GetParser()
{
    static QCommandLineParser parser;
    return parser;
}

KateDocumentInfo CreateMockDocument(QUrl url = {})
{
    KateDocumentInfo mockDocumentInfo;
    mockDocumentInfo.normalizedUrl = url;
    return mockDocumentInfo;
}
}

KateDocManagerTests::KateDocManagerTests(QObject *)
{
    // ensure ui file can be found and the translation domain is set to avoid warnings
    qApp->setApplicationName(QStringLiteral("kate"));
    KLocalizedString::setApplicationDomain(QByteArrayLiteral("kate"));
}

void KateDocManagerTests::setUp()
{
    auto tempdir = new QTemporaryDir;
    QVERIFY(tempdir->isValid());

    // ensure we use some dummy config
    KConfig::setMainConfigName(tempdir->path() + QStringLiteral("/testconfigfilerc"));

    app = std::make_unique<KateApp>(GetParser(), KateApp::ApplicationKWrite, tempdir->path());
}

void KateDocManagerTests::tearDown()
{
    app.reset(nullptr);
}

void KateDocManagerTests::canCreateDocument()
{
    setUp();

    auto documentManager = app->documentManager();

    QSignalSpy documentCreatedSpy(documentManager, &KateDocManager::documentCreated);
    const auto document = documentManager->createDoc(CreateMockDocument());
    Q_ASSERT(document != nullptr);
    Q_ASSERT(documentCreatedSpy.count() == 1);

    tearDown();
}

void KateDocManagerTests::popRecentlyClosedURLsClearsRecentlyClosedURLs()
{
    setUp();

    constexpr auto FirstTestURL = "Test URL 1";
    constexpr auto SecondTestURL = "Test URL 2";

    auto documentManager = app->documentManager();
    documentManager->closeAllDocuments();

    const auto createdDocuments = [&documentManager] {
        QList<KTextEditor::Document *> createdDocuments;
        createdDocuments.push_back(documentManager->createDoc(CreateMockDocument(QUrl(i18n(FirstTestURL)))));
        createdDocuments.push_back(documentManager->createDoc(CreateMockDocument(QUrl(i18n(SecondTestURL)))));
        return createdDocuments;
    }();

    documentManager->closeDocuments(createdDocuments, false);

    {
        const auto recentlyClosedURLs = documentManager->popRecentlyClosedURLs();
        Q_ASSERT(recentlyClosedURLs.size() == 2);
    }

    {
        const auto recentlyClosedURLs = documentManager->popRecentlyClosedURLs();
        Q_ASSERT(recentlyClosedURLs.size() == 0);
    }

    tearDown();
}

void KateDocManagerTests::popRecentlyClosedURLsReturnsNoneIfNoTabsClosedDuringSession()
{
    setUp();

    auto documentManager = app->documentManager();

    Q_ASSERT(documentManager->popRecentlyClosedURLs().empty());

    tearDown();
}

void KateDocManagerTests::popRecentlyClosedURLsReturnsURLIfTabClosedDuringSession()
{
    setUp();

    constexpr auto FirstTestURL = "Test URL 1";
    constexpr auto SecondTestURL = "Test URL 2";

    auto documentManager = app->documentManager();
    documentManager->closeAllDocuments();

    const auto createdDocuments = [&documentManager] {
        QList<KTextEditor::Document *> createdDocuments;
        createdDocuments.push_back(documentManager->createDoc(CreateMockDocument(QUrl(i18n(FirstTestURL)))));
        createdDocuments.push_back(documentManager->createDoc(CreateMockDocument(QUrl(i18n(SecondTestURL)))));
        return createdDocuments;
    }();

    const bool documentClosed = documentManager->closeDocuments({createdDocuments[0]}, false);
    Q_ASSERT(documentClosed);

    const auto recentlyClosedURLs = documentManager->popRecentlyClosedURLs();
    Q_ASSERT(recentlyClosedURLs.contains(QUrl(i18n(FirstTestURL))));
    Q_ASSERT(!recentlyClosedURLs.contains(QUrl(i18n(SecondTestURL))));

    tearDown();
}

void KateDocManagerTests::closeDocumentsWithEmptyURLsAreNotRestorable()
{
    setUp();

    auto documentManager = app->documentManager();
    documentManager->closeAllDocuments();

    const auto createdDocuments = [&documentManager] {
        QList<KTextEditor::Document *> createdDocuments;
        createdDocuments.push_back(documentManager->createDoc(CreateMockDocument()));
        createdDocuments.push_back(documentManager->createDoc(CreateMockDocument()));
        return createdDocuments;
    }();

    const bool documentsClosed = documentManager->closeDocuments(createdDocuments, false);
    Q_ASSERT(documentsClosed);

    const auto recentlyClosedURLs = documentManager->popRecentlyClosedURLs();
    Q_ASSERT(recentlyClosedURLs.isEmpty());

    tearDown();
}

#include "moc_kate_doc_manager_tests.cpp"