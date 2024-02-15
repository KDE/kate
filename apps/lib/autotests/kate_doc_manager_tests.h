
#pragma once

#include "kateapp.h"

#include <QObject>

class KateDocManagerTests : public QObject
{
    Q_OBJECT
public:
    KateDocManagerTests(QObject *parent = nullptr);

private Q_SLOTS:
    void init();
    void cleanup();
    void canCreateDocument();
    void popRecentlyClosedUrlsClearsRecentlyClosedUrls();
    void popRecentlyClosedUrlsReturnsNoneIfNoTabsClosedDuringSession();
    void popRecentlyClosedUrlsReturnsUrlIfTabClosedDuringSession();
    void closedDocumentsWithEmptyUrlsAreNotRestorable();

private:
    std::unique_ptr<KateApp> app;
};