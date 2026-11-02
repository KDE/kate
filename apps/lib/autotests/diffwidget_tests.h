#pragma once

#include <QObject>

class DiffWidgetTests : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void test_scrollbarAtTopOnOpen();
    void test_inlineDiff_data();
    void test_inlineDiff();
    void test_editLineInMultiFileDiff_data();
    void test_editLineInMultiFileDiff();
};
