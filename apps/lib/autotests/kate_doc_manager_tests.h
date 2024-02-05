
#pragma once

#include "kateapp.h"

#include <QObject>

class KateDocManagerTests : public QObject
{
    Q_OBJECT
public:
    KateDocManagerTests(QObject *parent = nullptr);

private:
    void setUp();
    void tearDown();

private Q_SLOTS:
    void canCreateDocument();
    void popRecentlyClosedUrlsClearsRecentlyClosedUrls();
    void popRecentlyClosedUrlsReturnsNoneIfNoTabsClosedDuringSession();
    void popRecentlyClosedUrlsReturnsUrlIfTabClosedDuringSession();
    void closedDocumentsWithEmptyUrlsAreNotRestorable();

private:
    std::unique_ptr<KateApp> app;
};