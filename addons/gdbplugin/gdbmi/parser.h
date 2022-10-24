/*
    SPDX-FileCopyrightText: 2022 Héctor Mesa Jiménez <wmj.py@gmx.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef GDBMIPARSER_H
#define GDBMIPARSER_H

#include <QByteArray>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QStringList>
#include <optional>

#include "records.h"

namespace gdbmi
{

class GdbmiParser : public QObject
{
    Q_OBJECT
public:
    explicit GdbmiParser(QObject *parent = nullptr);

    /**
     * @brief parse gdb/mi responses
     *
     * @return last position successfully parsed
     */
    struct ParserHead {
        int last;
        bool error;
    };
    ParserHead parseResponse(const QByteArray &message);
    static bool isMIRequest(const QString &message);
    /**
     * @return MI command in the request message without "-" or nullopt
     */
    static std::optional<QString> getMICommand(const QString &message);
    /**
     * @brief Split token and command
     */
    static QStringList splitCommand(const QString &message);
    static bool isMISeparator(const QString &message);
    /**
     *
     * @return position of the last message separator. If a separator is multi-character ("\r\n"), the last char position is returned
     */
    static int splitLines(const QByteArray &message, bool lastIndex = false);

Q_SIGNALS:
    void outputProduced(const StreamOutput &record);
    void recordProduced(const Record &record);
    void parserError(const QString &error);

private:
    int parseRecord(const QByteArray &message, int position);
};

}

#endif // GDBMIPARSER_H
