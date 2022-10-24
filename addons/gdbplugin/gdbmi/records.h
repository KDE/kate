/*
    SPDX-FileCopyrightText: 2022 Héctor Mesa Jiménez <wmj.py@gmx.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef GDBMIRECORDS_H
#define GDBMIRECORDS_H

#include <QJsonObject>
#include <QString>
#include <optional>

namespace gdbmi
{

struct StreamOutput {
    enum { Console, Output, Log } channel;
    QString message;
};

struct Record {
    enum { Exec, Status, Notify, Result, Prompt } category;
    QString resultClass;
    QJsonObject value;
    std::optional<int> token;
};

}

#endif // GDBMIRECORDS_H
