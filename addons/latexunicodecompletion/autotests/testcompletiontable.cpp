/*
    SPDX-FileCopyrightText: 2021 Ilia Kats <ilia-kats@gmx.net>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

// this is just to confirm that string sorting in Python is the same as sorting with Qt

#include "completiontable.h"

#include <cstring>

#include <QObject>
#include <QTest>

class LatexCompletionTableTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testSorting()
    {
        for (int i = 0; i < n_completions - 1; ++i) {
            QVERIFY(strcmp(completiontable[i].completion, completiontable[i + 1].completion) < 0);
        }
    }
};

QTEST_MAIN(LatexCompletionTableTest)

#include "testcompletiontable.moc"
