#pragma once

#include <QObject>

class DiffWidgetTests : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void test_scrollbarAtTopOnOpen();
    void test_inlineDiff_data();
    void test_inlineDiff();
};
