// SPDX-FileCopyrightText: 2024 Kåre Särs <kare.sars@iki.fi>
// SPDX-License-Identifier: LGPL-2.0-or-later

#include "template.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Template w;
    QObject::connect(&w, &Template::done, &a, &QApplication::quit);
    w.show();

    return a.exec();
}
