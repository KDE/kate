#pragma once

#include <QObject>
#include <QTest>

class GitBlameTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testBlameFiles();
};
