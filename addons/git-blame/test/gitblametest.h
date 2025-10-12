#pragma once

#include <QObject>
#include <QTest>

class GitBlameTest : public QObject
{
    Q_OBJECT

private slots:
    void testBlameFiles();
};
