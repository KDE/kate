/*
    SPDX-FileCopyrightText: 2021 Ilia Kats <ilia-kats@gmx.net>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

// this is just to confirm that string sorting in Python is the same as sorting with Qt

#include "completiontable.h"

#include <algorithm>
#include <string>

#include <QObject>
#include <QStringView>
#include <QTest>

class LatexCompletionTableTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    static void testSorting()
    {
        for (int i = 0; i < n_completions - 1; ++i) {
            QStringView s1(completiontable[i].completion, completiontable[i].completion_strlen);
            QStringView s2(completiontable[i + 1].completion, completiontable[i + 1].completion_strlen);

            QVERIFY(s1 < s2);
        }
    }
};

QTEST_MAIN(LatexCompletionTableTest)

#include "testcompletiontable.moc"
