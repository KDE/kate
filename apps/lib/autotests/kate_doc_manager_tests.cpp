
#include "kate_doc_manager_tests.h"
#include "katedocmanager.h"

#include <KLocalizedString>

#include <QCommandLineParser>
#include <QSignalSpy>
#include <QTest>

QTEST_MAIN(KateDocManagerTests)

namespace
{
constexpr auto FirstTestUrl = "Test Url 1";
constexpr auto SecondTestUrl = "Test Url 2";

QCommandLineParser &getParser()
{
    static QCommandLineParser parser;
    return parser;
}

KateDocumentInfo createMockDocument(QUrl url = {})
{
    KateDocumentInfo mockDocumentInfo;
    mockDocumentInfo.normalizedUrl = url;
    return mockDocumentInfo;
}

QList<KTextEditor::Document *> createTestDocumentsWithUrls(KateDocManager *documentManager)
{
    QList<KTextEditor::Document *> createdDocuments;
    createdDocuments.push_back(documentManager->createDoc(createMockDocument(QUrl(i18n(FirstTestUrl)))));
    createdDocuments.push_back(documentManager->createDoc(createMockDocument(QUrl(i18n(SecondTestUrl)))));
    return createdDocuments;
}

QList<KTextEditor::Document *> createTestDocumentsWithoutUrls(KateDocManager *documentManager)
{
    QList<KTextEditor::Document *> createdDocuments;
    createdDocuments.push_back(documentManager->createDoc(createMockDocument()));
    createdDocuments.push_back(documentManager->createDoc(createMockDocument()));
    return createdDocuments;
}
}

KateDocManagerTests::KateDocManagerTests(QObject *)
{
    // ensure we are in test mode, for the part, too
    QStandardPaths::setTestModeEnabled(true);

    // ensure ui file can be found and the translation domain is set to avoid warnings
    qApp->setApplicationName(QStringLiteral("kate"));
    KLocalizedString::setApplicationDomain(QByteArrayLiteral("kate"));
}

void KateDocManagerTests::init()
{
    auto tempDir = std::make_unique<QTemporaryDir>();
    QVERIFY(tempDir->isValid());

    // ensure we use some dummy config
    KConfig::setMainConfigName(tempDir->path() + QStringLiteral("/testconfigfilerc"));

    app = std::make_unique<KateApp>(getParser(), KateApp::ApplicationKWrite, tempDir->path());
}

void KateDocManagerTests::cleanup()
{
    app.reset();
    tempDir.reset();
}

void KateDocManagerTests::canCreateDocument()
{
    auto documentManager = app->documentManager();

    QSignalSpy documentCreatedSpy(documentManager, &KateDocManager::documentCreated);
    const auto document = documentManager->createDoc(createMockDocument());
    Q_ASSERT(document != nullptr);
    Q_ASSERT(documentCreatedSpy.count() == 1);
}

void KateDocManagerTests::popRecentlyClosedUrlsClearsRecentlyClosedUrls()
{
    auto documentManager = app->documentManager();
    const auto createdDocuments = createTestDocumentsWithUrls(documentManager);

    documentManager->closeDocuments(createdDocuments, false);

    {
        const auto recentlyClosedUrls = documentManager->popRecentlyClosedUrls();
        Q_ASSERT(recentlyClosedUrls.size() == 2);
    }

    {
        const auto recentlyClosedUrls = documentManager->popRecentlyClosedUrls();
        Q_ASSERT(recentlyClosedUrls.size() == 0);
    }
}

void KateDocManagerTests::popRecentlyClosedUrlsReturnsNoneIfNoTabsClosedDuringSession()
{
    auto documentManager = app->documentManager();

    Q_ASSERT(documentManager->popRecentlyClosedUrls().empty());
}

void KateDocManagerTests::popRecentlyClosedUrlsReturnsUrlIfTabClosedDuringSession()
{
    auto documentManager = app->documentManager();
    const auto createdDocuments = createTestDocumentsWithUrls(documentManager);
    std::span<KTextEditor::Document *const> firstDoc{createdDocuments.data(), 1};

    const bool documentClosed = documentManager->closeDocuments(firstDoc, false);
    Q_ASSERT(documentClosed);

    const auto recentlyClosedUrls = documentManager->popRecentlyClosedUrls();
    Q_ASSERT(recentlyClosedUrls.contains(QUrl(i18n(FirstTestUrl))));
    Q_ASSERT(!recentlyClosedUrls.contains(QUrl(i18n(SecondTestUrl))));
}

void KateDocManagerTests::closedDocumentsWithEmptyUrlsAreNotRestorable()
{
    auto documentManager = app->documentManager();
    const auto createdDocuments = createTestDocumentsWithoutUrls(documentManager);

    const bool documentsClosed = documentManager->closeDocuments(createdDocuments, false);
    Q_ASSERT(documentsClosed);

    const auto recentlyClosedUrls = documentManager->popRecentlyClosedUrls();
    Q_ASSERT(recentlyClosedUrls.isEmpty());
}

#include "moc_kate_doc_manager_tests.cpp"
