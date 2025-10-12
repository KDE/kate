/*
    SPDX-FileCopyrightText: 2025 Kåre Särs <kare.sars@iki.fi>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QByteArray>
#include <QByteArrayView>
#include <QDateTime>
#include <QHash>
#include <QList>
#include <QString>

struct CommitInfo {
    QByteArrayView hash;
    QString authorName;
    QDateTime authorDate;
    QByteArrayView summary;
};

struct BlamedLine {
    QByteArrayView shortCommitHash;
    QByteArrayView lineText;
};

class KateGitBlameParser
{
public:
    KateGitBlameParser();

    bool parseGitBlame(const QByteArray &rawData);
    bool hasBlameInfo() const;
    int blameLineCount() const;
    const QByteArrayView &blameLine(int lineNr);
    const CommitInfo &blameLineInfo(int lineNr);

    void clear();

private:
    QByteArray m_rawCommitData;
    QHash<QByteArray, CommitInfo> m_blameInfoForHash;
    std::vector<BlamedLine> m_blameLines;
};
